import time

import click
import cv2 as cv
import numpy as np
from serial import Serial
import csv
import threading

PORT = "COM10"
BAUDRATE = 115200

PREAMBLE = "!START!\r\n"
DELTA_PREAMBLE = "!DELTA!\r\n"
SUFFIX = "!END!\r\n"
LANE_SUFFIX = "!END_LANE!\r\n"

ROWS = 144
COLS = 174


@click.command()
@click.option(
    "-p", "--port", default=PORT, help="Serial (COM) port of the target board"
)
@click.option("-br", "--baudrate", default=BAUDRATE, help="Serial port baudrate")
@click.option("--timeout", default=1, help="Serial port timeout")
@click.option("--rows", default=ROWS, help="Number of rows in the image")
@click.option("--cols", default=COLS, help="Number of columns in the image")
@click.option("--preamble", default=PREAMBLE, help="Preamble string before the frame")
@click.option(
    "--delta_preamble",
    default=DELTA_PREAMBLE,
    help="Preamble before a delta update during video compression.",
)
@click.option(
    "--suffix", default=SUFFIX, help="Suffix string after receiving the frame"
)
@click.option(
    "--short-input",
    is_flag=True,
    default=False,
    help="If true, input is a stream of 4b values",
)
@click.option(
    "--shorter",
    is_flag=True,
    default=False,
    help="If true, input is a stream of 1b values (RLE)",
)
@click.option("--rle", is_flag=True, default=False, help="Run-Length Encoding")
@click.option("--quiet", is_flag=True, default=False, help="Print fewer messages")
@click.option("-w", "--write_to_file", is_flag=True, default=False, 
        help="Write a full frame to a CSV file and exit")
def monitor(
    port: str,
    baudrate: int,
    timeout: int,
    rows: int,
    cols: int,
    preamble: str,
    delta_preamble: str,
    suffix: str,
    short_input: bool,
    shorter: bool,
    rle: bool,
    quiet: bool,
    write_to_file: bool
) -> None:
    """
    Display images transferred through serial port. Press 'q' to close.
    """
    #TODO: speed up key presses with multithreading
    """
    cv.namedWindow("Motor control")
    cv.imshow("Motor control",np.zeros((100,100)))
    while True:
        char = cv.waitKey(1)
        if char  == ord("w") or char == ord("a") or char == ord("s") or char == ord("d"):
           ser.write(char)
    threading.Thread(target=wasd_control)
    while True:
        pass
    """

    prev_frame_ts = None  # keep track of frames per second
    frame = None

    click.echo(f"Opening communication on port {port} with baudrate {baudrate}")

    if isinstance(suffix, str):
        suffix = suffix.encode("ascii")

    if isinstance(preamble, str):
        preamble = preamble.encode("ascii")

    lane_suffix = LANE_SUFFIX.encode("ascii")

    if isinstance(delta_preamble, str):
        delta_preamble = delta_preamble.encode("ascii")

    img_rx_size = rows * cols
    if short_input:
        img_rx_size //= 2
    if shorter:
        img_rx_size //= 8
    if rle:
        img_rx_size *= 2

    partial_frame_counter = 0  # count partial updates every full frame

    with Serial(port, baudrate, timeout=timeout) as ser:
        while True:
            if not quiet:
                click.echo("Waiting for input data...")

            full_update = wait_for_preamble(ser, preamble, delta_preamble)

            if full_update:
                if not quiet:
                    click.echo(
                        f"Full update (after {partial_frame_counter} partial updates)"
                    )
                partial_frame_counter = 0
            else:
                if not quiet:
                    click.echo("Partial update")
                partial_frame_counter += 1

                if frame is None:
                    click.echo(
                        "No full frame has been received yet. Skipping partial update."
                    )
                    continue

            if not quiet:
                click.echo("Receiving picture...")

            try:
                (raw_data, lane_data) = get_raw_data(ser, img_rx_size, suffix, lane_suffix)
                if not quiet:
                    click.echo(f"Received {len(raw_data)} bytes")
            except ValueError as e:
                click.echo(f"Error while waiting for frame data: {e}")

            proct = time.time()
            if short_input:
                raw_data = (
                    expand_4b_to_8b(raw_data)
                    if not rle
                    else expand_4b_to_8b_rle(raw_data)
                )
            if shorter:
                raw_data = expand_1b_to_8b_rle(raw_data)
            elif rle and len(raw_data) % 2 != 0:
                # sometimes there serial port picks up leading 0s
                # discard these
                raw_data = raw_data[1:]

            if rle:
                raw_data = decode_rle(raw_data)

            try:
                new_frame = load_raw_frame(raw_data, rows, cols)
            except ValueError as e:
                click.echo(f"Malformed frame. {e}")
                continue

            frame = new_frame if full_update else frame + new_frame

            now = time.time()
            #click.echo(f"Serial monitor processing took {now-proct} s")
            if prev_frame_ts is not None:
                try:
                    fps = 1 / (now - prev_frame_ts)
                    click.echo(f"Frames per second: {fps:.2f}")
                except ZeroDivisionError:
                    click.echo("FPS too fast to measure")
            prev_frame_ts = now

            frame_w_lane= draw_lane(frame, lane_data, quiet)

            cv.namedWindow("Video Stream", cv.WINDOW_KEEPRATIO)
            cv.imshow("Video Stream", frame_w_lane)

            if write_to_file and full_update:
                frame = frame.reshape(-1,)
                np.savetxt('sample_raw_frame.csv',frame,fmt="%d")

                print("Data written to sample_raw_frame.csv successfully! Shape=", frame.shape)
                while cv.waitKey(1) != ord("q"): 
                    pass
                return

            # Wait for 'q' to stop the program
            char = cv.waitKey(1)
            if char  == ord("w") or char == ord("a") or char == ord("s") or char == ord("d"):
               num = ser.write(bytearray([char]))
               click.echo(chr(char).upper())
            if char == ord("q"):
                break

    cv.destroyAllWindows()

def wasd_control(ser):
    cv.namedWindow("Motor control")
    cv.imshow("Motor control",np.zeros((100,100)))
    while True:
        char = cv.waitKey(1)
        if char  == ord("w") or char == ord("a") or char == ord("s") or char == ord("d"):
           ser.write(char)
           click.echo(chr(char).upper())

def wait_for_preamble(ser: Serial, preamble: str, partial_preamble: str) -> bool:
    """
    Wait for a preamble string in the serial port.

    Returns `True` if next frame is full, `False` if it's a partial update.
    """
    while True:
        try:
            line = ser.readline()
            if line == preamble:
                return True
            elif line == partial_preamble:
                return False
        except UnicodeDecodeError:
            pass


def get_raw_data(ser: Serial, num_bytes: int, suffix: bytes = b"", 
        lane_suffix: bytes = b"") -> bytes:
    """
    Get raw frame data from the serial port.
    """
    rx_max_len = num_bytes + len(suffix)
    max_tries = 10_000
    raw_img = b""
    lane_data = b""

    for _ in range(max_tries):
        raw_img += ser.read(max(1, ser.in_waiting))
        suffix_idx = raw_img.find(suffix)
        lane_idx = raw_img.find(lane_suffix)
        if suffix_idx!=-1 and lane_idx != -1:
            lane_data = raw_img[suffix_idx+len(suffix):lane_idx]
            raw_img = raw_img[:suffix_idx]
            break

        if len(raw_img) >= rx_max_len:
            raw_img = raw_img[:num_bytes]
            break
    else:
        raise ValueError("Max tries exceeded.")

    return raw_img, lane_data

def draw_lane(frame : np.ndarray, lane_data : bytes, quiet: bool = False) -> np.ndarray:
    # grayscale to color
    frame = cv.merge([frame, frame, frame])
    topleftx=lane_data[1]
    toplefty=lane_data[2]
    botleftx=lane_data[3]
    botlefty=lane_data[4]
    toprightx=lane_data[5]
    toprighty=lane_data[6]
    botrightx=lane_data[7]
    botrighty=lane_data[8]
    stop=lane_data[9]
    steer_ang=int.from_bytes(bytes([lane_data[10]]),'big',signed='True')
    if not quiet:
        print("topleftx|toplefty|botleftx|botlefty|toprightx|toprighty|botrightx|botrighty|stop|steer_ang")
        print(topleftx,"|",toplefty,"|",botleftx,"|",botlefty,"|",toprightx,"|",toprighty,"|",botrightx,"|",botrighty,"|",stop,"|",steer_ang)
    if topleftx != 0xff and toplefty != 0xff and botleftx != 0xff and botlefty != 0xff:
        frame = cv.line(frame,(botleftx,botlefty),(topleftx,toplefty), (0,255,0),thickness=1)
    if toprightx != 0xff and toprighty != 0xff and botrightx != 0xff and botrighty != 0xff:
        frame = cv.line(frame,(botrightx,botrighty),(toprightx,toprighty), (0,255,0),thickness=1)
    if stop:
        frame = cv.arrowedLine(frame,(int(COLS/2),113),(COLS//2,114),(0,0,255),thickness=1,) #fix at y=30
        frame = cv.putText(frame,f"STOPPED",(0,20),cv.FONT_HERSHEY_PLAIN,1,(0,0,255))
    else:
        frame = cv.arrowedLine(frame,(int(COLS/2),144),(int(COLS/2+np.tan(steer_ang/180*np.pi)*30),114),(0,0,255),thickness=1,) #fix at y=30
        frame = cv.putText(frame,f"{steer_ang}",(10,20),cv.FONT_HERSHEY_PLAIN,1,(0,0,255))
    return frame
 

def expand_4b_to_8b(raw_data: bytes) -> bytes:
    """
    Transform an input of 4-bit encoded values into a string of 8-bit values.

    For example, value 0xFA gets converted to [0xF0, 0xA0]
    """
    return bytes(val for pix in raw_data for val in [pix & 0xF0, (pix & 0x0F) << 4])


def expand_4b_to_8b_rle(raw_data: bytes) -> bytes:
    """
    Transform an input of 4-bit encoded RLE values into a string of 8-bit values.

    For example, value 0xFA gets converted to [0xF0, 0x0A]
    """
    return bytes(val for pix in raw_data for val in [pix & 0xF0, pix & 0x0F])

# (1 bit data, 7 bit count)
def expand_1b_to_8b_rle(raw_data: bytes) -> bytes:
    """
    Transform an input of 1-bit encoded RLE values into a string of 8-bit values.

    For example, value 0xFA gets converted to [0b11, 0b11, 0b10, 0b10]
    """
    return bytes(val for pix in raw_data for val in [pix & 0x80, pix & 0x7F])


def decode_rle(raw_data: bytes) -> bytes:
    """
    Decode Run-Length Encoded data.
    """
    return bytes(
        val
        for pix, count in zip(raw_data[::2], raw_data[1::2])
        for val in [pix] * count
    )


def load_raw_frame(raw_data: bytes, rows: int, cols: int) -> np.array:
    return np.frombuffer(raw_data, dtype=np.uint8).reshape((rows, cols, 1))


if __name__ == "__main__":
    monitor()

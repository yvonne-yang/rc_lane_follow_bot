import cv2 as cv
import csv
import click
import numpy as np

ROWS = 144
COLS = 174

@click.command()
@click.option("-f", "--framefile", default="sample_raw_frame.csv")
@click.option("-l", "--lanefile", default="processed_lane_data.csv")
def main(framefile: str,
        lanefile: str):
    frame = np.loadtxt(framefile, dtype=np.uint8).reshape(ROWS, COLS, 1)
    assert frame.shape == (ROWS, COLS, 1), f"Expected frame shape ({ROWS}, {COLS}, 1) got {frame.shape}"
    frame = cv.merge([frame, frame, frame])

    # TODO: get these from file also
    with open(lanefile) as f:
        lines = f.readline()
        [topleft, botleft, topright, botright, angle] = [int(a) for a in lines.split(" ")]
        print(topleft, botleft, topright, botright, angle)

    frame = cv.line(frame,(botleft,135),(topleft,110), (0,255,0),thickness=1)
    frame = cv.line(frame,(botright,135),(topright,110), (0,255,0),thickness=1)
    frame = cv.arrowedLine(frame,(int(COLS/2),144),(int(COLS/2+np.tan(angle/180*np.pi)*30),114),(0,0,255),thickness=1,) #fix at y=30
    frame = cv.putText(frame,f"{angle}",(0,20),cv.FONT_HERSHEY_PLAIN,1,(0,0,255))
    #frame = cv.line(frame,(2, 135),(45, 110), (0,255,0),thickness=1)
    #frame = cv.line(frame,(118, 135),(145, 110), (0,255,0),thickness=1)
    cv.namedWindow("Video Stream", cv.WINDOW_KEEPRATIO)
    cv.imshow("Video Stream", frame)

    while cv.waitKey(1) != ord("q"):
        pass


if __name__ == "__main__":
    main()

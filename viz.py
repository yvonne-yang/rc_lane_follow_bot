import cv2 as cv
import csv
import click
import numpy as np

ROWS = 144
COLS = 174

@click.command()
@click.option("-f", "--filein", default="sample_processed_frame.csv")
def main(filein: str):
    frame = np.loadtxt(filein, dtype=np.uint8).reshape(ROWS, COLS, 1)
    assert frame.shape == (ROWS, COLS, 1), f"Expected frame shape ({ROWS}, {COLS}, 1) got {frame.shape}"
    frame = cv.merge([frame, frame, frame])
    frame = cv.line(frame,(6, 135),(45, 110), (0,255,0),thickness=1)
    frame = cv.line(frame,(122, 135),(140, 110), (0,255,0),thickness=1)
    #frame = cv.line(frame,(2, 135),(45, 110), (0,255,0),thickness=1)
    #frame = cv.line(frame,(118, 135),(145, 110), (0,255,0),thickness=1)
    cv.namedWindow("Video Stream", cv.WINDOW_KEEPRATIO)
    cv.imshow("Video Stream", frame)

    while cv.waitKey(1) != ord("q"):
        pass


if __name__ == "__main__":
    main()

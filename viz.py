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

    # TODO: get these from serial comm
    botleft=(5,135)
    topleft=(7,110)
    botright=(162,135)
    topright=(155,110)
    steer_ang=45 #degrees

    frame = cv.line(frame,botleft,topleft, (0,255,0),thickness=1)
    frame = cv.line(frame,botright,topright, (0,255,0),thickness=1)
    frame = cv.arrowedLine(frame,(int(COLS/2),144),(int(COLS/2+np.tan(steer_ang/180*np.pi)*30),114),(0,0,255),thickness=1,) #fix at y=30
    frame = cv.putText(frame,f"{steer_ang}",(0,20),cv.FONT_HERSHEY_PLAIN,1,(0,0,255))
    #frame = cv.line(frame,(2, 135),(45, 110), (0,255,0),thickness=1)
    #frame = cv.line(frame,(118, 135),(145, 110), (0,255,0),thickness=1)
    cv.namedWindow("Video Stream", cv.WINDOW_KEEPRATIO)
    cv.imshow("Video Stream", frame)

    while cv.waitKey(1) != ord("q"):
        pass


if __name__ == "__main__":
    main()

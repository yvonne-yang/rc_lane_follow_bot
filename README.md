# Set up
```
python -m pip install -r lab_06_serial_monitor/requirements.txt
```
in your Python virtual env. To run the serial monitor,
```
python lab_06_serial_monitor/serial_monitor_lab_06.py --shorter --rle
```

Credit to ECE342 staff for serial monitor code.

# Debugging lane detection logic with static image file
1. use `-w` option in serial monitor to generate a raw image
2. use main function in `detect_lane.c` to read the image, process it and output a new processed image (both as csv files)
3. run `viz.py` on the new image

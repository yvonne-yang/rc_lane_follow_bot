A remote controlled robot car with STM32 microcontroller that
drives 2 motors with an H-bridge, detects simple lanes, and
sends a camera feed via Bluetooth. Use WASD for control.

Parts: *STM32-Nucleo board, OV7670 camera module, batteries, [robot car kit](https://l.facebook.com/l.php?u=https%3A%2F%2Fwww.amazon.ca%2Fdp%2FB07DNYQ3PX%3Fref_%3Dcm_sw_r_apin_dp_09XPEA4QFPR8WXB3KS26%26fbclid%3DIwAR2Es9F5nJ0au_UZVhIQtDhIt4j6uwuk1HU4Qz8695tyReclHmcpV_ZWJIk%26th%3D1&h=AT23mDe87YTlFQjSv0jPHp9ko1E5vME0PBpvtXu6NEgxZ2-zuMBkG2JMdPOeX-mnSEujoOSSgVkD2bSgnOF_z_pJtvJIkxfx2PCRsAPKNoR68ZjgA8NZ2thQ1JmFyvJU-HcQA223LdqvN2GuxhZAMw), HC-05 Bluetooth module*

![Wiring diagram](https://github.com/yvonne-yang/rc_lane_follow_bot/blob/master/lane_follow_bot_wiring_diagram_img.png)

-----------
# Demo
(will upload a better one soon)

https://user-images.githubusercontent.com/60620007/232679950-e0a701e3-d84c-4e79-84d6-76f3de76ac9c.mp4

![image](https://user-images.githubusercontent.com/60620007/232680741-503269e5-49b5-46b6-acd9-cf624c48d748.png)
![image](https://user-images.githubusercontent.com/60620007/232680907-6388dc28-80b3-4f91-875b-38b891a6c9f5.png)



-----------

## Set up for developers
```
python -m pip install -r lab_06_serial_monitor/requirements.txt
```
in your Python virtual env. To run the serial monitor,
```
python lab_06_serial_monitor/serial_monitor_lab_06.py --shorter --rle
```

Credit to ECE342 staff for serial monitor code.

## Debugging lane detection logic with static image file
1. use `-w` option in serial monitor to generate a raw image
2. use main function in `detect_lane.c` to read the image, process it and output a new processed image (both as csv files)
3. run `viz.py` on the new image

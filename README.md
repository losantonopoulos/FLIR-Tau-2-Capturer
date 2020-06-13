# FLIR-Tau-2-Capturer

In order to utilize the core of Tau 2, a grabber was used. The grabber accesses the tau core and connects with our system through a USB port.
This is an OpenCV 2.4 based approach that utilizes libthermalgrabber which can be found here, along with the thermal grabber:
https://thermalcapture.com/thermalcapture-grabber-usb/

Install Instructions (For opencv2.4)
```shell_session
$ g++ -std=c++11 capturer.cpp -o capturer -lthermalgrabber -lopencv_core -lopencv_imgproc -lopencv_highgui 
```
OR simply for opencv2.4:
```shell_session
$ make capturer 
```
and for opencv3.4:
```shell_session
$ make capturer_v2
```

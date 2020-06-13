# FLIR-Tau-2-Capturer

In order to utilize the core of Tau 2, a grabber was used. The grabber accesses the tau core and connects with our system through a USB port.
This is an OpenCV 2.4/3.4 based approach that utilizes libthermalgrabber which can be found here, along with the thermal grabber:
https://thermalcapture.com/thermalcapture-grabber-usb/

__Install Instructions__

Simply run:
```shell_session
$ make capturer
$ make viewer 
```

__Excecute__

If you want to save the thermal feed, you need to run the capturing software like so:
```shell_session
$ ./capturer 	[OUTPUT]
```
If you only want to view the feed, simply run:
```shell_session
$ ./viewer 	[OUTPUT]
```

The capturing software will continously monitor the available storage space, and will stop saving once no more free space is availabe.

In order to QUIT both applications, simply click on the "Feed" window and then press ESC.  
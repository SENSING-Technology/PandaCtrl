# PANDA Camera Configuration Loader for Linux

This program is used to load camera configuration files for the PANDA device on Linux systems.

## 1. Compilation

### Dependencies

Before compiling, you need to install some necessary dependencies:

```
sudo apt install libudev-dev gcc make
sudo apt-get install i2c-tools
```

### Compilation

After installing the dependencies, navigate to the `PandaCtrl` directory and compile the executable program:

```
cd PandaCtrl/
make
```

## 2. Program Usage

To use the program, run the following command:

```
sudo ./PandaCtrl [Absolute path of the configuration file]
```

Some camera configuration files that our company currently supports are included in the `PandaCtrl` directory, so you can use them directly.

For example, if the absolute path of the configuration file you need to load is:

```
/home/workplace/uvcex/panda/uvctools/PandaCtrl/c/SG3S-ISX031C-GMSL2F-Hxxx_1920x1536@30fps_yuv.ini
```

The command to load the configuration file would be:

```
sudo ./PandaCtrl /home/workplace/uvcex/panda/uvctools/PandaCtrl/profilePath/SG3S-ISX031C-GMSL2F-Hxxx_1920x1536@30fps_yuv.ini
```

## 3. Verify Device Detection

After loading the configuration, check if the system has detected the device by running:

```
lsusb | grep SENSING_USB3_CAMERA
```

If the output is similar to:

```
Bus 002 Device 024: ID 04b4:00c3 Cypress Semiconductor Corp. SENSING_USB3_CAMERA
```

It means the system has detected the device, and you can open the system camera to view the image data transmitted by the PANDA device.
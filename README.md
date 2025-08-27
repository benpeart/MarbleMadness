# Marble Madness
A Digital Marble Roller maker project.

## Configure WiFi settings

When Marble Madness first boots up, it will attempt to connect to WiFi using the current settings.
If that fails, it will will go into "Access Point mode" which will allow you to access it via WiFi using your laptop or phone.

Connect tothe  Marble Madness WiFi access point with your device and follow the onscreen instructions to configure the WiFi settings.
Marble Madness will then reboot and connect using the updated settings.

Any time Marble Madness cannot connect with the current WiFi settings, it will go into AP mode so that the settings can be updated.
If you are still having problems, reboot Marble Madness (unplug/plug) twice a few seconds apart (less than 10) and the settings will be cleared and it will go into AP mode.

## Over-the-air updates

**Important note**: Over-the-air updates can only be done when Marble Madness is powered on and connected to the internet but in 'off' mode.

1. Build your new image
2. On Marble Madness (or via the app) select the mode 'off.'
3. In a web browser, go to the URL http://MarbleMadness/update
4. Login with the correct credentials (default is admin/admin)
5. Ensure the "Firmware" button is selected and click the "Choose File" button
6. Find the firmware you just built (the default filename is .pio/build/node32s/firmware.bin) and choose "Open"
7. The firmware will be uploaded to Marble Madness and then it will reboot

## REST API documentation

Marble Madness connects to the WiFi with the device name "MarbleMadness." The web ui and REST API can be found at http://MarbleMadness/. 
Alternately, check your router for the IP address.

There are four REST endpoints that make up the REST API:

1. "http://MarbleMadness/api/settings"
1. "http://MarbleMadness/api/modes"
1. "http://MarbleMadness/api/faces"

## 'Settings' REST API

A GET sent to the Settings endpoint will return a result that includes the following entries:
PUT will allow you to set some or all of the same values.

```
{
    "mode": "MarbleMadness",
    "brightness": 255,
    "speed": 25,
    "clockFace": "Off",
    "clockColor": "#FFFFFF"
}
```

### Mode

The mode specifies which of the available MarbleMadness modes is currently active. This can vary as more are added or removed but currently the list of available modes includes:

```
{
    "MarbleRoller",
    "xy_test",
    "test",
    "off"
}    
```        

### Brightness

The valid range is 0 (off) to 255 (bright). The default is 255.

### Speed

Speed is a value between 0 (slow) to 255 (fast). The default is 127.
Not all modes can honor all speeds; each one is responsible for making a 'best effort.'

### clockFace

Choose what clock face (if any) is displayed.

```
{
    "Off",
    "Digital",
    "Analog"
}
```

### clockColor

A hexadecimal color is specified with: #RRGGBB, where the RR (red), GG (green) and BB (blue) hexadecimal integers specify the components of the color.

## 'Modes' REST API

A GET sent to the /api/modes endpoint will return an array of the available modes.
PUT will allow you to set some or all of the same values.

A sample result would be the following:

```
["MarbleRoller","xy_test","test"]
```

## 'Faces' REST API

A GET sent to the /api/faces endpoint will return an array of the available clock faces.
PUT will allow you to set some or all of the same values.

A sample result would be the following:

```
["Off","Digital","Analog"]
```

# ESP8266 Status Light

Control an ESP8266 powered NeoPixel strip via a rest api or web portal. Tested and works well with ESP8266 12-E Node MCU and WeMos D1 Mini. Can work on an ESP-01 but is less reliable. NeoPixels are finnicky from the ESP-01.

## First Boot

### Connecting

On first boot, the device will create an access point named `esp8266` which will be password protected via the default password `newcouch`.

To configure the device, join the network and enter the password. After a few seconds you should be greeted with a captive portal where you can configure the settings. If for whatever reason the portal does not display, you can navigate there directly once connected to the device by opening a browser to `192.168.0.4`.

### Configuring

From the settings page, you should click "Configure WiFi" in order to connect the device to your network.

Choose your network from the list, and then enter your network's password.

#### Hostname

At this time you may also set a custom hostname for the device. Setting a friendly hostname will make it easier for you to interact with your device. For example, if you set the hostname to `martha`, you will hereon out be able to access the device on your local network as `http://martha.local`.

If for whatever reason the hostname resolution is not working (are you on Windows?) you will need to identify the ip address of the device manually. You can do this in one of two ways. Either go to your router's device table and identify the ESP8266, or plug the device into your computer and connect to it via Serial (BAUD 115200). Once connected to the network it will print its ip address to the Serial Monitor.

## Subsequent Boots

Your device will always attempt to reconnect to the previously saved network. If it is not able to establish a network connection, it will switch into access point mode and start the configuration portal, as for the first boot.

The configuration portal will reset after 60 seconds, at which point the device will restart and attempt to connect to the network again.

### LED Indicators

#### First Powered On

When the device is first powered on, it will set the NeoPixels to a solid blue state. It will remain blue until it is able to connect to the network. This means it will will remain blue while the configuration portal is open.

#### Connection Established

Once it establishes a connection with the network, it will turn the lights off and start responding to your commands.

#### Connection Lost

If it loses connection to the network after previously (on this boot) being connected, it will turn itself bright orange and "breathe". It will continue to attempt to reconnect to the network, and if successful, it will return to its previous light mode.

## Firmware Updates

There are three different ways to update the firmware on the device.

**Caution:** By updating the firmware you are taking responsibility into your hands. OTA updates will no longer be possible if the uploaded sketch requires more than 50% of the available space.

### Updating Firmware via USB

If you connect the device to your computer, you can directly upload new sketches to it using the Arduino IDE or PlatformIO.

### Updating Firmware via Config Portal

If you reset the device and access the config portal, you will see an "Update" button on the main menu. Click this button and provide it a compiled `firmware.bin` file. If all goes well, the new firmware will be flashed to the device, and the device will reboot.

### Updating Firmware via ArduinoOTA and `esptool.py`

Using the Arduino IDE, PlatformIO, or `esptool.py`, you can send your sketches directly to the board by providing the ip address of the board and the password that you configured in the setup portal.

## Web Portal

The web portal is hosted directly from the device. Access it at `http://{yourHostname}.local`. Upon page load the device's current status will be reflected. From here on out, the device's status will only be updated with each request you send. If you are triggering the device manually or from other sources, the web portal may be out of sync. You can always refresh the page to update the state.

If someone else is curious as to what your current status is, they can check by navigating to `http://{yourHostname}.local/status` in any browser.

A preview of the web portal can be found in `/var/www`. You can run this repo locally with `npx live-server ./var/www`. If running locally, add the local storage variable `__esp8266Proxy` to proxy requests to your device. For example, `localStorage.setItem('__esp8266Proxy', 'http://martha.local');` You'll see that very little of the webpage is actually found in `./include/html.h`, rather it loads resources from this repo (via jsdelivr) and relies on those to build the content.

## API

The _REST_ API gives you a great deal of control over the device itself. Although most endpoints are implemented directly in the web portal, you may always use the API directly if you want to control the device by other means--for example via slack or some other connected device/service.

The API is simple. All endpoints return JSON, regardless of any sent headers. Defying norms, `GET requests are used heavily as triggers to set device state.`

CORS is enabled, so you can make requests from any webpage or Chrome extension. As the API is open, you can access it by any means desired.

### Return Value

All endpoints return the current device state unless otherwise specified.

```
{
  "color": [
    255, // red [0, 255]
    0, // green [0, 255]
    0, // blue [0, 255]
    150 // alpha [0, 150]
  ],
  "mode_num": 10, // current mode number [0, 10]
  "mode": "off", // current mode name
  "brightness": 150, // same as color[3]
  "speed": 3, // light pattern speed [1,5]
  "status": "Unknown"
}
```

### Endpoints

#### Getters

###### `GET /config/state`

Get the current device state.

###### `GET /hostname`

Get the currently set hostname as `{ hostname }`.

###### `GET /status`

Get the current status as `{ status }`.

#### Setters

##### Core

###### `POST /config`

Configure any setting (or combination of settings) manually.

Accepts all values as shown in the [return value](#return-value).

Returns the updated config. Note that in some cases, certain configuration options may not be possible, or may take precedence over others. Consult the returned value to verify the current state of the device.

###### `GET /power/on`

Turn the lights on (reverts to the last known state). Ignored if lights are already on. Note that this will not reset the previous status, it will remain "Unknown" until it is set again.

###### `GET /power/off`

Turn the lights off. Ignored if lights are already off. This will also set the state to "Unknown".

###### `GET /power/toggle`

Turn lights off or revert to previous state. As this is a wrapper around `/power/off` and `/power/on`, see notes regarding the effect on the current status.

###### `GET /reset/wifi`

Clear the saved WiFi credentials. This will also disconnect you from the network.

###### `GET /reset/soft`

Clear the saved WiFi credentials and clear the flash storage (hostname/password). This will also disconnect you from the network.

###### `GET /reset/hard`

Clear the saved WiFi credentials, clear the flash storage (hostname/password), and force the device to reboot. This is glitchy, and usually results in the device reconnecting to the same WiFi network immediately. On a second reboot it will typically enter the config portal.

##### Status

###### `GET /status/free`

Mark self as "Free" and set lights green. Set solid if not in a color mode. Set brightness to max(50, previousBrightness).

###### `GET /status/busy`

Mark self as "Busy" and set lights pink. Set solid if not in a color mode. Set brightness to max(50, previousBrightness).

###### `GET /status/dnd`

Mark self as "DND" (do not disturb) and set lights red. Set solid if not in a color mode. Set brightness to max(50, previousBrightness).

###### `GET /status/unknown`

Mark self as "Unknown" but don't update any other settings.

##### Light Modes

###### `GET /config/mode/next`

Change to next light mode. Note that if you change light mode groups (color vs rainbow) your status will be changed.

###### `GET /config/mode/prev`

Change to previous mode light mode. Note that if you change light mode groups (color v rainbow) your status will be changed.

Further note that "previous" here implies a decrement in a series, and not the light mode that was last selected.

###### `GET /config/mode/solid`

Change mode to solid, retaining current color. Status is retained, unless you switch from "Party!", in which case it is set to "Unknown".

###### `GET /config/mode/breath`

Change mode to breath, retaining current color. Status is retained, unless you switch from "Party!", in which case it is set to "Unknown".

###### `GET /config/mode/marquee`

Change mode to marquee, retaining current color. Status is retained, unless you switch from "Party!", in which case it is set to "Unknown".

###### `GET /config/mode/theater`

Change mode to theater, retaining current color. Status is retained, unless you switch from "Party!", in which case it is set to "Unknown".

###### `GET /config/mode/rainbow`

Change mode to rainbow. Status is set to "Party!" unless you have a custom status set.

###### `GET /config/mode/rainbow/marquee && /config/mode/marquee/rainbow`

Change mode to rainbow marquee. Status is set to "Party!" unless you have a custom status set.

###### `GET /config/mode/rainbow/theater && /config/mode/theater/rainbow`

Change mode to rainbow theater. Status is set to "Party!" unless you have a custom status set.

##### Animation Speed

###### `GET /config/speed/slow`

Set speed to 1 (slowest).

###### `GET /config/speed/medium && /config/speed/med`

Set speed to 3.

###### `GET /config/speed/fast`

Set speed to 5 (fastest).

##### Brightness

###### `GET /config/brightness/low`

Set brightness to 25/150.

###### `GET /config/brightness/medium && /config/brightness/med`

Set brightness to 50/150.

###### `GET /config/brightness/high`

Set brightness to 150/150.

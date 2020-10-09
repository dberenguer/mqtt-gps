# MQTT-GPS

MQTT-GPS is a proof of concept about the connection of a commercial GPS receiver via MQTT. This code can work with most NMEA-based GPS receivers and can run on any ESP32 board. This project compiles on Arduino IDE.

## Initial requirements

1. This code needs to be compatible with [Adafruit's ultimate GPS module](https://www.adafruit.com/product/790)
2. The firmware must detect when the current GPS position enters a given geofence, specified by a coordinate and a radius.
3. Transmission of GPS date, time, position and alerts over MQTT.
4. Configuration of geofence settings via MQTT

## GPS receiver

[Adafruit's ultimate GPS module](https://www.adafruit.com/product/790) relies on the popular MTK3339 chipset made by Mediatek. Like most GNSS receivers in the market, the MTK3339 provides GPS data in NMEA 0183 format. Hardware connection to our ESP32 board is shown here:

|ES32          |GPS module |
| ---------    |-------- |
|3.3V          |Vcc      |
|GND           |GND      |
|GPIO34 (TXD1) |RXD      |
|GPIO12 (RXD1) |TXD      |

RXD and TXD pins need to be set accordingly to the available pins on your ESP32 board. Connecting VBACKUP on the GPS module to a 3V battery is also a good practice in order to keep GPS almanacs and ephemeris data even in case of a power loss.

## Arduino libraries

The following Arduino libraries are needed:

* [WiFiManager for ESP32 by zhouhan0126](https://github.com/zhouhan0126/WIFIMANAGER-ESP32) : this library deploys an initial access point and embedded web server as a way to configure your WiFi settings. This library needs to be manually installed on your Arduino library folder.
* [ArduinoJson](https://arduinojson.org/) : JSON library primarily used by our code to parse received commands via MQTT. This library can be installed from the Arduino library manager.
* [TinyGPSPlus by mikalhart](https://github.com/mikalhart/TinyGPSPlus) : this is a powerful parser of NMEA 0183 data. This library needs to be manually installed on your Arduino library folder.
* [PubSubClient by Nick O'Leary](https://github.com/knolleary/pubsubclient) : MQTT client for Arduino. This library can be installed from the Arduino library manager.

## Project files

This project is formed by five source files:

* mqtt-gps.ino : main arduino file containing the higher-level programming.
* config.h : configuration file where most of the static settings are declared.
* gps.h : wrapping class for the TynyGPSPlus library.
* mqttclient.h : MQTT wrapping class for PuSubClient.

## Configuration

The most common settings to be set on config.h are:

* GPS_TX_PIN : UART TXD pin to GPS
* GPS_RX_PIN : UART RXD pin to GPS
* MQTT_BROKER : IP address of the MQTT broker
* MQTT_PORT : MQTT port. Default is 1883
* TX_INTERVAL : transmission (MQTT) interval in milliseconds. Default is 30 sec.

## How it works

The device first connects to the WiFi network and transmits a first MQTT packet on the system topic:

```
topic: mqtt-gps/XXXXXXXXXXXX/system

message: connected
```

Where XXXXXXXXXXXX is the MAC address of the ESP32 SoC. Then, every 30 seconds a new MQTT packet containing GPS data is published on the gps topic:

```
topic: mqtt-gps/XXXXXXXXXXXX/gnss

message:
{
  "timestamp": "2020-10-09T13:08:26",
  "coordinates": [38.450936, -6.376491, 522.200000]
}
```

Only when a geofence alert is triggered, an alert is transmitted on the alert topic:

```
topic: mqtt-gps/XXXXXXXXXXXX/alert

message: ALERT GEOFENCE
```

Geofence alerts are transmitted the first time a GPS fix is detected within the geofence circle. Once the actual position goes out of the geofence again, the device is back allowed to trigger new alerts.

### Geofencing

Geofences are specified by the center of a circle and its radius. These settings can be configured via MQTT through the control topic:

```
topic: mqtt-gps/XXXXXXXXXXXX/control

message:
{
  "geofence": {
    "lat": <latitude>,
    "lon": <longitude>,
    "rad": <radius in meters>
  }
}
```

As result, the geofence can be set dynamically over MQTT depending on the actual position of the device. Once the device has received the new geofence settings a new packet is transmitted then confirming these new settings:

```
topic: mqtt-gps/XXXXXXXXXXXX/system

message:
{
  "geofence": {
    "lat": <latitude>,
    "lon": <longitude>,
    "rad": <radius in meters>
  }
}
```

Geofencing involves checking distances on a time basis. Fortunately, the TinyGPSPlus library provides a method called _distanceBetween_ that does this job for us.

## Open issues

Floats read with the ArduinoJson library are truncated to 2 decimal places for some reason. This issue needs further investigation


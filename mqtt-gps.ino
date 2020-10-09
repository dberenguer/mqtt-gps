/**
 * Copyright (c) 2020 panStamp <contact@panstamp.com>
 * 
 * This file is part of the RESPIRA project.
 * 
 * panStamp  is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 * 
 * panStamp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with panStamp; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
 * USA
 * 
 * Author: Daniel Berenguer
 * Creation date: Oct 9 2020
 */

#include <WiFi.h>
#include <esp_system.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFiManager.h>         // https://github.com/zhouhan0126/WIFIMANAGER-ESP32

#include <ArduinoJson.h>

#include "config.h"
#include "mqttclient.h"
#include "gps.h"

// Wifi manager
WiFiManager wifiManager;
const char wmPassword[] = "panstamp";

// Device MAC address
char deviceMac[16];

// Device ID
char deviceId[32];

/**
 * MQTT client
 */
MQTTCLIENT mqtt(MQTT_BROKER, MQTT_PORT);

/**
 * MQTT topics
 */
char systemTopic[64];
char gnssTopic[64];
char alertTopic[64];
char controlTopic[64];

// Timestamp of last transmission
uint32_t lastTxTime = 0;

// GPs object
GPS gps(GPS_RX_PIN, GPS_TX_PIN);

// Config space
CONFIG config;

// Geofence alert triggered
bool geoFenceAlert = false;

/**
 * Restart board
 */
void restart(void)
{
  Serial.println("Restarting system");
  
  // Restart ESP
  ESP.restart();  
}

/**
 * MQTT packet received
 * 
 * @param topic MQTT topic
 * @param MQTT payload
 */
void mqttReceived(char *topic, char *payload)
{
  Serial.print("MQTT command received: ");
  Serial.println(payload);

  // Deserialize JSON object
  DynamicJsonDocument jsonCmd(128);
  DeserializationError error = deserializeJson(jsonCmd, payload);

  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  // Pointer to command field
  const char *cmd = jsonCmd["cmd"];

  // Raw command
  if (cmd)
  {
    // Process command
    if (!strcasecmp(cmd, "restart"))
      restart();
  }
  // Geofence
  else
  {
    float lat = jsonCmd["geofence"]["lat"];
    if (!lat)    
    {
      Serial.println("No lattiude found in geofence command");
      return;
    }
  
    float lon = jsonCmd["geofence"]["lon"];
    if (!lon)
    {
      Serial.println("No longitude found in geofence command");
      return;
    }
    
    uint16_t rad = jsonCmd["geofence"]["rad"];
    if (!rad)
    {
      Serial.println("No radius found in geofence command");
      return;
    }

    Serial.println("New geofence");
    Serial.print("latitude = ");
    Serial.println(lat);
    Serial.print("longitude = ");
    Serial.println(lon);
    Serial.print("radius = ");
    Serial.println(rad);

    // Save new settings
    config.setGeoFence(lat, lon, rad);

    // Pass geofence settings to GPS object
    gps.setGeoFence(lat, lon, rad);

    // Publish new settings
    mqttTransmitSettings();
  }
}

/**
 * Transmit MQTT packet containing settings
 */
void mqttTransmitSettings(void)
{
  // Build JSON packet
  char packet[160];
  sprintf(packet, "{\"geofence\":{\"lat\":%f, \"lon\":%f, \"rad\":%d}}",
          config.getGeoFlatitude(), config.getGeoFlongitude(), config.getGeoFradius());

  // Publish settings
  mqtt.publish(systemTopic, packet);
}

/**
 * Transmit MQTT packet with GNSS data
 */
void mqttTransmitGnss(void)
{
  char json[256];
  gps.getData(json);

  // Publish last GPS readings
  mqtt.publish(gnssTopic, json);
}

/**
 * Arduino setup function
 */
void setup()
{ 
  Serial.begin(115200);
  Serial.println("Starting...");

  // Get MAC
  uint8_t mac[6];
  WiFi.softAPmacAddress(mac);
  sprintf(deviceMac, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  // Set device ID
  sprintf(deviceId, "%s %s", APP_NAME, deviceMac);

  // Initialize config space
  config.begin();

  // Buind MQTT topics
  sprintf(systemTopic, "%s/%s/system", MQTT_MAIN_TOPIC, deviceMac);
  sprintf(gnssTopic, "%s/%s/gnss", MQTT_MAIN_TOPIC, deviceMac);
  sprintf(alertTopic, "%s/%s/alert", MQTT_MAIN_TOPIC, deviceMac);
  sprintf(controlTopic, "%s/%s/control", MQTT_MAIN_TOPIC, deviceMac);  
  
  // WiFi Manager timeout
  wifiManager.setConfigPortalTimeout(300);

  // WiFi Manager autoconnect
  if (!wifiManager.autoConnect(deviceId))
  {
    Serial.println("failed to connect and hit timeout");
    ESP.restart();
    delay(1000);
  }
  else
  {
    Serial.println("");
    Serial.print("MAC address: ");
    Serial.println(deviceMac);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  // Subs cribe to control topic
  mqtt.subscribe(controlTopic);
  mqtt.attachInterrupt(mqttReceived);  

  // Connect to MQTT server
  Serial.println("Connecting to MQTT broker");
  if (mqtt.begin(deviceMac))
  {
    Serial.println("Connected to MQTT broker");
    mqtt.publish(systemTopic, "connected");
  }
 
  // Initialize GPS
  gps.begin();

  // Set geofence if valid settigns are available in config space
  if ((config.getGeoFradius() > 0) && config.getGeoFradius() < 0xFFFF)
    gps.setGeoFence(config.getGeoFlatitude(), config.getGeoFlongitude(), config.getGeoFradius());  
}

/**
 * Endless loop
 */
void loop()
{
  // Check GPS
  if (gps.run())
  {
    if (gps.checkGeofence())
    {
      if (!geoFenceAlert)
      {
        // Publish alert
        mqtt.publish(alertTopic, "ALERT GEOFENCE");

        geoFenceAlert = true;
      }
    }
    else
      geoFenceAlert = false;
  }

  // Time to transmit periodic reading?
  if (((millis() - lastTxTime) >= TX_INTERVAL))
  {
    lastTxTime = millis();

    // Publish MQTT packet containing GPS data
    mqttTransmitGnss();
  }
  else
    mqtt.handle();
}

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

#include <EEPROM.h>

/**
 * Application name
 */
const char APP_NAME[] = "mqtt-gps";

/**
 * Transmission interval (msec)
 */
const uint32_t TX_INTERVAL = 30000;

/**
 * MQTT broker settings
 */
const char MQTT_BROKER[] = "broker_ip_addr";
const uint16_t MQTT_PORT = 1883;
const char MQTT_USERNAME[] = "";
const char MQTT_PASSWORD[] =  "";

const char MQTT_MAIN_TOPIC[] = "mqtt-gps";

/**
 * GPS pins
 */
const uint8_t GPS_TX_PIN = 34;
const uint8_t GPS_RX_PIN = 12;

/**
 * Custom EEPROM addresses
 */
#define EEPROM_CENTER_LAT_ADDR  0
#define EEPROM_CENTER_LAT_SIZE  4
#define EEPROM_CENTER_LON_ADDR  EEPROM_CENTER_LAT_ADDR + EEPROM_CENTER_LAT_SIZE
#define EEPROM_CENTER_LON_SIZE  4
#define EEPROM_RADIUS_ADDR      EEPROM_CENTER_LON_ADDR + EEPROM_CENTER_LON_SIZE
#define EEPROM_RADIUS_SIZE      2
#define EEPROM_SIZE             EEPROM_CENTER_LAT_SIZE + EEPROM_CENTER_LON_SIZE + EEPROM_RADIUS_SIZE

/**
 * COORD data type
 */
typedef union COORD
{
  float f;
  uint8_t bytes[4];
};

/**
 * CENTER data type
 */
typedef struct CENTER
{
  COORD lat;
  COORD lon;
};

/**
 * Custom parameters to be saved in non-volatile space
 */
class CONFIG
{
  private:
    /**
     * Coordinates (lat, lon) of geofence center
     */
    CENTER center;

    /**
     * Radius of geofence circle in meters
     */
    uint16_t radius;
      
    /**
     * Read settings from non-volatile space
     */
    inline uint8_t read(void)
    {
      // Read latitude of geofence center
      for(uint8_t i=0 ; i<EEPROM_CENTER_LAT_SIZE ; i++)
        center.lat.bytes[i] = EEPROM.read(i + EEPROM_CENTER_LAT_ADDR);

      // Read longitude of geofence center
      for(uint8_t i=0 ; i<EEPROM_CENTER_LON_SIZE ; i++)
        center.lon.bytes[i] = EEPROM.read(i + EEPROM_CENTER_LON_ADDR);

      // Read radius 
      radius = 0;
      for(uint8_t i=0 ; i<EEPROM_RADIUS_SIZE ; i++)
      {
        radius <<= 8; 
        radius = EEPROM.read(i + EEPROM_RADIUS_ADDR);
      }
    }     

    /**
     * Save settings into non-volatile space
     */
    inline void save(void)
    {
      // Save latitude of geofence center
      for(uint8_t i=0 ; i<EEPROM_CENTER_LAT_SIZE ; i++)
        EEPROM.write(i + EEPROM_CENTER_LAT_ADDR, center.lat.bytes[i]);

      // Save longitude of geofence center
      for(uint8_t i=0 ; i<EEPROM_CENTER_LON_SIZE ; i++)
        EEPROM.write(i + EEPROM_CENTER_LON_ADDR, center.lon.bytes[i]);

      // Save radius of geofence
      for(uint8_t i=0 ; i<EEPROM_RADIUS_SIZE ; i++)
      {
        uint8_t b = radius >> (8 * (EEPROM_RADIUS_SIZE - i -1));
        EEPROM.write(i + EEPROM_RADIUS_ADDR, b);
      }

      EEPROM.commit();
    }
    
  public:     
    /**
     * Initialize config space
     */
    inline void begin(void)
    {
      EEPROM.begin(EEPROM_SIZE);
      read();
    }

    /**
     * Set geofence settings
     * 
     * @param lat : latitude of the new center of the geofence circle
     * @param lon : longitude of the new center of the geofence circle
     * @param rad : new radius of the geofence circle
     */
    inline void setGeoFence(float lat, float lon, uint16_t rad)
    {
      center.lat.f = lat;
      center.lon.f = lon;
      radius = rad; 
    }

    /**
     * Get lattiude of geofence center
     * 
     * @return latitude
     */
    inline float getGeoFlatitude(void)
    {
      return center.lat.f;
    }

    /**
     * Get longitude of geofence center
     * 
     * @return longitude
     */
    inline float getGeoFlongitude(void)
    {
      return center.lon.f;
    }

    /**
     * Get lattiude of geofence center
     * 
     * @return radius in meters
     */
    inline uint16_t getGeoFradius(void)
    {
      return radius;
    }
};

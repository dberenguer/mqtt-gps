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


#ifndef _GPS_H
#define _GPS_H

#include <Arduino.h>
#include <TinyGPS++.h>                       

/**
 * Main class
 */
class GPS
{
  private:
    /**
     * Tiny GPS object
     */
    TinyGPSPlus gps;

    /**
     * Serial port
     */
    HardwareSerial gpsSerial;

    /**
     * UART pins
     */
    uint8_t rxPin;
    uint8_t txPin;

    /**
     * Geofence coordinates
     */
    float geoFenceLat;
    float geoFenceLon;

    /**
     * Geofence radius
     */
    uint16_t geoFenceRad;

  public:
    /**
     * Class constructor
     * 
     * @param rx : UART RXD pin
     * @param tx : UART TXD pin
     */
    inline GPS(const uint8_t rx, const uint8_t tx) : gpsSerial(1)
    {
      rxPin = rx;
      txPin = tx;

      geoFenceLat = 0;
      geoFenceLon = 0;
      geoFenceRad = 0;
    }

    /**
     * Initialize GPS interface
     */
    inline void begin(void)
    {
      gpsSerial.begin(9600, SERIAL_8N1, txPin, rxPin);
    }

    /**
     * Process GPS data
     * 
     * @return true in case of valid GPS data received
     */
    inline bool run(void)
    {
      bool available = false;
      
      while (gpsSerial.available())
      {
        gps.encode(gpsSerial.read());
        available = true;
      }

      if (gps.date.month() == 0)
        return false;

      return available;
    }

    /**
     * Get latitude
     * 
     * @return latitude
     */
    inline float getLatitude(void)
    {
      return gps.location.lat();
    }

    /**
     * Get latitude
     * 
     * @return longitude
     */
    inline float getLongitude(void)
    {
      return gps.location.lng();
    }

    /**
     * Get time and location data in arJSONray format
     * 
     * @param json date/time/location json buffer
     */
    inline void getData(char *json)
    {
      sprintf(json, "{\"timestamp\": \"%04d-%02d-%02dT%02d:%02d:%02d\", \"coordinates\": [%f, %f, %f]}",
        gps.date.year(), gps.date.month(), gps.date.day(),
        gps.time.hour(), gps.time.minute(), gps.time.second(),
        gps.location.lat(), gps.location.lng(), gps.altitude.meters()
      );
    }

    /**
     * Set geofence settings
     * 
     * @param lat : lattiude of geofence center
     * @param lon : longitude of geofence center
     * @param rad : radius of geofence in meters
     */
    inline void setGeoFence(float lat, float lon, uint16_t rad)
    {
      geoFenceLat = lat;
      geoFenceLon = lon;
      geoFenceRad = rad;
    }

    /**
     * Check geofence against current position
     * 
     * @return true in case of current position within geofence. Return false otherwise
     */
    inline bool checkGeofence(void)
    {
      double distance = gps.distanceBetween(geoFenceLat, geoFenceLon, gps.location.lat(), gps.location.lng());

      // Within geofence?
      if (distance < geoFenceRad)
        return true;

      return false;
    }

};

#endif

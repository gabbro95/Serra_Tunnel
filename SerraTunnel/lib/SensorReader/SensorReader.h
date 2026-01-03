#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_BME280.h> 
#include <RTClib.h> 
#include <WiFi.h>
#include "time.h"

#include "../Config/Config.h"
#include "../DataStructures/DataStructures.h"
#include "../MySecrets/MySecrets.h"

class SensorReader {
public:
    SensorReader(SensorReadings_t* sensors, ActuatorStates_t* actuators);
    void initSensors();
    void readAllSensors();
    
private:
    SensorReadings_t* _sensorReadings;
    ActuatorStates_t* _actuatorStates;

    // Sensori I2C
    Adafruit_BME280 _bmeOffice;
    Adafruit_BME280 _bmeSerra1;
    Adafruit_BME280 _bmeSerra2;
    RTC_DS1307 _rtc; 

    // --- CONFIGURAZIONE ORARIO (NTP per ITALIA) ---
    const char* ntpServer = "pool.ntp.org";
    const long  gmtOffset_sec = 3600;      // Italia è GMT+1
    const int   daylightOffset_sec = 3600; // Ora legale (+1 ora extra d'estate)

    void readOfficeSensors();
    void readRTC();
    void readBME();
    void readLDR();
    void readDigitalSensors();

    void aggiornaRTCconWiFi();
    
    void tcaSelectChannel(uint8_t channel);
};

#endif
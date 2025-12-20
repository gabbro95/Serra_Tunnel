#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include "NimBLEDevice.h"
#include "../Config/Config.h" 
#include "../DataStructures/DataStructures.h"

// UUIDs Dashboard
#define SERVICE_UUID            "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// Sensori
#define CHAR_TEMP_SERRA_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHAR_TEMP_OFFICE_UUID   "beb5483e-36e1-4688-b7f5-ea07361b27a8" // Stato
#define CHAR_TEMP2_SERRA_UUID   "beb5483e-36e1-4688-b7f5-ea07361b28a8"
#define CHAR_TEMP3_SERRA_UUID   "beb5483e-36e1-4688-b7f5-ea07361b29a8"
#define CHAR_HUM_SERRA_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26a9"
#define CHAR_HUM1_SERRA_UUID    "beb5483e-36e1-4688-b7f5-ea07361b27a9"
#define CHAR_HUM2_SERRA_UUID    "beb5483e-36e1-4688-b7f5-ea07361b28a9"
#define CHAR_HUM3_SERRA_UUID    "beb5483e-36e1-4688-b7f5-ea07361b29a9"
#define CHAR_SOIL_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26aa"
#define CHAR_SOIL1_UUID         "beb5483e-36e1-4688-b7f5-ea07361b27aa"
#define CHAR_SOIL2_UUID         "beb5483e-36e1-4688-b7f5-ea07361b28aa"
#define CHAR_RTC_HOUR_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26ac" // Stato
#define CHAR_RTC_MINUTE_UUID    "beb5483e-36e1-4688-b7f5-ea07361b27ac" // Stato

// Attuatori/Comandi
#define CHAR_FAN_UUID           "826b526d-8956-4299-8051-168d1840614e"
#define CHAR_WATER_UUID         "826b526d-8956-4299-8051-168d1840614f"
#define CHAR_HEATER_UUID        "826b526d-8956-4299-8051-168d18406151" // Stato
#define CHAR_CURTAIN_UUID       "826b526d-8956-4299-8051-168d18406152" // Stato (Tende)
#define CHAR_AUTO_UUID          "826b526d-8956-4299-8051-168d18406150"
#define CHAR_CALL_MODE_UUID     "826b526d-8956-4299-8051-168d18406149"
#define CHAR_EXTRACTOR_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26c2" // Stato Estrattore

class BLEManager {
public:
    BLEManager(SensorReadings_t* sensors, ActuatorStates_t* actuators);
    void initBLE();
    void notifySensors();
    bool isConnected() { return _deviceConnected; }

private:
    SensorReadings_t* _sensorReadings;
    ActuatorStates_t* _actuatorStates;

    NimBLEServer* _pServer;
    NimBLEService* _pService;
    
    // Characteristics
    NimBLECharacteristic* _tempSerraChar;
    NimBLECharacteristic* _tempOfficeChar;
    NimBLECharacteristic* _temp2SerraChar;
    NimBLECharacteristic* _temp3SerraChar;
    NimBLECharacteristic* _humSerraChar;
    NimBLECharacteristic* _hum1SerraChar;
    NimBLECharacteristic* _hum2SerraChar;
    NimBLECharacteristic* _hum3SerraChar;
    NimBLECharacteristic* _soilChar;
    NimBLECharacteristic* _soil1Char;
    NimBLECharacteristic* _soil2Char; 

    NimBLECharacteristic* _rtcHourChar; 
    NimBLECharacteristic* _rtcMinuteChar; 
    
    NimBLECharacteristic* _fanChar;
    NimBLECharacteristic* _waterChar;
    NimBLECharacteristic* _heaterChar;     
    NimBLECharacteristic* _extractorChar;
    NimBLECharacteristic* _curtainChar;    
    NimBLECharacteristic* _autoChar;
    NimBLECharacteristic* _callModeChar;

    bool _deviceConnected = false;
    
    class ServerCallbacks : public NimBLEServerCallbacks {
    public:
        ServerCallbacks(BLEManager* manager) : _manager(manager) {}
        void onConnect(NimBLEServer* pServer) override { _manager->_deviceConnected = true; }
        void onDisconnect(NimBLEServer* pServer) override { 
            _manager->_deviceConnected = false; 
            NimBLEDevice::getAdvertising()->start(); 
        }
    private:
        BLEManager* _manager;
    };
    
    class ActuatorCallbacks : public NimBLECharacteristicCallbacks {
    public:
        ActuatorCallbacks(ActuatorStates_t* actuators);
        void onWrite(NimBLECharacteristic* pCharacteristic) override;
    private:
        ActuatorStates_t* _actuatorStates;
    };

    ActuatorCallbacks* _controlCallback; 
};

#endif
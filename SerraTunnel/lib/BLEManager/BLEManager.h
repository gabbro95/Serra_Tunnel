#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

#include "NimBLEDevice.h"
#include "../Config/Config.h" 
#include "../DataStructures/DataStructures.h"

#define SERVICE_UUID            "4fafc201-1fb5-459e-8fcc-c5c9c331914b"

// --- UUIDs SENSORI ---
#define CHAR_TEMP_AVG_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26a8" 
#define CHAR_HUM_AVG_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26a9" 
#define CHAR_TEMP_MIN_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26ab" 
#define CHAR_TEMP_MAX_UUID      "beb5483e-36e1-4688-b7f5-ea07361b26ac"
#define CHAR_HUM_MIN_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26ad"
#define CHAR_HUM_MAX_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26ae"
#define CHAR_LUX_UUID           "beb5483e-36e1-4688-b7f5-ea07361b26aa" 

// Zone
#define CHAR_TEMP_NORD_UUID     "beb5483e-36e1-4688-b7f5-ea07361b27a8"
#define CHAR_HUM_NORD_UUID      "beb5483e-36e1-4688-b7f5-ea07361b27a9"
#define CHAR_TEMP_CENTRO_UUID   "beb5483e-36e1-4688-b7f5-ea07361b27a0"
#define CHAR_HUM_CENTRO_UUID    "beb5483e-36e1-4688-b7f5-ea07361b27a1"
#define CHAR_TEMP_SUD_UUID      "beb5483e-36e1-4688-b7f5-ea07361b27a2"
#define CHAR_HUM_SUD_UUID       "beb5483e-36e1-4688-b7f5-ea07361b27a3"

// Diagnostica e Stato
#define CHAR_BATTERY_UUID       "beb5483e-36e1-4688-b7f5-ea07361b28a8"
#define CHAR_RTC_TIME_UUID      "beb5483e-36e1-4688-b7f5-ea07361b29a8"
#define CHAR_RTC_DATE_UUID      "beb5483e-36e1-4688-b7f5-ea07361b29a9" 
#define CHAR_SYSTEM_STATUS_UUID "beb5483e-36e1-4688-b7f5-ea07361b31a8"

// --- UUIDs CONTROLLI ---
#define CHAR_AUTO_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26b0"
#define CHAR_PUMP_UUID          "beb5483e-36e1-4688-b7f5-ea07361b26b1"
#define CHAR_HEATER_UUID        "beb5483e-36e1-4688-b7f5-ea07361b26b2"
#define CHAR_EXTRACTOR_UUID     "beb5483e-36e1-4688-b7f5-ea07361b26b3"
#define CHAR_CURTAIN_UUID       "beb5483e-36e1-4688-b7f5-ea07361b26b4"
#define CHAR_GROW_LIGHTS_UUID   "beb5483e-36e1-4688-b7f5-ea07361b26b5"

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
    bool _deviceConnected = false;

    NimBLECharacteristic* _tempAvgChar;
    NimBLECharacteristic* _humAvgChar;
    NimBLECharacteristic* _tempMinChar;
    NimBLECharacteristic* _tempMaxChar;
    NimBLECharacteristic* _humMinChar; 
    NimBLECharacteristic* _humMaxChar; 
    NimBLECharacteristic* _luxChar;
    
    NimBLECharacteristic* _tempNordChar;
    NimBLECharacteristic* _humNordChar;
    NimBLECharacteristic* _tempCentroChar;
    NimBLECharacteristic* _humCentroChar;
    NimBLECharacteristic* _tempSudChar;
    NimBLECharacteristic* _humSudChar;
    
    NimBLECharacteristic* _batteryChar;
    NimBLECharacteristic* _rtcTimeChar;
    NimBLECharacteristic* _rtcDateChar;
    NimBLECharacteristic* _systemStatusChar;

    NimBLECharacteristic* _autoChar;
    NimBLECharacteristic* _pumpChar;
    NimBLECharacteristic* _heaterChar;
    NimBLECharacteristic* _extractorChar;
    NimBLECharacteristic* _curtainChar;
    NimBLECharacteristic* _growLightsChar;

    class ServerCallbacks : public NimBLEServerCallbacks {
        BLEManager* _manager;
    public:
        ServerCallbacks(BLEManager* manager) : _manager(manager) {}
        void onConnect(NimBLEServer* pServer) override;
        void onDisconnect(NimBLEServer* pServer) override;
    };

    class ActuatorCallbacks : public NimBLECharacteristicCallbacks {
        ActuatorStates_t* _actuatorStates;
        SensorReadings_t* _sensorReadings;
    public:
        ActuatorCallbacks(ActuatorStates_t* actuators, SensorReadings_t* sensors) 
            : _actuatorStates(actuators), _sensorReadings(sensors) {}
        void onWrite(NimBLECharacteristic* pCharacteristic) override;
    };
};

#endif
#include <Arduino.h>
#include "Wire.h"

// Inclusione delle classi
#include "../Config/Config.h"
#include "../DataStructures/DataStructures.h"
#include "../BLEManager/BLEManager.h"
#include "../SensorReader/SensorReader.h"
#include "../LogicController/LogicController.h"
#include "../Timer/Timer.h"

// Globals
SensorReadings_t sensorData;
ActuatorStates_t actuatorData;

// Objects
SensorReader* sensorReader;
LogicController* logicController;
BLEManager* bleManager;

// Timers
Timer lastSensorReadTime(sensorReadInterval); const long sensorReadInterval = 5000; 
Timer lastBleNotifyTime(bleNotifyInterval); const long bleNotifyInterval = 2000; 


void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10); 
    Serial.println("\n--- Smart GreenOffice Hub (ESP32) ---");

    logicController = new LogicController(&sensorData, &actuatorData);
    logicController->initLogic();

    sensorReader = new SensorReader(&sensorData, &actuatorData);
    sensorReader->initSensors();

    bleManager = new BLEManager(&sensorData, &actuatorData);
    bleManager->initBLE();
    
    logicController->setControlHourTimer(DELAYHOUR);

    lastSensorReadTime.set_auto_reset(true);
    lastBleNotifyTime.set_auto_reset(true);
    
    lastSensorReadTime.start();
    lastBleNotifyTime.start();
    Serial.println("System Ready.");
}

void loop() {
    if (lastSensorReadTime.update()) {
        sensorReader->readAllSensors();
        
        Serial.printf("DEBUG: Office: %.1f°C | Serra Avg: %.1f°C | Soil: %.1f%%\n", 
            sensorData.tempOffice, sensorData.tempSerraAverage, sensorData.soilAverage);
    }

    logicController->runLogicCycle();
    
    if (bleManager->isConnected() && lastBleNotifyTime.update()) bleManager->notifySensors();
}
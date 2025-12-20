#include "BLEManager.h"

BLEManager::ActuatorCallbacks::ActuatorCallbacks(ActuatorStates_t* actuators)
    : _actuatorStates(actuators) {}

void BLEManager::ActuatorCallbacks::onWrite(NimBLECharacteristic* pCharacteristic) {
    uint8_t value = pCharacteristic->getValue().length() == 0 ? 0 : *pCharacteristic->getValue().data();
    bool newState = (value == 1);
    std::string uuidObj = pCharacteristic->getUUID().toString();
    const char* charUuid = uuidObj.c_str();

    // 1. Auto Mode
    if (strcmp(charUuid, CHAR_AUTO_UUID) == 0) {
        _actuatorStates->isAutoModeActive = newState;
    }
    // 2. Impostazione Stato Tende (Funziona anche in Auto)
    else if (strcmp(charUuid, CHAR_CALL_MODE_UUID) == 0) {
        _actuatorStates->areCurtainsOpen = newState;
        Serial.printf("BLE: Call su: %s\n", newState ? "Attiva" : "Disattiva");
    }
    // 2. Impostazione Stato Tende (Funziona anche in Auto)
    else if (strcmp(charUuid, CHAR_CURTAIN_UUID) == 0) {
        _actuatorStates->areCurtainsOpen = newState;
        Serial.printf("BLE: Tende impostate su: %s\n", newState ? "APERTE" : "CHIUSE");
    }
    // 3. Controlli Manuali (Solo se Auto è OFF)
    else {
        if (_actuatorStates->isAutoModeActive) { return; } // Ignora

        if (strcmp(charUuid, CHAR_FAN_UUID) == 0) {
            _actuatorStates->fanMode = newState ? FAN_SILENT : FAN_OFF;
            _actuatorStates->fanSpeedPWM = newState ? FAN_SPEED_SILENT_PWM : 0;
        } else if (strcmp(charUuid, CHAR_WATER_UUID) == 0) {
            _actuatorStates->isPumpOn = newState;
        } else if (strcmp(charUuid, CHAR_HEATER_UUID) == 0) {
            _actuatorStates->isHeaterOn = newState;
        } else if (strcmp(charUuid, CHAR_EXTRACTOR_UUID) == 0) {
            _actuatorStates->isExtractorOn = newState;
        }
    }
    
    pCharacteristic->setValue(&value, 1);
    pCharacteristic->notify();
}

BLEManager::BLEManager(SensorReadings_t* sensors, ActuatorStates_t* actuators) 
    : _sensorReadings(sensors), _actuatorStates(actuators) {
    _controlCallback = new ActuatorCallbacks(actuators);
}

void BLEManager::initBLE() {
    NimBLEDevice::init("Serra_Hub_Faiti"); 
    _pServer = NimBLEDevice::createServer();
    _pServer->setCallbacks(new ServerCallbacks(this));
    _pService = _pServer->createService(SERVICE_UUID);
    
    // Sensori
    _tempSerraChar = _pService->createCharacteristic(CHAR_TEMP_SERRA_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    _tempOfficeChar = _pService->createCharacteristic(CHAR_TEMP_OFFICE_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY); 
    _temp2SerraChar = _pService->createCharacteristic(CHAR_TEMP2_SERRA_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    _temp3SerraChar = _pService->createCharacteristic(CHAR_TEMP3_SERRA_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    _humSerraChar = _pService->createCharacteristic(CHAR_HUM_SERRA_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    _hum1SerraChar = _pService->createCharacteristic(CHAR_HUM1_SERRA_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    _hum2SerraChar = _pService->createCharacteristic(CHAR_HUM2_SERRA_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    _hum3SerraChar = _pService->createCharacteristic(CHAR_HUM3_SERRA_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    _soilChar = _pService->createCharacteristic(CHAR_SOIL_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    
    //  Orologio
    _rtcHourChar = _pService->createCharacteristic(CHAR_RTC_HOUR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY); 
    _rtcMinuteChar = _pService->createCharacteristic(CHAR_RTC_MINUTE_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY); 

    // Attuatori
    _autoChar = _pService->createCharacteristic(CHAR_AUTO_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE);
    _autoChar->setCallbacks(_controlCallback);
    
    _callModeChar = _pService->createCharacteristic(CHAR_CALL_MODE_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE);
    _callModeChar->setCallbacks(_controlCallback);
    
    _curtainChar = _pService->createCharacteristic(CHAR_CURTAIN_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE);
    _curtainChar->setCallbacks(_controlCallback);

    _fanChar = _pService->createCharacteristic(CHAR_FAN_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE);
    _fanChar->setCallbacks(_controlCallback);
    
    _waterChar = _pService->createCharacteristic(CHAR_WATER_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE);
    _waterChar->setCallbacks(_controlCallback);

    _heaterChar = _pService->createCharacteristic(CHAR_HEATER_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE);
    _heaterChar->setCallbacks(_controlCallback);
    
    _extractorChar = _pService->createCharacteristic(CHAR_EXTRACTOR_UUID, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE);
    _extractorChar->setCallbacks(_controlCallback);
    
    _pService->start();
    NimBLEDevice::getAdvertising()->start();
}

void BLEManager::notifySensors() {
    if (!_deviceConnected) return;
    char buffer[10];

    // Invio Dati
    sprintf(buffer, "%.1f", _sensorReadings->tempSerraAverage);
    _tempSerraChar->setValue(buffer); _tempSerraChar->notify();

    sprintf(buffer, "%.1f", _sensorReadings->tempOffice); 
    _tempOfficeChar->setValue(buffer); _tempOfficeChar->notify();

    sprintf(buffer, "%.1f", _sensorReadings->tempSerra1);
    _temp2SerraChar->setValue(buffer); _temp2SerraChar->notify();

    sprintf(buffer, "%.1f", _sensorReadings->tempSerra2);
    _temp3SerraChar->setValue(buffer); _temp3SerraChar->notify();

    sprintf(buffer, "%.1f", _sensorReadings->humSerraAverage);
    _humSerraChar->setValue(buffer); _humSerraChar->notify();

    sprintf(buffer, "%.1f", _sensorReadings->humOffice);
    _hum1SerraChar->setValue(buffer); _hum1SerraChar->notify();

    sprintf(buffer, "%.1f", _sensorReadings->humSerra1);
    _hum1SerraChar->setValue(buffer); _hum1SerraChar->notify();

    sprintf(buffer, "%.1f", _sensorReadings->humSerra2);
    _hum2SerraChar->setValue(buffer); _hum2SerraChar->notify();



    sprintf(buffer, "%.1f", _sensorReadings->soilAverage);
    _soilChar->setValue(buffer); _soilChar->notify();

    sprintf(buffer, "%.1f", _sensorReadings->soil1);
    _soil1Char->setValue(buffer); _soil1Char->notify();

    sprintf(buffer, "%.1f", _sensorReadings->soil2);
    _soil2Char->setValue(buffer); _soil2Char->notify();


    sprintf(buffer, "%i", _sensorReadings->currentHour);
    _rtcHourChar->setValue(buffer); _rtcHourChar->notify();

    sprintf(buffer, "%i", _sensorReadings->currentMinute);
    _rtcMinuteChar->setValue(buffer); _rtcMinuteChar->notify();

    // Sincronizza Stati
    uint8_t val;
    val = _actuatorStates->isAutoModeActive; _autoChar->setValue(&val, 1); _autoChar->notify();
    val = _actuatorStates->isCallModeActive; _callModeChar->setValue(&val, 1); _callModeChar->notify();
    val = _actuatorStates->areCurtainsOpen;  _curtainChar->setValue(&val, 1); _curtainChar->notify();
    val = _actuatorStates->isHeaterOn;       _heaterChar->setValue(&val, 1); _heaterChar->notify();
    val = (_actuatorStates->fanMode != FAN_OFF); _fanChar->setValue(&val, 1); _fanChar->notify();
    val = _actuatorStates->isPumpOn;         _waterChar->setValue(&val, 1); _waterChar->notify();
    val = _actuatorStates->isExtractorOn;         _extractorChar->setValue(&val, 1); _extractorChar->notify();
}
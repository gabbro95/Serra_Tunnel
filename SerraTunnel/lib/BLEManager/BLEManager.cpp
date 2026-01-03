#include "BLEManager.h"

BLEManager::BLEManager(SensorReadings_t* sensors, ActuatorStates_t* actuators) 
    : _sensorReadings(sensors), _actuatorStates(actuators) {}

void BLEManager::initBLE() {
    NimBLEDevice::init("GreenOffice_Hub");
    _pServer = NimBLEDevice::createServer();
    _pServer->setCallbacks(new ServerCallbacks(this));

    NimBLEService* pService = _pServer->createService(SERVICE_UUID);

    auto createReadNotify = [&](const char* uuid) {
        return pService->createCharacteristic(uuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    };

    _tempAvgChar = createReadNotify(CHAR_TEMP_AVG_UUID);
    _humAvgChar  = createReadNotify(CHAR_HUM_AVG_UUID);
    _tempMinChar = createReadNotify(CHAR_TEMP_MIN_UUID);
    _tempMaxChar = createReadNotify(CHAR_TEMP_MAX_UUID);
    _humMinChar  = createReadNotify(CHAR_HUM_MIN_UUID); // INIT
    _humMaxChar  = createReadNotify(CHAR_HUM_MAX_UUID); // INIT
    _luxChar     = createReadNotify(CHAR_LUX_UUID);
    
    _tempNordChar = createReadNotify(CHAR_TEMP_NORD_UUID);
    _humNordChar  = createReadNotify(CHAR_HUM_NORD_UUID);
    _tempCentroChar = createReadNotify(CHAR_TEMP_CENTRO_UUID);
    _humCentroChar  = createReadNotify(CHAR_HUM_CENTRO_UUID);
    _tempSudChar    = createReadNotify(CHAR_TEMP_SUD_UUID);
    _humSudChar     = createReadNotify(CHAR_HUM_SUD_UUID);

    _batteryChar      = createReadNotify(CHAR_BATTERY_UUID);
    _rtcTimeChar      = createReadNotify(CHAR_RTC_TIME_UUID);
    _rtcDateChar      = createReadNotify(CHAR_RTC_DATE_UUID);
    _systemStatusChar = createReadNotify(CHAR_SYSTEM_STATUS_UUID);

    auto actCallbacks = new ActuatorCallbacks(_actuatorStates, _sensorReadings);
    auto setupControl = [&](NimBLECharacteristic* &c, const char* uuid) {
        c = pService->createCharacteristic(uuid, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY);
        c->setCallbacks(actCallbacks);
    };

    setupControl(_autoChar, CHAR_AUTO_UUID);
    setupControl(_pumpChar, CHAR_PUMP_UUID);
    setupControl(_heaterChar, CHAR_HEATER_UUID);
    setupControl(_extractorChar, CHAR_EXTRACTOR_UUID);
    setupControl(_curtainChar, CHAR_CURTAIN_UUID);
    setupControl(_growLightsChar, CHAR_GROW_LIGHTS_UUID);

    pService->start();
    NimBLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
    NimBLEDevice::getAdvertising()->start();
}

void BLEManager::notifySensors() {
    if (!_deviceConnected) return;

    _tempAvgChar->setValue(_sensorReadings->tempSerraAverage); _tempAvgChar->notify();
    _humAvgChar->setValue(_sensorReadings->humSerraAverage); _humAvgChar->notify();
    _tempMinChar->setValue(_sensorReadings->tempMin); _tempMinChar->notify();
    _tempMaxChar->setValue(_sensorReadings->tempMax); _tempMaxChar->notify();
    _humMinChar->setValue(_sensorReadings->humMin); _humMinChar->notify(); // NOTIFY
    _humMaxChar->setValue(_sensorReadings->humMax); _humMaxChar->notify(); // NOTIFY
    _luxChar->setValue(_sensorReadings->luxValue); _luxChar->notify();

    _tempNordChar->setValue(_sensorReadings->tempOffice); _tempNordChar->notify();
    _humNordChar->setValue(_sensorReadings->humOffice); _humNordChar->notify();
    _tempCentroChar->setValue(_sensorReadings->tempSerraCentro); _tempCentroChar->notify();
    _humCentroChar->setValue(_sensorReadings->humSerraCentro); _humCentroChar->notify();
    _tempSudChar->setValue(_sensorReadings->tempSerraSud); _tempSudChar->notify();
    _humSudChar->setValue(_sensorReadings->humSerraSud); _humSudChar->notify();

    _batteryChar->setValue((int32_t)_sensorReadings->batteryPercent); _batteryChar->notify();

    char buf[12];
    snprintf(buf, sizeof(buf), "%02d:%02d", _sensorReadings->currentHour, _sensorReadings->currentMinute);
    _rtcTimeChar->setValue(buf); _rtcTimeChar->notify();

    snprintf(buf, sizeof(buf), "%02d/%02d/%04d", _sensorReadings->currentDay, _sensorReadings->currentMonth, _sensorReadings->currentYear);
    _rtcDateChar->setValue(buf); _rtcDateChar->notify();

    uint8_t status = 0;
    if (_sensorReadings->pirState)      status |= (1 << 0);
    if (_actuatorStates->isCallModeActive)  status |= (1 << 1);
    if (_sensorReadings->tankLow)      status |= (1 << 2);
    if (_sensorReadings->bmeNordOk)    status |= (1 << 3);
    if (_sensorReadings->bmeCentroOk)  status |= (1 << 4);
    if (_sensorReadings->bmeSudOk)    status |= (1 << 5);
    _systemStatusChar->setValue(&status, 1); _systemStatusChar->notify();
}

void BLEManager::ServerCallbacks::onConnect(NimBLEServer* pServer) { 
    _manager->_deviceConnected = true; 
}
void BLEManager::ServerCallbacks::onDisconnect(NimBLEServer* pServer) { 
    _manager->_deviceConnected = false; 
    NimBLEDevice::getAdvertising()->start(); 
}

void BLEManager::ActuatorCallbacks::onWrite(NimBLECharacteristic* p) {
    std::string val = p->getValue();
    if (val.length() == 0) return;
    bool state = (val[0] == 1);
    std::string uuid = p->getUUID().toString();

    if (uuid == CHAR_AUTO_UUID) _actuatorStates->isAutoModeActive = state;
    else if (uuid == CHAR_PUMP_UUID) _actuatorStates->isPumpOn = state;
    else if (uuid == CHAR_HEATER_UUID) _actuatorStates->isHeaterDeskOn = state;
    else if (uuid == CHAR_EXTRACTOR_UUID) _actuatorStates->isExtractorOn = state;
    else if (uuid == CHAR_CURTAIN_UUID) _actuatorStates->areCurtainsOpen = state;
    else if (uuid == CHAR_GROW_LIGHTS_UUID) _actuatorStates->isGrowLightOn = state;
}
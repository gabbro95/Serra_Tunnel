#include "SensorReader.h"

SensorReader::SensorReader(SensorReadings_t* sensors, ActuatorStates_t* actuators)
    : _sensorReadings(sensors), _actuatorStates(actuators) {}

// Funzione di utilità per il multiplexer TCA9548A
void SensorReader::tcaSelectChannel(uint8_t i) {
    if (i > 7) return; 
    Wire.beginTransmission(TCA9548A_ADDR);
    Wire.write(1 << i);
    Wire.endTransmission();  
}

void SensorReader::initSensors() {
    Wire.begin(PIN_SDA, PIN_SCL);
    
    // --- 1. Init RTC (Canale 3) ---
    tcaSelectChannel(RTC_CHANNEL);
    if (!_rtc.begin()) {
        Serial.println("ERRORE CRITICO: RTC non trovato.");
        while (1);    
    } 

    // --- CONTROLLO BATTERIA ---
    // Dopo aver rimosso R2, questa lettura sarà reale (con una batteria non ricaricabile)
    float voltaggio = (analogRead(PIN_BAT) * 3.3) / 4095.0;
    // Se il voltaggio è < 2.5V, la batteria è quasi scarica
    Serial.printf("Stato Batteria: %.2fV\n", voltaggio);

    // --- CONTROLLO AVVIO ---
    if (!_rtc.isrunning()) {
        Serial.println("[!] Orario non valido o RTC fermo. Provo il WiFi...");
        
        WiFi.begin(ssid, password);
        int tentativi = 0;
        while (WiFi.status() != WL_CONNECTED && tentativi < 20) {
            delay(500); Serial.print("."); tentativi++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            aggiornaRTCconWiFi();
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
        } else {
            Serial.println("\n[!] WiFi fallito. L'ora potrebbe essere errata!");
            // Opzione di emergenza: imposta ora compilazione
            _rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        }
    } else {
        Serial.println("[OK] Orario recuperato correttamente dalla batteria.");
    }

    // Verifica finale
    DateTime now = _rtc.now();
    Serial.printf("Ora attuale: %02d:%02d:%02d\n", now.hour(), now.minute(), now.second());
    
    // --- 2. Init BME280 (Canali 0, 1, 2) ---
    // Inizializzazione BME Ufficio (Canale 0)
    tcaSelectChannel(BME_CHANNEL_OFFICE); 
    if (!_bmeOffice.begin(BME_ADDRESS_COMMON)) { Serial.println("ERRORE: BME280 Office non trovato."); }
    // Inizializzazione BME Serra 1 (Canale 1)
    tcaSelectChannel(BME_CHANNEL_SERRA_1);
    if (!_bmeSerra1.begin(BME_ADDRESS_COMMON)) { Serial.println("ERRORE: BME280 Serra 1 non trovato."); }
    // Inizializzazione BME Serra 2 (Canale 2)
    tcaSelectChannel(BME_CHANNEL_SERRA_2);
    if (!_bmeSerra2.begin(BME_ADDRESS_COMMON)) { Serial.println("ERRORE: BME280 Serra 2 non trovato."); }
    
    // TORNA AL CANALE 0 O CHIUDI PER SICUREZZA
    tcaSelectChannel(BME_CHANNEL_OFFICE);
}

// Funzione calcolo media BME
float SensorReader::calculateAverageBME(float val1, float val2, float val3) {
    float average  = (val1 + val2 + val3) / 3.0F;
    return average;
}

// Funzione calcolo media sonde
float SensorReader::calculateAverageSoil(float val1, float val2) {
    float average  = (val1 + val2) / 2.0F;
    return average;
}

// Funzione di lettura dell'orologio
void SensorReader::readRTC() {
    // Lettura RTC (Canale 3)
    tcaSelectChannel(RTC_CHANNEL);
    DateTime now = _rtc.now();
    _sensorReadings->currentDay = now.day();
    _sensorReadings->currentHour = now.hour();
    _sensorReadings->currentMinute = now.minute();
    
    // Verifica se è scattata la mezzanotte per il reset della logica
    static int lastDay = -1;
    if (_sensorReadings->currentDay != lastDay && lastDay != -1) {
        _actuatorStates->isNewDay = true;
    } else {
        _actuatorStates->isNewDay = false;
    }
    lastDay = _sensorReadings->currentDay;
}

// Funzione di lettura dei sensori temperatura
void SensorReader::readBME() {
    // Lettura Sensore Ufficio (BME280)
    tcaSelectChannel(BME_CHANNEL_OFFICE); 
    float tempOffice = _bmeOffice.readTemperature();
    float humOffice = _bmeOffice.readHumidity();
    _sensorReadings->tempOffice = tempOffice; _sensorReadings->humOffice = humOffice;
   
    // Lettura Sensore Serra 1
    tcaSelectChannel(BME_CHANNEL_SERRA_1); 
    float temp1 = _bmeSerra1.readTemperature();
    float hum1 = _bmeSerra1.readHumidity();
    _sensorReadings->tempSerra1 = temp1; _sensorReadings->humSerra1 = hum1;

    // Lettura Sensore Serra 2
    tcaSelectChannel(BME_CHANNEL_SERRA_2); 
    float temp2 = _bmeSerra2.readTemperature();
    float hum2 = _bmeSerra2.readHumidity();
    _sensorReadings->tempSerra2 = temp2; _sensorReadings->humSerra2 = hum2;

    // Calcolo Media Serra
    _sensorReadings->tempSerraAverage = calculateAverageBME(tempOffice,temp1, temp2);
    _sensorReadings->humSerraAverage = calculateAverageBME(humOffice,hum1, hum2);
}

// Funzione di lettura dei sensori del terreno
void SensorReader::readSoilSensor() {
    // Lettura Sensori Analogici (Terreno)
    _sensorReadings->soil1 = (float)analogRead(PIN_SOIL_1);
    _sensorReadings->soil2 = (float)analogRead(PIN_SOIL_2);
    // Esempio semplice: 4095 = secco, 0 = bagnato (invertire e normalizzare)
    _sensorReadings->soilAverage = calculateAverageSoil(_sensorReadings->soil1, _sensorReadings->soil2);
}

// Funzione di lettura della luce solare
void SensorReader::readLDR() {
    // Lettura Sensori Analogici (LDR)
    _sensorReadings->luxValue = (float)analogRead(PIN_LDR); // LDR
}

// Funzioni di lettura dei sensori (Digitali)
void SensorReader::readDigitalSensors() {
    // Rilevamento Pioggia/Livello 
    _sensorReadings->isRaining = digitalRead(PIN_RAIN_SENSOR);
    // Assumiamo che il sensore di livello tank sia LOW se vuoto
    _sensorReadings->tankLow = !digitalRead(PIN_RAIN_SENSOR); 
}

void SensorReader::readOfficeSensors() {
    // Rilevamento Presenza
    _sensorReadings->pirState = digitalRead(PIN_PIR_OFFICE);
}

// Funzione di lettura dei sensori
void SensorReader::readAllSensors() {
    readRTC();
    readBME();
    readSoilSensor();
    readLDR();
    readDigitalSensors();
    readOfficeSensors();
}

void SensorReader::aggiornaRTCconWiFi() {
    configTime(3600, 3600, "pool.ntp.org"); // GMT+1 e Ora Legale per Italia
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        _rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, 
                            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
        Serial.println("\n[OK] RTC sincronizzato con internet.");
    }
}
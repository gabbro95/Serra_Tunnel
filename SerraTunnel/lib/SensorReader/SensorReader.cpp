#include "SensorReader.h"
#include <Wire.h>
#include <RTClib.h> // Libreria necessaria per DS3231/RTC

// Istanza RTC
RTC_DS3231 rtc;

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
    if (!rtc.begin()) {
        Serial.println("ERRORE CRITICO: RTC non trovato.");
        while (1);    } 

    // --- CONNESSIONE WIFI ---
    Serial.print("Connessione al WiFi");
    WiFi.begin(ssid, password);

    // Tentiamo la connessione per un massimo di 10 secondi (per non bloccare tutto se manca internet)
    int tentativi = 0;
    while (WiFi.status() != WL_CONNECTED && tentativi < 20) {
        delay(500);
        Serial.print(".");
        tentativi++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi Connesso!");
        
        // --- SINCRONIZZAZIONE NTP ---
        // Configura l'orario interno dell'ESP32
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        
        // Aggiorna l'RTC esterno con l'ora appena presa
        aggiornaRTCconWiFi();
        
        // Disconnetti WiFi
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
    } else {
        Serial.println("\nImpossibile connettersi al WiFi. L'RTC userà l'orario che ha in memoria.");
    }

    // Verifica finale
    DateTime now = rtc.now();
    Serial.printf("Orario attuale RTC: %02d:%02d:%02d - %02d/%02d/%04d\n", 
        now.hour(), now.minute(), now.second(), now.day(), now.month(), now.year());
    
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

// Funzione calcolo media
float SensorReader::calculateAverage(float val1, float val2, float val3) {
    float average  = (val1 + val2 + val3) / 3.0F;
    return average;
}

// Funzione di lettura dell'orologio
void SensorReader::readRTC() {
    // Lettura RTC (Canale 3)
    tcaSelectChannel(RTC_CHANNEL);
    DateTime now = rtc.now();
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
    _sensorReadings->tempSerraAverage = calculateAverage(tempOffice,temp1, temp2);
    _sensorReadings->humSerraAverage = calculateAverage(humOffice,hum1, hum2);
}

// Funzione di lettura dei sensori del terreno
void SensorReader::readSoilSensor() {
    // Lettura Sensori Analogici (Terreno)
    _sensorReadings->soil1 = (float)analogRead(PIN_SOIL_1);
    _sensorReadings->soil2 = (float)analogRead(PIN_SOIL_2);
    _sensorReadings->soil3 = (float)analogRead(PIN_SOIL_3);
    // Esempio semplice: 4095 = secco, 0 = bagnato (invertire e normalizzare)
    _sensorReadings->soilAverage = calculateAverage(_sensorReadings->soil1, _sensorReadings->soil2, _sensorReadings->soil3);
}

// Funzione di lettura della luce solare
void SensorReader::readLDR() {
    // Lettura Sensori Analogici (LDR)
    _sensorReadings->luxValue = (float)analogRead(PIN_LDR); // LDR
}

// Funzione di lettura dei sensori (Digitali)
void SensorReader::readDigitalSensors() {
    // Rilevamento Presenza e Pioggia/Livello 
    _sensorReadings->pirState = digitalRead(PIN_PIR_OFFICE);
    _sensorReadings->isRaining = digitalRead(PIN_RAIN_SENSOR);
    // Assumiamo che il sensore di livello tank sia LOW se vuoto
    _sensorReadings->tankLow = !digitalRead(PIN_RAIN_SENSOR); 
}

// Funzione di lettura dei sensori
void SensorReader::readAllSensors() {
    readRTC();
    readBME();
    readSoilSensor();
    readLDR();
    readDigitalSensors();
}

void aggiornaRTCconWiFi() {
    struct tm timeinfo;
    
    // Tenta di ottenere l'ora locale dal sistema (che è stato sincronizzato via NTP)
    if(!getLocalTime(&timeinfo)){
        Serial.println("Errore: Impossibile ottenere l'ora dal server NTP.");
        return;
    }

    // Se siamo qui, abbiamo l'ora esatta!
    Serial.println("Orario NTP ottenuto. Aggiorno il modulo RTC...");

    // Scriviamo l'ora nell'RTC fisico
    // timeinfo.tm_year conta dal 1900, quindi aggiungiamo 1900
    // timeinfo.tm_mon va da 0 a 11, quindi aggiungiamo 1
    rtc.adjust(DateTime(
        timeinfo.tm_year + 1900, 
        timeinfo.tm_mon + 1, 
        timeinfo.tm_mday, 
        timeinfo.tm_hour, 
        timeinfo.tm_min, 
        timeinfo.tm_sec
    ));
    
    Serial.println("RTC aggiornato con successo via Wi-Fi!");
}
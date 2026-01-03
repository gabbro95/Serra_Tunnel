#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <Arduino.h>

// INPUT SENSORI
struct SensorReadings_t {
    // BME
    float tempOffice;       
    float humOffice;
    float tempSerraCentro;  
    float humSerraCentro;
    float tempSerraSud;     
    float humSerraSud;

    bool bmeNordOk;
    bool bmeCentroOk;
    bool bmeSudOk;

    float tempSerraAverage;
    float humSerraAverage;
    float tempMin;
    float tempMax;
    float humMin;
    float humMax;          

    // Ambiente
    float luxValue;  
    bool isNaturalLightSufficient;     
    bool pirState;          
    bool tankLow;           

    // Orologio RTC
    int currentHour;        // Ora attuale (0-23)
    int currentMinute;      // Minuti attuali (0-59)
    int currentDay;         // Giorno attuale (1-31)
    int currentMonth;
    int currentYear;
    int batteryPercent;  

};

enum FanMode_t {
    FAN_OFF = 0,
    FAN_SILENT = 1,      
    FAN_RECIRCULATION = 2, 
    FAN_TURBO = 3        
};

// OUTPUT ATTUATORI
struct ActuatorStates_t {
    // Stato Sistema
    bool isAutoModeActive;      
    bool isCallModeActive;      
    bool areCurtainsOpen;       

    // Relè attivazione
    bool isPumpOn;              
    bool isGrowLightOn;           
    bool isHeaterDeskOn;             
    bool isExtractorOn;           

    // Relè stato
    bool isPump;              
    bool isGrowLight;       
    bool isHeaterDesk;           
    bool isExtractor;           

    // PWM
    FanMode_t fanMode;          
    int fanSpeedPWM;            
    int deskLightPWM;           

    // Allarmi
    bool warningTankLow;        
    bool warningOverheat;       
    
    // Info Fotoperiodo
    int hoursOfLightAccumulated; // Ore totali di luce (Naturale + LED)
    bool isNewDay;                 // Indica se è appena scattata la mezzanotte

};

#endif
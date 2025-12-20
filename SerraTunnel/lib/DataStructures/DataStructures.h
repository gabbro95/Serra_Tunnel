#ifndef DATA_STRUCTURES_H
#define DATA_STRUCTURES_H

#include <Arduino.h>

enum FanMode_t {
    FAN_OFF = 0,
    FAN_SILENT = 1,      
    FAN_RECIRCULATION = 2, 
    FAN_TURBO = 3        
};

// INPUT SENSORI
typedef struct {
    // Ufficio
    float tempOffice;       
    float humOffice;        
    
    // Serra (Media)
    float tempSerraAverage;     
    float humSerraAverage;  

    // Dati Grezzi Serra (opzionali per debug)
    float tempSerra1; float tempSerra2;
    float humSerra1; float humSerra2;

    // Terreno
    float soilAverage;      
    float soil1; float soil2;           

    // Ambiente
    float luxValue;  
    bool isNaturalLightSufficient;     
    bool pirState;          
    bool isRaining;         
    bool tankLow;           

    // Orologio RTC
    int currentHour;        // Ora attuale (0-23)
    int currentMinute;      // Minuti attuali (0-59)
    int currentDay;         // Giorno attuale (1-31)

} SensorReadings_t;

// OUTPUT ATTUATORI
typedef struct {
    // Stato Sistema
    bool isAutoModeActive;      
    bool isCallModeActive;      
    bool areCurtainsOpen;       

    // Relè attivazione
    bool isPumpOn;              
    bool isGrowLightOn;         
    bool isHeaterOn;           
    bool isHeaterDeskOn;             
    bool isExtractorOn;           

    // Relè stato
    bool isPump;              
    bool isGrowLight;         
    bool isHeater;     
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

} ActuatorStates_t;

#endif
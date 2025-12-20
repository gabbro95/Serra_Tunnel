#ifndef LOGIC_CONTROLLER_H
#define LOGIC_CONTROLLER_H

#include <Arduino.h>
#include "../Config/Config.h"
#include "../DataStructures/DataStructures.h"
#include "../SensorReader/SensorReader.h"
#include "../Timer/Timer.h"

class LogicController {
public:
    LogicController(SensorReadings_t* sensors, ActuatorStates_t* actuators);
    void initLogic();
    void runLogicCycle();

private:
    SensorReadings_t* _sensorReadings;
    ActuatorStates_t* _actuatorStates;

    Timer* switchOffHeater;
    Timer* switchOnHeater;
    Timer* switchOffPump;

    // Funzioni di utilità per gli attuatori
    void setRelayState(int pin, bool state);
    void setPWMState(int channel, int speed);
    void updatePwm();
    void applyActuatorStates();

    // Logiche Principali (le funzioni che hai menzionato)
    void checkLighting();         // Gestisce il fotoperiodo basato su LDR e RTC
    void checkIrrigation();       // Gestisce la pompa e l'umidità del terreno
    void checkClimate();          // Gestisce riscaldamento, l'Estrattore e ventilazione
    void checkCallMode();         // Logica per silenziare/accendere luci in Call Mode
    void checkCurtainsInterlock(); // Logica di sicurezza tende chiuse
};

#endif
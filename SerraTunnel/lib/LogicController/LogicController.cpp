#include "LogicController.h"

// Ultima ora in cui è stata accesa la luce (per il debug)
// unsigned long _lastLightOnTime = 0; 

LogicController::LogicController(SensorReadings_t* sensors, ActuatorStates_t* actuators)
    : _sensorReadings(sensors), _actuatorStates(actuators) {}

void LogicController::setRelayState(int pin, bool state) {
    digitalWrite(pin, state ? LOW : HIGH);
}
void LogicController::setPWMState(int channel, int speed) {
    ledcWrite(channel, speed);
}

void LogicController::initLogic() {
    ledcSetup(LEDC_CHANNEL_DESK_LIGHT, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcAttachPin(PIN_DESK_LIGHT_PWM, LEDC_CHANNEL_DESK_LIGHT);
    ledcSetup(LEDC_CHANNEL_FAN, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcAttachPin(PIN_RELAY_FAN, LEDC_CHANNEL_FAN); 
    
    // Setup Relè 
    pinMode(PIN_RELAY_EXTRACTOR, OUTPUT);
    pinMode(PIN_RELAY_HEATER, OUTPUT);
    pinMode(PIN_RELAY_PUMP, OUTPUT);
    pinMode(PIN_RELAY_GROW_LIGHT, OUTPUT);

    _actuatorStates->isExtractor = false;  // Inizializza OFF (Active LOW = true)
    _actuatorStates->isExtractorOn = !_actuatorStates->isExtractor;  
    _actuatorStates->isHeater = false;
    _actuatorStates->isHeaterOn = !_actuatorStates->isHeater;
    _actuatorStates->isPump = false;
    _actuatorStates->isPumpOn = !_actuatorStates->isPump;
    _actuatorStates->isGrowLight = false;
    _actuatorStates->isGrowLightOn = !_actuatorStates->isGrowLight;

    _actuatorStates->isAutoModeActive = true; 
    _actuatorStates->areCurtainsOpen = true; 
    _actuatorStates->fanMode = FAN_RECIRCULATION; 
    _actuatorStates->hoursOfLightAccumulated = 0.0;
    
    applyActuatorStates();
}

// --- LOGICA 1: GESTIONE LUCE (FOTOPERIODO 16/18H con RTC) ---
void LogicController::checkLighting() {
    
    // 1. Reset Giornaliero Fotoperiodo
    if (_actuatorStates->isNewDay) {
        // Reset del contatore di ore di luce accumulate (naturale + artificiale)
        _actuatorStates->hoursOfLightAccumulated = 0.0;
        Serial.println("RTC: Mezzanotte scattata, reset fotoperiodo.");
    }
    
    // 2. Accumulo Ore
    // Se c'è luce naturale o artificiale, incrementa il contatore.
    bool isLightActive = (_sensorReadings->luxValue > LUX_THRESHOLD_DAY) || _actuatorStates->isGrowLightOn;
    
    if (isLightActive && (controlHourTime.get_state() == TIMER_EXPIRED)) { // Ogni minuto
        _actuatorStates->hoursOfLightAccumulated++;
        controlHourTime.start();
        // Serial.printf("Ore Luce Accumulate: %.2f\n", _actuatorStates->hoursOfLightAccumulated);
    }

    // 3. Decisione Accensione/Spegnimento (Logica del Fotoperiodo)
    _sensorReadings->isNaturalLightSufficient = _sensorReadings->luxValue > LUX_THRESHOLD_DAY;
    
    if (_actuatorStates->isAutoModeActive) {
        // Accendi se:
        // - NON c'è sufficiente luce naturale ORA (giorno nuvoloso o tramonto)
        // - E non abbiamo ancora raggiunto l'obiettivo di 16 ore.
        if (!_sensorReadings->isNaturalLightSufficient && _actuatorStates->hoursOfLightAccumulated < LIGHT_TARGET_HOURS) {
            _actuatorStates->isGrowLightOn = true;
        } 
        // Spegni se:
        // - ABBIAMO già raggiunto l'obiettivo di 16 ore
        // - O c'è abbondante luce naturale (che prenderà il sopravvento)
        else if (_actuatorStates->hoursOfLightAccumulated >= LIGHT_TARGET_HOURS || _sensorReadings->isNaturalLightSufficient) {
            _actuatorStates->isGrowLightOn = false;
        }
    }
    // Nota: I controlli manuali disattivano la logica Auto, ma il contatore continua ad aggiornarsi.
}

// --- LOGICA 2: IRRIGAZIONE (Separata) ---
void LogicController::checkIrrigation() {
    // Esempio semplificato: normalizza l'ADC (0-4095) in percentuale approssimativa (0-100)
    float soilPercent = 100.0F - (_sensorReadings->soilAverage / 4095.0F) * 100.0F;

    // Condizioni per irrigare
    bool needsWater = soilPercent < SOIL_MOISTURE_MIN_PERCENT;
    bool safetyCheck = !_sensorReadings->tankLow;
    // NON irriga se in Call Mode
    bool isQuiet = !_actuatorStates->isCallModeActive;
    
    if (_actuatorStates->isAutoModeActive && needsWater && safetyCheck && isQuiet) {
        _actuatorStates->isPumpOn = true;
        // In un progetto reale: qui si attiverebbe un timer per 15s (WATER_PUMP_DURATION_SEC)
        // Per semplicità di codice, l'uscita resta ON finché non si spegne manualmente 
        // o finché il sensore terreno risale (ma questo non è l'ideale per le pompe).
    } else {
        // Se non necessario, assicurati che sia spenta (se non è in fase di irrigazione temporizzata)
        _actuatorStates->isPumpOn = false; 
    }
}


// --- LOGICA 3: CLIMA (RISCALDAMENTO E VENTILAZIONE) ---
void LogicController::checkClimate() {
    float tGlobale = _sensorReadings->tempSerraAverage; // Usiamo tGlobale per tutte le decisioni climatiche
    
    // 1. ESTRATTORE FORZATO (Priorità 1: Espulsione calore critico)
    // Accendi Estrattore se la temperatura globale supera la soglia
    if (tGlobale > TEMP_THRESHOLD_EXTRACT) {
        _actuatorStates->isExtractorOn = true;
        _actuatorStates->isHeaterOn = false; // Interlock con Riscaldamento
        
    } 
    // Spegni Estrattore se la temperatura scende sotto la soglia con Isteresi
    else if (_actuatorStates->isExtractorOn && tGlobale < (TEMP_THRESHOLD_EXTRACT - TEMP_HYSTERESIS)) {
        _actuatorStates->isExtractorOn = false;
    }
    
    // Emergenza Calore (se Estrattore è guasto o insufficiente)
    if (tGlobale > TEMP_CRITIC_MAX) {
        _actuatorStates->fanMode = FAN_TURBO;
        _actuatorStates->fanSpeedPWM = FAN_SPEED_TURBO_PWM;
        
    }

    // Se l'estrattore è acceso, blocca qualsiasi altra logica di clima.
    if (_actuatorStates->isExtractorOn) {
        _actuatorStates->isHeaterOn = false; // Interlock con Riscaldamento
        return;
    }


    // 2. RISCALDAMENTO (Priorità 2: Controllo Freddo)
    if (_actuatorStates->isCallModeActive) {
        if (tGlobale < TEMP_OPTIMAL_MIN - TEMP_HYSTERESIS) {
            _actuatorStates->isHeaterDeskOn = true;
        } else if (tGlobale > TEMP_OPTIMAL_MAX - TEMP_HYSTERESIS) { // Isteresi di 1°C per spegnere il riscaldamento
            _actuatorStates->isHeaterDeskOn = false; 
        }
    }
    if (tGlobale < TEMP_HEATING_MIN) {
        _actuatorStates->isHeaterOn = true;
        _actuatorStates->fanMode = FAN_OFF;
        _actuatorStates->fanSpeedPWM = 0;
        return; 
    } else if (tGlobale > TEMP_OPTIMAL_MIN + TEMP_HYSTERESIS) { // Isteresi di 1°C per spegnere il riscaldamento
        _actuatorStates->isHeaterOn = false; 
    }
 
    // 3. VENTILAZIONE INTERNA (Ricircolo - Priorità 3: Ottimizzazione e Turbo)

    // Ricircolo Ottimale
    bool isOptimal = (tGlobale >= TEMP_OPTIMAL_MIN && tGlobale <= TEMP_OPTIMAL_MAX);
    
    if (isOptimal && !_actuatorStates->isCallModeActive) {
        _actuatorStates->fanMode = FAN_RECIRCULATION;
        _actuatorStates->fanSpeedPWM = FAN_SPEED_RECIRCULATION_PWM;
    } else if (!_actuatorStates->isCallModeActive && _actuatorStates->fanMode != FAN_TURBO) {
        // Se non ottimale e non in turbo/call mode, spegni il ricircolo
        _actuatorStates->fanMode = FAN_OFF;
        _actuatorStates->fanSpeedPWM = 0;
    }
}

// --- LOGICA 4: GESTIONE "CALL MODE" (Ufficio) ---
void LogicController::checkCallMode() {
    // Se c'è qualcuno, disabilita il rumore e accendi la luce ufficio
    if (_actuatorStates->isCallModeActive) {
        _actuatorStates->fanMode = FAN_SILENT;
        _actuatorStates->fanSpeedPWM = FAN_SPEED_SILENT_PWM;
        _actuatorStates->isPumpOn = false; // Pausa irrigazione
        if (_sensorReadings->pirState) _actuatorStates->deskLightPWM = DESK_LIGHT_BRIGHTNESS_ON;
    } else {
        // Quando non c'è nessuno e la call è off
        if (!_sensorReadings->pirState  && !_actuatorStates->isCallModeActive) {
            _actuatorStates->deskLightPWM = 0; // Spegni luce ufficio
        }
    }
}

// --- LOGICA 5: BLOCCO TENDE CHIUSE (Vince su tutto) ---
void LogicController::checkCurtainsInterlock() {
    if (_actuatorStates->areCurtainsOpen == false) {
        // Se tende chiuse, blocca movimenti e riscaldamento (isolamento)
        _actuatorStates->fanMode = FAN_OFF;
        _actuatorStates->fanSpeedPWM = 0;
        _actuatorStates->isHeaterOn = false;
    }
}

void LogicController::applyActuatorStates() {
    // Applica stati
    if (_actuatorStates->isPumpOn != _actuatorStates->isPump) {
        _actuatorStates->isPump = _actuatorStates->isPumpOn;
        setRelayState(PIN_RELAY_PUMP, _actuatorStates->isPumpOn);
    }
    if (_actuatorStates->isGrowLightOn != _actuatorStates->isGrowLight) {
        _actuatorStates->isGrowLight = _actuatorStates->isGrowLight;
        setRelayState(PIN_RELAY_GROW_LIGHT, _actuatorStates->isGrowLightOn);
    }
    if (_actuatorStates->isHeaterOn != _actuatorStates->isHeater) {
        _actuatorStates->isHeater = _actuatorStates->isHeaterOn;
        setRelayState(PIN_RELAY_HEATER, _actuatorStates->isHeaterOn);
    }
    if (_actuatorStates->isExtractorOn != _actuatorStates->isExtractor) {
        _actuatorStates->isExtractor = _actuatorStates->isExtractorOn;
        setRelayState(PIN_RELAY_EXTRACTOR, _actuatorStates->isExtractorOn); 
    }

    setPWMState(LEDC_CHANNEL_FAN, _actuatorStates->fanSpeedPWM); 
    setPWMState(LEDC_CHANNEL_DESK_LIGHT, _actuatorStates->deskLightPWM);
}

void LogicController::runLogicCycle() {

    checkCallMode(); // Deve essere eseguito prima, imposta il flag isCallModeActive

    // 1. Logiche di Sicurezza Base
    if (_sensorReadings->tankLow) _actuatorStates->isPumpOn = false;

    if (_actuatorStates->isAutoModeActive) {
        // 2. Sequenza Automatica
        controlHourTime.update();
        checkLighting();   
        checkIrrigation();
        checkClimate();   
    } 
    
    // 3. Blocco Finale Sicurezza Tende (Vince su tutto)
    checkCurtainsInterlock();

    applyActuatorStates();
}

void LogicController::setControlHourTimer(unsigned long duration_ms_) {
    controlHourTime.set_duration(duration_ms_);
}

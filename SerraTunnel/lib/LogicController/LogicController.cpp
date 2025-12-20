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

void LogicController::updatePwm() {
    // Gestione della velocità in base alla modalità (FanMode_t)
    switch (_actuatorStates->fanMode)
    {
        case FAN_OFF:
            _actuatorStates->fanSpeedPWM = FAN_SPEED_OFF_PWM;
            break;
        
        case FAN_SILENT:
            _actuatorStates->fanSpeedPWM = FAN_SPEED_SILENT_PWM; // Velocità ridotta (es. per la notte)
            break;

        case FAN_RECIRCULATION:
            _actuatorStates->fanSpeedPWM = FAN_SPEED_RECIRCULATION_PWM; // Velocità media per umidità
            break;

        case FAN_TURBO:
            _actuatorStates->fanSpeedPWM = FAN_SPEED_TURBO_PWM;
            break;
    }
}

void LogicController::initLogic() {
    ledcSetup(LEDC_CHANNEL_DESK_LIGHT, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcAttachPin(PIN_DESK_LIGHT_PWM, LEDC_CHANNEL_DESK_LIGHT);
    ledcSetup(LEDC_CHANNEL_FAN, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcAttachPin(PIN_RELAY_FAN, LEDC_CHANNEL_FAN); 
    
    // Setup Relè 
    pinMode(PIN_RELAY_EXTRACTOR, OUTPUT);
    //pinMode(PIN_RELAY_HEATER, OUTPUT);
    pinMode(PIN_RELAY_HEATER_DESK, OUTPUT);
    pinMode(PIN_RELAY_PUMP, OUTPUT);
    pinMode(PIN_RELAY_GROW_LIGHT, OUTPUT);

    _actuatorStates->isExtractor = false;  // Inizializza OFF (Active LOW = true)
    _actuatorStates->isExtractorOn = _actuatorStates->isExtractor;  
    //_actuatorStates->isHeater = false;
    //_actuatorStates->isHeaterOn = _actuatorStates->isHeater;
    _actuatorStates->isHeaterDesk = false;
    _actuatorStates->isHeaterOn = _actuatorStates->isHeater;
    _actuatorStates->isPump = false;
    _actuatorStates->isPumpOn = _actuatorStates->isPump;
    _actuatorStates->isGrowLight = false;
    _actuatorStates->isGrowLightOn = _actuatorStates->isGrowLight;

    _actuatorStates->isAutoModeActive = true; 
    _actuatorStates->areCurtainsOpen = false; 
    _actuatorStates->fanMode = FAN_RECIRCULATION; 
    _actuatorStates->hoursOfLightAccumulated = 0.0;
    
    applyActuatorStates();

    switchOffHeater->set_duration(HEATER_DURATION_ON);
    switchOnHeater->set_duration(HEATER_DURATION_OFF);
    switchOffPump->set_duration(WATER_PUMP_DURATION);
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
    static int controlHourTime = -1;

    if (isLightActive && controlHourTime != _sensorReadings->currentHour && controlHourTime != -1) { // Ogni minuto
        _actuatorStates->hoursOfLightAccumulated++;
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
    // Aggiornamento Timer
    switchOffPump->update();

    // Controllo Timer
    if (switchOffPump->get_state() == TIMER_EXPIRED) {
        if (_actuatorStates->isPumpOn) {
            _actuatorStates->isPumpOn = false; 
            switchOffPump->start();
        } else switchOffPump->stop();
    }

    // Esempio semplificato: normalizza l'ADC (0-4095) in percentuale approssimativa (0-100)
    float soilPercent = 100.0F - (_sensorReadings->soilAverage / 4095.0F) * 100.0F;

    // Condizioni per irrigare
    bool needsWater = soilPercent < SOIL_MOISTURE_MIN_PERCENT;
    bool safetyCheck = !_sensorReadings->tankLow;
    // NON irriga se in Call Mode
    bool isQuiet = !_actuatorStates->isCallModeActive;
    
    if (needsWater && safetyCheck && isQuiet && switchOffPump->get_state() == TIMER_STOPPED) {
        _actuatorStates->isPumpOn = true;
        switchOffPump->start();
    } 
}


// --- LOGICA 3: CLIMA (RISCALDAMENTO E VENTILAZIONE) ---
void LogicController::checkClimate() {
    // Aggiornamento Timer
    switchOffHeater->update();
    switchOnHeater->update();

    // Controllo Timer
    if (switchOffHeater->get_state() == TIMER_EXPIRED)  {
        _actuatorStates->isHeaterDeskOn = false;
        switchOffHeater->stop();
        switchOnHeater->start();
    } else if (switchOnHeater->get_state() == TIMER_EXPIRED) {
        switchOnHeater->stop();
    }

    float tGlobale = _sensorReadings->tempSerraAverage;
    float hGlobale = _sensorReadings->humSerraAverage; 

    // -------------------------------------------------------------------------
    // VENTILAZIONE INTERNA (Ricircolo - Priorità 3: Ottimizzazione Clima e Antimuffa)
    // -------------------------------------------------------------------------

    // È il momento ideale per la temperatura?
    bool isTempOptimal = (tGlobale >= TEMP_OPTIMAL_MIN && tGlobale <= TEMP_OPTIMAL_MAX);
    // L'umidità sta salendo troppo? (Prevenzione condensa sulle foglie)
    bool isHumHighRecirc = (hGlobale > HUM_THRESHOLD_RECIRCULATION);

    // Attiviamo il ricircolo se la temperatura è ottimale OPPURE se l'umidità è alta
    if (
        (isTempOptimal || isHumHighRecirc) 
        && _actuatorStates->fanMode != FAN_TURBO
    ) _actuatorStates->fanMode = FAN_RECIRCULATION;
    else if (_actuatorStates->fanMode != FAN_TURBO) {
        // Se non siamo in turbo, non è ottimale e non c'è umidità, spegni tutto.
        _actuatorStates->fanMode = FAN_OFF;
    }

    // -------------------------------------------------------------------------
    // ESTRATTORE FORZATO (Priorità 1: Espulsione calore O Umidità eccessiva)
    // -------------------------------------------------------------------------

    // A. Condizione Temperatura: Fa troppo caldo?
    bool isTooHot = (tGlobale > TEMP_THRESHOLD_EXTRACT);

    // B. Condizione Umidità: C'è troppa umidità, ma la temperatura è sicura?
    // (Non buttiamo fuori umidità se rischiamo di congelare le piante)
    bool isTooHumid = (hGlobale > HUM_THRESHOLD_EXTRACT && tGlobale > TEMP_MIN_FOR_HUMIDITY_VENT);

    // ACCENSIONE
    if (isTooHot || isTooHumid) {
        _actuatorStates->isExtractorOn = true;
        _actuatorStates->isHeaterDeskOn = false; // Interlock con Riscaldamento
        switchOffHeater->stop();
    } 
    // SPEGNIMENTO (con Isteresi)
    else if (_actuatorStates->isExtractorOn) {
        // Verifichiamo se le condizioni sono rientrate
        bool tempOk = tGlobale < (TEMP_THRESHOLD_EXTRACT - TEMP_HYSTERESIS);
        bool humOk = hGlobale < (HUM_THRESHOLD_EXTRACT - HUM_HYSTERESIS);

        // Spegniamo solo se SIA la temperatura CHE l'umidità sono sotto controllo
        if (tempOk && humOk) {
            _actuatorStates->isExtractorOn = false;
        }
    }
    
    // Emergenza Calore Estremo (Override assoluto)
    if (tGlobale > TEMP_CRITIC_MAX) {
        _actuatorStates->fanMode = FAN_TURBO;
        _actuatorStates->fanSpeedPWM = FAN_SPEED_TURBO_PWM;
    }

    // Se l'estrattore è acceso, usciamo (ha priorità su riscaldamento e ricircolo)
    if (_actuatorStates->isExtractorOn) {
        _actuatorStates->isHeaterOn = false; 
        switchOffHeater->stop();
        return;
    }

    // -------------------------------------------------------------------------
    // RISCALDAMENTO (Priorità 2: Controllo Freddo)
    // -------------------------------------------------------------------------
    // Nota: Il riscaldamento abbassa naturalmente l'umidità relativa, quindi aiuta anche per quello.
    
    if (_actuatorStates->isCallModeActive) {
        if (tGlobale < TEMP_OPTIMAL_MIN - TEMP_HYSTERESIS && (switchOffHeater->get_state() == TIMER_STOPPED && switchOnHeater->get_state() == TIMER_STOPPED)) {
            _actuatorStates->isHeaterDeskOn = true;
            switchOffHeater->start();
        } else if (tGlobale > TEMP_OPTIMAL_MIN + TEMP_HYSTERESIS && switchOffHeater->get_state() == TIMER_RUNNING) {
            _actuatorStates->isHeaterDeskOn = false; 
            switchOffHeater->stop();
        }
    }
    
    // Logica antigelo standard
    if (tGlobale < TEMP_HEATING_MIN && (switchOffHeater->get_state() == TIMER_STOPPED && switchOnHeater->get_state() == TIMER_STOPPED)) {
        _actuatorStates->isHeaterDeskOn = true;
        switchOffHeater->start();
        _actuatorStates->fanMode = FAN_OFF; // Ferma ventole per non raffreddare l'aria calda appena prodotta
        _actuatorStates->fanSpeedPWM = 0;
        return; 
    } else if (tGlobale > TEMP_OPTIMAL_MIN + TEMP_HYSTERESIS && switchOffHeater->get_state() == TIMER_RUNNING) {
        _actuatorStates->isHeaterDeskOn = false; 
        switchOffHeater->stop();
    }
}

// --- LOGICA 4: GESTIONE "CALL MODE" (Ufficio) ---
void LogicController::checkCallMode() {
    // Se c'è qualcuno, disabilita il rumore e accendi la luce ufficio
    if (_actuatorStates->isCallModeActive) {
        _actuatorStates->fanMode = FAN_SILENT;
        _actuatorStates->isPumpOn = false; // Pausa irrigazione
        if (_sensorReadings->pirState) _actuatorStates->deskLightPWM = DESK_LIGHT_BRIGHTNESS_ON;
    } else {
        // Quando non c'è nessuno e la call è off
        if (!_sensorReadings->pirState  && !_actuatorStates->isCallModeActive) {
            _actuatorStates->deskLightPWM = DESK_LIGHT_BRIGHTNESS_OFF; // Spegni luce ufficio
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
        _actuatorStates->isHeaterDeskOn = false;
        _actuatorStates->isExtractorOn = false;
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
    if (_actuatorStates->isHeaterDeskOn != _actuatorStates->isHeaterDesk) {
        _actuatorStates->isHeaterDesk = _actuatorStates->isHeaterDeskOn;
        setRelayState(PIN_RELAY_HEATER_DESK, _actuatorStates->isHeaterDeskOn);
    }
    if (_actuatorStates->isExtractorOn != _actuatorStates->isExtractor) {
        _actuatorStates->isExtractor = _actuatorStates->isExtractorOn;
        setRelayState(PIN_RELAY_EXTRACTOR, _actuatorStates->isExtractorOn); 
    }

    updatePwm();
    setPWMState(LEDC_CHANNEL_FAN, _actuatorStates->fanSpeedPWM); 
    setPWMState(LEDC_CHANNEL_DESK_LIGHT, _actuatorStates->deskLightPWM);
}

void LogicController::runLogicCycle() {
    // 1. Logiche di Sicurezza Base
    if (_sensorReadings->tankLow) _actuatorStates->isPumpOn = false;

    if (_actuatorStates->isAutoModeActive) {
        // 2. Sequenza Automatica
        checkLighting();   
        checkIrrigation();
        checkClimate();   
        checkCallMode(); 

    } 
    
    // 3. Blocco Finale Sicurezza Tende (Vince su tutto)
    checkCurtainsInterlock();

    applyActuatorStates();
}

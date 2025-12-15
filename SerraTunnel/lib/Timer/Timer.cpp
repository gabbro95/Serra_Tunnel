#include "Timer.h"

// --- Implementazione della Classe ESP32Timer ---

Timer::Timer(unsigned long duration) 
    : start_time_ms_(0), 
      duration_ms_(duration), 
      current_state_(TIMER_STOPPED),
      auto_reset_enabled_(false) {}

void Timer::start() {
    if (current_state_ != TIMER_RUNNING) {
        start_time_ms_ = millis();
        current_state_ = TIMER_RUNNING;
    }
}

void Timer::stop() {
    current_state_ = TIMER_STOPPED;
    // Non resettiamo start_time_ms_ qui per permettere il calcolo del tempo trascorso.
}

void Timer::reset() {
    start_time_ms_ = 0;
    current_state_ = TIMER_STOPPED;
}

void Timer::set_duration(unsigned long duration) {
    duration_ms_ = duration;
}

void Timer::set_auto_reset(bool enable) {
    auto_reset_enabled_ = enable;
}

unsigned long Timer::get_elapsed_time() const {
    if (current_state_ == TIMER_STOPPED || current_state_ == TIMER_EXPIRED) {
        // Se è fermo o scaduto, il tempo trascorso è uguale alla durata impostata.
        return duration_ms_; 
    }
    // Altrimenti, calcola la differenza
    return millis() - start_time_ms_;
}

unsigned long Timer::get_remaining_time() const {
    if (current_state_ == TIMER_STOPPED || current_state_ == TIMER_EXPIRED) {
        return 0; // Se fermo o scaduto, non c'è tempo rimanente
    }

    unsigned long elapsed = millis() - start_time_ms_;

    if (elapsed >= duration_ms_) {
        // Teoricamente non dovrebbe succedere se update() è chiamato regolarmente,
        // ma è un buon controllo
        return 0; 
    }

    return duration_ms_ - elapsed;
}

bool Timer::update() {
    if (current_state_ == TIMER_RUNNING) {
        // Calcola il tempo trascorso
        unsigned long elapsed = millis() - start_time_ms_;

        if (elapsed >= duration_ms_) {
            // Il timer è scaduto
            current_state_ = TIMER_EXPIRED;

            if (auto_reset_enabled_) {
                // Se il reset automatico è attivo, riavvia subito il timer
                start_time_ms_ = millis(); 
                current_state_ = TIMER_RUNNING;
            }
            return true; // Segnala che è scaduto
        }
    }
    return false; // Non è scaduto in questo ciclo di loop
}
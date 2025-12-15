#ifndef ESP32TIMER_H
#define ESP32TIMER_H

#include <Arduino.h>

// Stati operativi del timer.
enum TimerState {
    TIMER_STOPPED,    ///< Il timer è fermo e non misura il tempo.
    TIMER_RUNNING,    ///< Il timer è in esecuzione e sta misurando il tempo.
    TIMER_EXPIRED     ///< Il tempo impostato (durata) è scaduto.
};

// Classe per un timer non bloccante basato su millis() per ESP32/Arduino.
class Timer {
private:
    unsigned long start_time_ms_; // Tempo in millis() all'avvio del timer.
    unsigned long duration_ms_;   // Durata totale impostata del timer.
    TimerState current_state_;    // Stato attuale del timer.
    bool auto_reset_enabled_;     // Flag per il reset automatico.

public:
    /**
     * Costruttore. Inizializza il timer.
     * duration Durata iniziale del timer in millisecondi (default 0).
     */
    Timer(unsigned long duration = 0);

    // Avvia o riprende il timer.
    void start();

    // Ferma il timer.
    void stop();

    // Resetta il timer e lo riporta allo stato iniziale.
    void reset();

    /**
     * Imposta la durata del timer in millisecondi.
     * duration La nuova durata in ms.
     */
    void set_duration(unsigned long duration);

    /**
     * @brief Abilita o disabilita la modalità di reset automatico.
     * Se abilitato, il timer si riavvia automaticamente dopo l'expired.
     */
    void set_auto_reset(bool enable);

    /**
     * Aggiorna lo stato interno del timer (chiamata obbligatoria nel loop()).
     * @return true se il timer è appena scaduto, false altrimenti.
     */
    bool update();

    /**
     * Ottiene lo stato attuale del timer.
     * @return Lo stato corrente (TIMER_STOPPED, TIMER_RUNNING, TIMER_EXPIRED).
     */
    TimerState get_state() const { return current_state_; }

    /**
     * Ottiene il tempo trascorso dall'avvio.
     * @return Il tempo trascorso in millisecondi.
     */
    unsigned long get_elapsed_time() const;

    /**
     * Ottiene il tempo rimanente alla scadenza.
     * @return Il tempo rimanente in millisecondi.
     */
    unsigned long get_remaining_time() const;
};

#endif // TIMER_H
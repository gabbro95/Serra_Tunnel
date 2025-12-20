#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// --- I. PINOUT GPIO ---

// I2C (BME280 + Multiplexer + RTC)
#define PIN_SDA                     21
#define PIN_SCL                     22
#define TCA9548A_ADDR               0x70 
#define BME_ADDRESS_COMMON          0x76 
#define RTC_ADDRESS                 0x68 // Indirizzo comune DS3231
#define BME_CHANNEL_OFFICE          0    
#define BME_CHANNEL_SERRA_1         1    
#define BME_CHANNEL_SERRA_2         2    
#define RTC_CHANNEL                 3    

// Analogici
#define PIN_SOIL_1                  32
#define PIN_SOIL_2                  33
#define PIN_BAT                     34
#define PIN_LDR                     35  

// Digitali
#define PIN_PIR_OFFICE              5   
#define PIN_RAIN_SENSOR             23  

// Attuatori (Relè - Active LOW)
#define PIN_RELAY_PUMP              16  
#define PIN_RELAY_GROW_LIGHT        17  
#define PIN_RELAY_HEATER_DESK            27  
#define PIN_RELAY_EXTRACTOR         25 

// PWM 
#define PIN_RELAY_FAN               18 
#define PIN_DESK_LIGHT_PWM          19   

// --- II. PARAMETRI LEDC (PWM) ---
#define LEDC_CHANNEL_DESK_LIGHT     0
#define LEDC_CHANNEL_FAN            1   
#define LEDC_RESOLUTION             10  
#define LEDC_FREQUENCY              5000 

#define DESK_LIGHT_BRIGHTNESS_OFF   0 
#define DESK_LIGHT_BRIGHTNESS_ON    1023

#define FAN_SPEED_OFF_PWM           0 
#define FAN_SPEED_SILENT_PWM        100  
#define FAN_SPEED_RECIRCULATION_PWM 300  
#define FAN_SPEED_TURBO_PWM         1023 

// --- III. SOGLIE CRITICHE ---

// Temperature
#define TEMP_HEATING_MIN            18.0 
#define TEMP_OPTIMAL_MIN            20.0 
#define TEMP_OPTIMAL_MAX            25.0 
#define TEMP_CRITIC_MAX             32.0 
#define TEMP_THRESHOLD_EXTRACT      26.0    // Soglia di accensione Estrattore
#define TEMP_HYSTERESIS             1.0     // Isteresi per spegnimento Estrattore (26 - 1 = 25°C)

// Soglie Umidità
#define HUM_THRESHOLD_EXTRACT       85.0    // Sopra 85% tenta di scaricare fuori (se T > 12°C)
#define HUM_THRESHOLD_RECIRCULATION 75.0    // Sopra 75% accende ventilazione interna preventivo
#define HUM_HYSTERESIS              5.0     // Isteresi per spegnimento
#define TEMP_MIN_FOR_HUMIDITY_VENT  12.0    // NON aprire estrattore per umidità se ci sono meno di 12°C

// Luce (Fotoperiodo)
#define LIGHT_TARGET_HOURS          16      // Ore di luce totali da garantire
#define LUX_THRESHOLD_DAY           100.0   // Soglia per considerare "Giorno" (Luce Naturale)

// Soglia Umidità terreno
#define SOIL_MOISTURE_MIN_PERCENT   40   

// Macro calcolo della durata del Timer
#define SEC_TO_MS(s) ((s) * 1000UL)
#define MIN_TO_MS(m) ((m) * 60 * 1000UL)

// Tempi Timer
#define HEATER_DURATION_ON MIN_TO_MS(15)
#define HEATER_DURATION_OFF MIN_TO_MS(5)
#define WATER_PUMP_DURATION SEC_TO_MS(15) 

#endif
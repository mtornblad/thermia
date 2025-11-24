#include "adc.h"
#include "globals.h"
#include "spoofer.h" // För ResistanceToTemp_100x
#include <xc.h>
#include <stdio.h>

// Vi antar att NTC-givaren är kopplad i en spänningsdelare
// med ett fast motstånd R_fix = 10 kOhm (10000 Ohm) till VCC.
// ADC-värdet (Vout) mäts mellan R_fix och NTC (R_ntc).
#define R_FIX_OHM_100X 1000000L // 10000 Ohm * 100
#define ADC_MAX_VALUE  1023      // 10-bitars ADC (0-1023)

// Interna tillstånd för ADC-sekvensering
#define CHANNEL_OUTDOOR 0b00001 // AN1 (RA1)
#define CHANNEL_INDOOR  0b00101 // AN5 (RB5)

void ADC_Init(void) {
    // ADC Konfiguration:
    
    // 1. Vref: VDD (5V)
    ADREFbits.ADNREF = 0; // Vref- = Vss
    ADREFbits.ADPREF = 0b00; // Vref+ = VDD

    // 2. Clock: Fosc/64 (ADC-klocka ~1MHz)
    ADCLK = 0x3F; // Fosc/64
    
    // 3. Resultatformat: Högerjusterat (10-bitars)
    ADCON0bits.FM = 0b01; 
    
    // 4. Initial kanal: Utetemp AN1
    ADCON0bits.ADCH = CHANNEL_OUTDOOR; 
    
    ADCON0bits.ADON = 1; // Slå på ADC-modulen
}

/**
 * @brief Beräknar NTC-motståndet baserat på rådata från ADC.
 * Förutsätter en spänningsdelare med R_fix på 10kOhm mot VCC.
 * R_NTC = R_fix * ( (ADC_MAX_VALUE + 1) / ADC_VALUE - 1 )
 * @param adc_raw Råvärde från ADC (0-1023).
 * @return Motstånd i Ohm * 100 (int32_t).
 */
static int32_t calculate_ntc_resistance(uint16_t adc_raw) {
    if (adc_raw == 0) {
        return 5000000L; // Maxvärde (50kOhm * 100)
    }
    
    int32_t numerator = (int32_t)R_FIX_OHM_100X * 1024L;
    int32_t term1 = numerator / adc_raw;
    int32_t term2 = R_FIX_OHM_100X;
    
    int32_t resistance_100x = term1 - term2;
    
    if (resistance_100x > 5000000L) {
        return 5000000L;
    }
    
    return resistance_100x;
}


bool ADC_Process(void) {
    static uint8_t state = 0;
    static uint16_t timer = 0; 
    static uint8_t current_channel = CHANNEL_OUTDOOR; // AN1

    timer++; 
    
    switch(state) {
        case 0: // Starta konvertering (vänta 1000 cykler)
            if (timer > 1000) { 
                ADCON0bits.ADGO = 1; // Starta konvertering
                state = 1;
                timer = 0;
            }
            break;
            
        case 1: // Vänta på resultat
            if (!ADCON0bits.ADGO) {
                // Konvertering klar
                uint16_t adc_raw = (ADRESH << 8) | ADRESL;
                
                // --- 1. Konvertera och lagra värdet för den just avlästa kanalen ---
                int32_t resistance_100x = calculate_ntc_resistance(adc_raw);
                int16_t temp_100x;
                uint8_t hi_reg, lo_reg;
                
                // Välj rätt NTC-kurva och lagra resultatet på rätt plats i registerMap
                
                if (current_channel == CHANNEL_OUTDOOR) {
                    temp_100x = ResistanceToTemp_100x(resistance_100x); 
                    hi_reg = REG_ADC_NTC_OUTDOOR_HI;
                    lo_reg = REG_ADC_NTC_OUTDOOR_LO;
                    
                    // Nästa kanal blir INNE
                    current_channel = CHANNEL_INDOOR;
                    ADCON0bits.ADCH = CHANNEL_INDOOR;
                    
                } else { // current_channel == CHANNEL_INDOOR
                    temp_100x = ResistanceToTemp_100x(resistance_100x); 
                    hi_reg = REG_ADC_NTC_INDOOR_HI;
                    lo_reg = REG_ADC_NTC_INDOOR_LO;
                    
                    // Nästa kanal blir UTE
                    current_channel = CHANNEL_OUTDOOR;
                    ADCON0bits.ADCH = CHANNEL_OUTDOOR;
                }
                
                // Lagra beräknad temperatur
                registerMap[hi_reg] = (uint8_t)((temp_100x >> 8) & 0xFF);
                registerMap[lo_reg] = (uint8_t)(temp_100x & 0xFF);
                
                // Lagra råvärde (för debug/kalibrering)
                registerMap[REG_ADC_NTC_RAW_HI] = (uint8_t)((adc_raw >> 8) & 0xFF);
                registerMap[REG_ADC_NTC_RAW_LO] = (uint8_t)(adc_raw & 0xFF);
                       
                state = 0; // Gå tillbaka till start
                timer = 0;
                return true;
            }
            break;
    }
    return false;
}

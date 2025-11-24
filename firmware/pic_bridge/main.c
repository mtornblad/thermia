#include "globals.h"
#include "system.h"
#include "debug.h"
#include "modbus.h"
#include "spoofer.h"
#include "onewire.h"
#include "i2c.h"
#include "adc.h"

// --- HÖG PRIORITETS ISR ---
void __interrupt(high_priority) High_Priority_ISR(void) {
    // 1. Hantera I2C Slave-avbrott (Högt prioriterad/realtidskritisk)
    if (I2C_Slave_ISR_Handler()) {
        // Avbrottet hanterades av I2C-modulen
        return;
    }
    
    // 2. Andra avbrott (T.ex. Timer, UART, etc.)
}

void main(void) {
    // Initiera System (Klocka, Pinnar, PPS)
    SYSTEM_Initialize();
    
    
    // Initiera Moduler
    DEBUG_Init();
    MODBUS_Init();
    SPOOFER_Init();
    ONEWIRE_Init();
    I2C_Init();
    ADC_Init();
    
    printf("Thermia Bridge v1.0 - PIC18F47Q43 Startup\r\n");
    
    // Huvudprogramloop
    while (1) {
        
        // Hantera Modbus-kommunikation med ESP32 (UART2) och RS485 (UART1)
        MODBUS_Task();
        
        // Uppdatera reläer och Digipot baserat på registerMap
        SPOOFER_Process();
        
        // Kör icke-blockerande OneWire-mätning
        ONEWIRE_Process();
        
        // Kör icke-blockerande ADC-mätning (Real NTC values)
        ADC_Process();
        
        // Här kan andra lågprioriterade uppgifter läggas till
        
        // Lägg till en liten fördröjning (för att undvika tight loop)
        __delay_ms(1); 
    }
}

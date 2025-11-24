#include "modbus.h"
#include "globals.h"
#include <stdio.h>

// Global minneskarta
volatile uint8_t registerMap[TOTAL_REGS];

void MODBUS_Init(void) {
    // --- UART1 (RS485 External) - 9600 Baud @ 64MHz ---
    // U1BRG = (64000000 / (16 * 9600)) - 1 = 416.6 -> 416
    U1BRG = 416; 
    U1CON0bits.TXEN = 1;
    U1CON0bits.RXEN = 1;
    U1CON1bits.ON = 1;
    
    // --- UART2 (XIAO Internal) - 115200 Baud @ 64MHz ---
    // U2BRG = 34 (samma som Debug UART3)
    U2BRG = 34; 
    U2CON0bits.TXEN = 1;
    U2CON0bits.RXEN = 1;
    U2CON1bits.ON = 1;
}

// Skickar en byte till XIAO/ESP32 via UART2
void ESP_SendByte(uint8_t data) {
    while(!U2ERRIRbits.TXMTIF);
    U2TXB = data;
}

void MODBUS_Task(void) {
    // 1. Hantera kommandon från XIAO/ESP32 (UART2) - Förenklat protokoll
    // Format: 'R' <RegIndex> eller 'W' <RegIndex> <Value>
    if (U2PIRbits.RXIF) {
        static uint8_t state = 0;
        static uint8_t regIndex = 0;
        
        uint8_t rx = U2RXB;
        
        if (state == 0) {
            if (rx == 'R') state = 1; // Väntar på index att läsa
            else if (rx == 'W') state = 2; // Väntar på index att skriva
        }
        else if (state == 1) {
            // State: Läsa - Har nu fått RegIndex
            regIndex = rx;
            uint8_t val = registerMap[regIndex];
            ESP_SendByte(val); // Skicka tillbaka värdet
            state = 0; // Återgå till start
        }
        else if (state == 2) {
            // State: Skriva (1/2) - Har nu fått RegIndex
            regIndex = rx;
            state = 3; // Väntar på värdet
        }
        else if (state == 3) {
            // State: Skriva (2/2) - Har nu fått Value
            registerMap[regIndex] = rx;
            ESP_SendByte('K'); // Skicka 'OK' (ACK)
            state = 0; // Återgå till start
        }
    }
    
    // 2. Hantera RS485 (UART1) - Förberedd för full Modbus RTU
    if (U1PIRbits.RXIF) {
        // Full Modbus RTU stack/bibliotek bör implementeras här
        volatile uint8_t dummy = U1RXB; // Läs bufferten för att rensa avbrottet
    }
}

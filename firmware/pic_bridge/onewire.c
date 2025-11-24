#include "onewire.h"
#include "globals.h"
#include <xc.h>
#include <stdio.h>

// DS18B20 Commands
#define SKIP_ROM 0xCC
#define CONVERT_T 0x44
#define READ_SCRATCHPAD 0xBE

// Baudrates (@64MHz)
// Reset Puls (480us) => 9600 Baud: U4BRG = 416
#define BAUD_RESET 416
// Bit Puls (60us) => 115200 Baud: U4BRG = 34
#define BAUD_BITS  34

void ONEWIRE_Init(void) {
    // UART4 används i Single-Wire mode
    U4CON0bits.TXEN = 1;
    U4CON0bits.RXEN = 1;
    U4CON0bits.MODE = 0; // Asynchronous 8-bit
    U4CON1bits.ON = 1;
}

// TouchByte använder den aktuella BRG-inställningen (RESET eller BITS)
uint8_t OW_TouchByte(uint8_t data) {
    // Vänta tills TX-bufferten är tom
    while(!U4ERRIRbits.TXMTIF);
    // Läs bufferten för att rensa flaggan (om något fanns kvar)
    U4RXB; 
    U4TXB = data;
    // Vänta på att ta emot data
    while(!U4PIRbits.RXIF); 
    return U4RXB;
}

// Initierar kommunikationen (Reset/Presence)
bool OW_Reset(void) {
    U4CON1bits.ON = 0;
    // Sätt baudrate för Reset-puls (9600)
    U4BRG = BAUD_RESET; 
    U4CON1bits.ON = 1;
    // Skicka Reset Pulse (0xF0) och läs svaret
    uint8_t response = OW_TouchByte(0xF0);
    // Presence Pulse är allt annat än 0xF0
    return (response != 0xF0);
}

// Skicka en bit och läs svaret
uint8_t OW_TouchBit(uint8_t bitVal) {
    U4CON1bits.ON = 0;
    // Sätt baudrate för bit-puls (115200)
    U4BRG = BAUD_BITS; 
    U4CON1bits.ON = 1;

    // Skicka 0xFF för att läsa, 0x00 för att skriva '0'
    uint8_t send = (bitVal) ? 0xFF : 0x00;
    uint8_t recv = OW_TouchByte(send);
    
    // En bit läst som '1' om svaret är 0xFF
    return (recv == 0xFF) ? 1 : 0;
}

// Skriver en hel byte (LSB först)
void OW_WriteByte(uint8_t val) {
    for (uint8_t i = 0; i < 8; i++) {
        OW_TouchBit(val & 0x01);
        val >>= 1;
    }
}

// Läser en hel byte (LSB först)
uint8_t OW_ReadByte(void) {
    uint8_t val = 0;
    for (uint8_t i = 0; i < 8; i++) {
        // Skicka '1' för att trigga Read Slot
        if (OW_TouchBit(1)) { 
            val |= (1 << i);
        }
    }
    return val;
}

// Icke-blockerande process för OneWire-mätning
bool ONEWIRE_Process(void) {
    static uint8_t state = 0;
    // Använd en räknare för att mäta tid (måste kallas ofta)
    static uint16_t timer = 0; 
    
    // Öka räknaren (antar att denna funktion anropas frekvent)
    timer++; 
    
    switch(state) {
        case 0: // Starta mätning (vänta 10000 cykler)
            if (timer > 10000) { 
                if (OW_Reset()) {
                    OW_WriteByte(SKIP_ROM);
                    OW_WriteByte(CONVERT_T); // Starta temperaturkonvertering
                    state = 1;
                    timer = 0;
                }
            }
            break;
            
        case 1: // Vänta på konvertering (DS18B20 tar ca 750ms vid 12-bit)
            if (timer > 50000) { // Exempel: 50,000 cykler (måste kalibreras)
                state = 2;
            }
            break;
            
        case 2: // Läs resultat
            if (OW_Reset()) {
                OW_WriteByte(SKIP_ROM);
                OW_WriteByte(READ_SCRATCHPAD);
                
                uint8_t lo = OW_ReadByte(); // Läs LSB
                uint8_t hi = OW_ReadByte(); // Läs MSB (Sign)
                
                // Konvertera rådata till temperatur
                int16_t raw = (hi << 8) | lo;
                float temp = raw / 16.0f; // Upplösning 1/16 grad
                
                // Lagra i registerMap som int16_t * 100
                int16_t stored = (int16_t)(temp * 100.0f);
                
                registerMap[REG_DS18B20_TEMP_HI] = (stored >> 8) & 0xFF;
                registerMap[REG_DS18B20_TEMP_LO] = stored & 0xFF;
                
                // Debug-utskrift
                printf("Temp: %.2f C\r\n", temp);
            }
            state = 0; // Gå tillbaka till start
            timer = 0;
            return true; // Mätning slutförd
    }
    return false; // Fortfarande i väntan
}

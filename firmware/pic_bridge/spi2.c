#include "spi2.h"
#include "globals.h"
#include "spoofer.h" // För Digipot-kommandon (CMD_WRITE_DATA etc.)
#include <xc.h>

// Intern funktion för att initiera SPI2-modulen
void SPI2_Init(void) {
    // SPI2 konfiguration: Master Mode, FOSC/64 (1 MHz @ 64MHz Fosc)
    
    // SPI är redan mappad i system.c till RA2 (SDO2) och RB4 (SCK2)
    
    SSP2CON1bits.SSPEN = 0; // Avaktivera först
    
    // Master Mode, Clock = FOSC/64
    SSP2CON1bits.SSPM = 0b0010; 
    
    // SPI Mode 0,0 (CPOL=0, CPHA=0)
    SSP2CON1bits.CKP = 0; // Clock Polarity: Idle Low
    SSP2STATbits.CKE = 0; // Clock Edge: Data sampled on rising edge
    
    // SSP2CON2bits.SEN = 0; // Standardläge (ej Slave Select)
    
    SSP2CON1bits.SSPEN = 1; // Aktivera SPI2
    
    // DP_SHDN_PIN (RA5) initieras till 0 (Normal drift) i SPOOFER_Init()
    // DP_CS_PIN (RA4) initieras till 1 (Inaktiv) i SPOOFER_Init()
}

// Skickar en byte via HW SPI2 och returnerar den mottagna byten
uint8_t SPI2_Write_Byte(uint8_t data) {
    SSP2BUF = data; // Ladda data i bufferten
    // Vänta tills överföringen är klar (BF = Buffer Full/Received)
    while(!SSP2STATbits.BF); 
    // Läs bufferten för att rensa flaggan (och få eventuell inkommande data)
    return SSP2BUF; 
}

// Skriver ett wiper-värde (0-255) till Potentiometer 0 (Innegivare)
void SPI2_WriteWiper_Pot0(uint8_t wiper_value) {
    uint8_t command = ADDR_POT_0 | CMD_WRITE_DATA;
    
    DP_CS_PIN = 0; // Aktivera Chip Select (CS Low)
    
    // Skriv Kommando-byte
    SPI2_Write_Byte(command); 
    
    // Skriv Data-byte (värde 0-255)
    SPI2_Write_Byte(wiper_value); 
    
    DP_CS_PIN = 1; // Deaktivera Chip Select (CS High)
}


// Skriver ett wiper-värde (0-255) till Potentiometer 1 (Utegivare)
void SPI2_WriteWiper_Pot1(uint8_t wiper_value) {
    uint8_t command = ADDR_POT_1 | CMD_WRITE_DATA;
    
    DP_CS_PIN = 0; // Aktivera Chip Select (CS Low)
    
    // Skriv Kommando-byte
    SPI2_Write_Byte(command); 
    
    // Skriv Data-byte (värde 0-255)
    SPI2_Write_Byte(wiper_value); 
    
    DP_CS_PIN = 1; // Deaktivera Chip Select (CS High)
}

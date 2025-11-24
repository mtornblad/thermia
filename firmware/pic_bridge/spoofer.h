#ifndef SPOOFER_H
#define SPOOFER_H

#include <stdint.h>
#include <stdbool.h>
#include "globals.h"

// --- Reläer (För de 3 reläerna på RC3, RC4, RC5) ---
#define RELAY_HEAT_PUMP_ON LATCbits.LATC3
#define RELAY_PUMP_1_ON    LATCbits.LATC4
#define RELAY_PUMP_2_ON    LATCbits.LATC5

// --- Digipot SPI Styrning ---
#define DP_CS_PIN      LATAbits.LATA4 // Chip Select (Active Low)
#define DP_SHDN_PIN    LATAbits.LATA5 // Shutdown (Active High)

// --- MCP4251 Kommandon ---
#define CMD_WRITE_DATA      0b00000000
#define CMD_WRITE_TCON      0b01000000
// Övriga kommandon (läs) behövs inte för skrivoperationer

// --- Adresser för Potentiometrar ---
#define ADDR_POT_0          0b00000000 // Potentiometer 0 (Wiper 0)
#define ADDR_POT_1          0b00010000 // Potentiometer 1 (Wiper 1)

// --- Funktioner ---
void SPOOFER_Init(void);
void SPOOFER_Process(void);
void SPOOFER_Write(uint8_t address, uint8_t value);

#endif // SPOOFER_H

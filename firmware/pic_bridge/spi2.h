#ifndef SPI2_H
#define SPI2_H

#include <stdint.h>

// Initierar HW SPI2-modulen
void SPI2_Init(void);

// Skickar och tar emot en byte via HW SPI2
uint8_t SPI2_Write_Byte(uint8_t data);

// Skriver wiper-värde (0-255) till de individuella Digipots
void SPI2_WriteWiper_Pot0(uint8_t wiper_value); // Innegivare
void SPI2_WriteWiper_Pot1(uint8_t wiper_value); // Utetgivare

#endif // SPI2_H

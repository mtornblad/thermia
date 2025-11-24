#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initierar MSSP1-modulen som I2C Slave.
 * Konfigurerar adress, avbrott och driftläge.
 */
void I2C_Init(void);

/**
 * @brief I2C Slave Interrupt Service Routine Logic.
 * Denna funktion hanterar alla I2C-transaktioner (Address Match, Read, Write).
 * Den är designad att kallas från huvud-ISR i main.c.
 * @return true om avbrottet hanterades, false annars.
 */
bool I2C_Slave_ISR_Handler(void);

#endif // I2C_H

#ifndef MODBUS_H
#define	MODBUS_H

#include <stdint.h>

void MODBUS_Init(void);
void MODBUS_Task(void);
void ESP_SendByte(uint8_t data);

#endif	/* MODBUS_H */

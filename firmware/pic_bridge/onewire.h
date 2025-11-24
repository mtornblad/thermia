#ifndef ONEWIRE_H
#define	ONEWIRE_H

#include <stdint.h>
#include <stdbool.h>

void ONEWIRE_Init(void);
// Returnerar true om en mätning slutfördes och lagrades i registerMap
bool ONEWIRE_Process(void); 

#endif	/* ONEWIRE_H */

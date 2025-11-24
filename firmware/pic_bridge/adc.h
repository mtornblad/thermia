#ifndef ADC_H
#define ADC_H

#include <stdbool.h>

/**
 * @brief Initierar ADC-modulen på PIC:en.
 */
void ADC_Init(void);

/**
 * @brief Läser av ADC-värdet från NTC-givaren och konverterar till temperatur.
 * Resultatet lagras i registerMap.
 * @return true om en ny mätning slutfördes.
 */
bool ADC_Process(void);

#endif // ADC_H

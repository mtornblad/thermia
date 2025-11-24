#include "debug.h"
#include "globals.h"
#include <stdio.h>

void DEBUG_Init(void) {
    // UART3 konfigureras för 115200 Baud @ 64MHz
    // U3BRG = (_XTAL_FREQ / (16 * Baud)) - 1 = (64000000 / (16 * 115200)) - 1 = 33.72 -> 34
    U3BRG = 34; 
    U3CON0bits.BRGS = 0; // High speed mode (BRG16=0)
    U3CON0bits.TXEN = 1; // Enable Transmitter
    U3CON0bits.RXEN = 1; // Enable Receiver
    U3CON1bits.ON = 1;   // Enable UART3
}

// putch är den funktionen XC8-kompilatorn kallar när man använder printf
void putch(char data) {
    // Vänta tills TX-bufferten är tom (TXMTIF = Transmit Shift Register Empty)
    while(!U3ERRIRbits.TXMTIF); 
    U3TXB = data;
}

#include "system.h"
#include "globals.h"

void SYSTEM_Initialize(void) {
    OSCFRQ = 0x08;  // 64 MHz
    OSCCON1 = 0x60; // HFINTOSC (Internal Oscillator)
    PIN_MANAGER_Initialize();
    INTCON0bits.GIE = 1; // Global Interrupt Enable
}

void PIN_MANAGER_Initialize(void) {
    PPSLOCK = 0x55; PPSLOCK = 0xAA; PPSLOCKbits.PPSLOCKED = 0;

    // --- UART1 (RS485) RC6/RC7 --- (No change, standard UART)
    RC6PPS = 0x20; U1RXPPS = 0x17; // RC6 = TX, RC7 = RX
    TRISCbits.TRISC6 = 0; TRISCbits.TRISC7 = 1;
    TRISCbits.TRISC2 = 0; LATCbits.LATC2 = 0; // DE/RE (Output)

    // --- UART2 (XIAO/ESP32) RC0/RC1 --- (Via Level Shifter)
    RC0PPS = 0x21; U2RXPPS = 0x11; // RC0 = TX, RC1 = RX
    TRISCbits.TRISC0 = 0; TRISCbits.TRISC1 = 1;
    // INLVLCbits.INLVLC1 = 0; // Not needed with Level Shifter

    // --- UART3 (Debug) RB0/RB3 ---
    RB0PPS = 0x22; U3RXPPS = 0x0C; 
    TRISBbits.TRISB0 = 0; TRISBbits.TRISB3 = 1;

    // --- UART4 (OneWire Master) RA0 ---
    RA0PPS = 0x23; U4RXPPS = 0x00;
    TRISAbits.TRISA0 = 1; 
    ODCONAbits.ODCA0 = 1; // Open Drain för OneWire

    // --- I2C1 (Pump) RB1/RB2 --- (Shared pins)
    I2C1SCLPPS = 0x09; RB1PPS = 0x37; // RB1 = SCL
    I2C1SDAPPS = 0x0A; RB2PPS = 0x38; // RB2 = SDA
    TRISBbits.TRISB1 = 1; TRISBbits.TRISB2 = 1; 
    ODCONBbits.ODCB1 = 1; ODCONBbits.ODCB2 = 1; // Open Drain (requires external pull-ups)

    // --- SPOOFING: RELAY Control (RC3, RC4, RC5) ---
    TRISCbits.TRISC3 = 0; LATCbits.LATC3 = 0; // RELAY 1 (Output)
    TRISCbits.TRISC4 = 0; LATCbits.LATC4 = 0; // RELAY 2 (Output)
    TRISCbits.TRISC5 = 0; LATCbits.LATC5 = 0; // RELAY 3 (Output)
    ANSELCbits.ANSELC3 = 0; ANSELCbits.ANSELC4 = 0; ANSELCbits.ANSELC5 = 0;

    // --- SPOOFING: DIGIPOT SPI Control (RA4, RA5) ---
    // CS (Chip Select) och SHDN (Shutdown)
    TRISAbits.TRISA4 = 0; LATAbits.LATA4 = 1; // DP_CS (Chip Select, Active Low)
    TRISAbits.TRISA5 = 0; LATAbits.LATA5 = 1; // DP_SHDN (Shutdown, Active High)
    ANSELAbits.ANSELA4 = 0; ANSELAbits.ANSELA5 = 0;

    // --- ADC PINS (Real NTC Sensors) ---
    TRISAbits.TRISA1 = 1; ANSELAbits.ANSELA1 = 1; // RA1 (AN1) - Utegivare
    TRISBbits.TRISB5 = 1; ANSELBbits.ANSELB5 = 1; // RB5 (AN5) - Innegivare

    // --- NTC CURVE SELECT (RD0) ---
    TRISDbits.TRISD0 = 1; // RD0 som Input
    WPUDRbits.WPUDR0 = 1; // Aktivera intern Pull-up
    // Läs och lagra NTC Curve Select (0 = 22k, 1 = 150 Ohm)
    if (PORTDbits.RD0 == 0) { // Om jordad (Bygel till GND)
        registerMap[REG_NTC_CURVE_SELECT] = 0; // 22 kOhm
    } else {
        registerMap[REG_NTC_CURVE_SELECT] = 1; // 150 Ohm (Default)
    }

    // --- SPI2 (Used for Digipot) ---
    // SDO2 (Output) - Used for transmitting data to the Digipot
    RA2PPS = 0x24; 
    TRISAbits.TRISA2 = 0; ANSELAbits.ANSELA2 = 0;
    
    // SCK2 (Clock) - Clock signal for the Digipot
    RB4PPS = 0x25;
    TRISBbits.TRISB4 = 0; ANSELBbits.ANSELB4 = 0;

    // --- ICSP/OTA PINS (Controlled by XIAO/Level Shifter) ---
    TRISAbits.TRISA3 = 1; // RA3 (MCLR) as input (Will be controlled externally)
    ANSELAbits.ANSELA3 = 0;
    
    // RB6, RB7 används för PGC/PGD (Input under normal drift)
    TRISBbits.TRISB6 = 1; // PGC/PGD
    TRISBbits.TRISB7 = 1;
    ANSELBbits.ANSELB6 = 0; ANSELBbits.ANSELB7 = 0;

    PPSLOCK = 0x55; PPSLOCK = 0xAA; PPSLOCKbits.PPSLOCKED = 1;
}

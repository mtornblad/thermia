#ifndef GLOBALS_H
#define	GLOBALS_H

#include <xc.h> // Huvud-header för PIC-familjen (bytt till PIC18F47Q43)
#include <stdint.h>
#include <stdbool.h>

// Systemfrekvens (64MHz intern oscillator)
#define _XTAL_FREQ 64000000UL

// I2C Address mot Värmepumpen
#define I2C_SLAVE_ADDR 0x2E 

// Minnesstorlek (Matchar Thermias registerrymd)
#define TOTAL_REGS 256

// --- PIC VERSION & DIAGNOSTIK (Modbus Holding/Input Regs: 250-255) ---
#define REG_FW_MAJOR_VERSION    250
#define REG_FW_MINOR_VERSION    251
#define REG_I2C_STATUS          252 // I2C State Machine status/felkod
#define REG_NTC_CURVE_SELECT    253 // NTC kurva: 1=150 Ohm, 0=22 kOhm (Läst från RD0)


// --- RIKTIGA SENSORVÄRDEN (Modbus Input Regs: 200+) ---
// DS18B20 (OneWire) Temperatur: Lagras som °C * 100
#define REG_DS18B20_TEMP_HI     200
#define REG_DS18B20_TEMP_LO     201
// ADC NTC Utegivare (Riktigt värde, innan spoofing): Lagras som °C * 100
#define REG_ADC_NTC_OUTDOOR_HI  202
#define REG_ADC_NTC_OUTDOOR_LO  203
// ADC NTC Innegivare (Riktigt värde, innan spoofing): Lagras som °C * 100
#define REG_ADC_NTC_INDOOR_HI   204
#define REG_ADC_NTC_INDOOR_LO   205
// ADC Rådata för NTC (för kalibrering)
#define REG_ADC_NTC_RAW_HI      206
#define REG_ADC_NTC_RAW_LO      207


// --- XIAO KONTROLL & SPOOFING MÅL (Modbus Holding Regs: 230+) ---
#define REG_SPOOFING_ENABLED    230 // 1 = På, 0 = Av (Styr Digipots)

// Måltemperaturer för Spoofing: Lagras som °C * 100
#define REG_TARGET_OUTDOOR_TEMP_HI  231 // Mål för Utetemp (Pot 1)
#define REG_TARGET_OUTDOOR_TEMP_LO  232
#define REG_TARGET_INDOOR_TEMP_HI   233 // Mål för Innetemp (Pot 0)
#define REG_TARGET_INDOOR_TEMP_LO   234

// Relästyrning (Bitfält)
#define REG_RELAY_CONTROL       240 // Bit 0: EVU/Heat Pump, Bit 1: Pump 1, Bit 2: Pump 2

// I2C Kommando Hook (För att hantera Master Write-triggning)
#define REG_I2C_ENABLE_CONTROL  241 // 1 = I2C ISR på, 0 = Av

// --- FÅNGADE KOMMANDON FRÅN PUMPEN (I2C Master Write, Loggas av PIC) ---
#define REG_TARGET_COMMAND_ADDR     242 // Den ThermiaIQ-adress pumpen ville styra (t.ex. 0x0F)
#define REG_TARGET_COMMAND_VALUE_HI 243 // Hög byte av börvärdet pumpen ville skriva
#define REG_TARGET_COMMAND_VALUE_LO 244 // Låg byte av börvärdet pumpen ville skriva


// --- THERMIA IQ STATUS (I2C Master Write Loggning 1:1) ---

// --- SENSORER (2-byte, 0.1 C upplösning) ---
#define REG_T_OUTDOOR_HI        0   // Utomhustemp.
#define REG_T_OUTDOOR_LO        1
#define REG_T_ROOM_HI           2   // Rumstemp.
#define REG_T_ROOM_LO           3
#define REG_T_FLOW_HI           4   // Framledningstemp.
#define REG_T_FLOW_LO           5
#define REG_T_RETURN_HI         6   // Returledningstemp.
#define REG_T_RETURN_LO         7
#define REG_T_WATER_HI          8   // Varmvattentemp.
#define REG_T_WATER_LO          9
#define REG_T_BRINE_IN_HI       10  // Brine in temp.
#define REG_T_BRINE_IN_LO       11
#define REG_T_BRINE_OUT_HI      12  // Brine ut temp.
#define REG_T_BRINE_OUT_LO      13

// --- STATUS/BOOLEAR (1-byte) ---
#define REG_S_STATUS1           18  // STATUS1 (Bit 0: Pump Status, 1: EVU)
#define REG_S_ALARM_HIGH_PRESS  19
#define REG_S_ALARM_LOW_PRESS   20
#define REG_S_ALARM_MOTOR       21
#define REG_S_ALARM_LOW_FLOW    22
#define REG_S_ALARM_BRINE       23
#define REG_S_ALARM_OUTDOOR     24
#define REG_S_ALARM_FLOW        25
#define REG_S_ALARM_RETURN      26
#define REG_S_ALARM_WATER       27
#define REG_S_ALARM_ROOM        28
#define REG_S_ALARM_OVERHEAT    29

// --- KONTROLLER (2-byte) ---
#define REG_SET_FLOW_TARGET_HI  30  // Framledningstemp., bör
#define REG_SET_FLOW_TARGET_LO  31
#define REG_SET_SHUNT_TARGET_HI 32  // Framledn.temp., shunt, bör
#define REG_SET_SHUNT_TARGET_LO 33

// --- INSTÄLLNINGAR/BERÄKNADE (1-byte eller 2-byte) ---
#define REG_I_INTEGRAL_HI       40
#define REG_I_INTEGRAL_LO       41
#define REG_I_INTEGRAL_UPPNADD  42
#define REG_S_PROGRAM_VERSION   43
#define REG_SET_ROOM_TARGET_HI  44  // Rumstemp., bör (Används av Thermiq Write)
#define REG_SET_ROOM_TARGET_LO  45

// ... Fortsättning av register (ThermiaIQ 50+) ...
#define REG_P_DRIFTLAGE         50
#define REG_P_KURVA             51
#define REG_P_RUMFAKTOR         60
#define REG_P_UTETEMP_STOPP     66

// ... och så vidare upp till 116
#define REG_P_LOGGTID           100
#define REG_P_DRIFTTID_COMPR    109
#define REG_P_DRIFTTID_TILLSATTS 113

// PIC Firmware Version
#define FW_VERSION_MAJOR 1
#define FW_VERSION_MINOR 0

// Global minneskarta - Delad resurs mellan I2C, Modbus och Ethernet
extern volatile uint8_t registerMap[TOTAL_REGS];

#endif	/* GLOBALS_H */

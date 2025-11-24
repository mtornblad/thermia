# Thermia Bridge Project Context (PIC18F47Q43 + Universal XIAO)

Detta dokument sammanfattar arkitekturen, hårdvarukonfigurationen, firmwarens uppbyggnad och de nästa stegen för utvecklingen av **Baseboard v30 (Thermia Bridge)**. Huvudmålet är att verifiera PIC-firmwaren och slutföra EasyEDA-schemat.

## 1. Arkitektur & Hårdvara

**Designnamn:** Baseboard v30 (Full Feature / Connector Edition)
**Filosofi:** Separerad 5V Safety Core (**PIC18F47Q43**) från 3.3V Network Core (**XIAO/ESP32**).

| Komponent | Funktion | Noteringar |
| :--- | :--- | :--- |
| **MCU (Safety Core)** | **PIC18F47Q43 (5V)** | Huvudprocessor. Kör Modbus RTU Slave. **(40-pins paket)** |
| **Network Socket** | XIAO/ESP32 Universal | Sockel för WiFi/Ethernet/Thread. **Stöder LVP/OTA** |
| **Ethernet** | W5500 + Jack | Integrerad på PCB. Drivs av XIAO SPI. |
| **Digipot/Spoofer** | MCP4251 + 3x Reläer | Styrning av Ute-/Innegivare & EVU-kontakt. |
| **I2C & Power** | JST-XH 4-pin | Huvudanslutning till Värmepump (5V/GND/SCL/SDA). |
| **OneWire** | Grove 4-pin | För DS18B20-sensorer. |
| **ICSP Port** | 1.27mm Header (1x6) | Programmeringsport för PICkit. **Används för XIAO OTA.** |

### Kritiska Pinout (PIC18F47Q43):

* **I2C (Pump):** RB1 (SCL), RB2 (SDA).
* **UART1 (RS485):** RC6 (TX), RC7 (RX), RC2 (DE/RE).
* **UART2 (XIAO Comms):** RC0 (TX), RC1 (RX). *(Via nivåskiftare)*
* **UART3 (Debug):** RB0 (TX), RB3 (RX).
* **UART4 (OneWire HW):** RA0 (TX/RX/Single-Wire).
* **ADC (Riktig NTC):** RA1 (Ute NTC), RB5 (Inne NTC).
* **NTC Curve Select:** **RD0** (Digital input för 150Ω/22kΩ val).
* **Spoofer Relays:** RC3, RC4, RC5.
* **Digipot SPI (SPI2):** RC2 (SCK2 - flyttad), RA2 (SDO2), RA4 (CS), RA5 (SHDN).
* **OTA Control:** **RA3 (MCLR)**, RB6 (PGC), RB7 (PGD).

> **Designbeslut: PIC-byte till PIC18F47Q43**
> Byte till 40-pinners PIC18F47Q43 gjordes för att eliminera pin-konflikter. Detta möjliggjorde separata pinnar för NTC Curve Select (**RD0**), dedikerade ADC-ingångar (RA1, RB5) och frigjorde pinnar för den komplexa OTA/LVP-kontrollen via XIAO.

### Hårdvaru- & OTA-krav:
1.  **Nivåskiftning:** Alla I/O mellan PIC (5V) och XIAO (3.3V) måste gå via **bidirektionella nivåskiftare** (inklusive UART2 TX/RX, PGC, PGD).
2.  **MCLR Styrning (LVP):** PIC RA3 (MCLR) måste kontrolleras av XIAO via en MOSFET/Transistor för att kunna leverera 5V och försätta PIC:en i LVP-läge.

## 2. Firmware (PIC C Code - XC8)

Firmware är uppdelad i moduler. **I2C-Slaven (i `main.c`) har högsta prioritet.**

### Huvudtasker (main.c)
* **I2C ISR:** Hanterar snabb kommunikation med pumpen.
* **MODBUS_Task():** Hanterar Modbus RTU-protokollet via UART2.
* **ONEWIRE_Process():** Läser DS18B20 sensorer via UART4.
* **ADC_Process():** Läser och konverterar riktiga NTC-värden (Ute/Inne).
* **SPOOFER_Process():** Uppdaterar Digipots och reläer baserat på Modbus-mål.

## 3. Nästa steg för Utveckling

Den mest kritiska uppgiften som återstår är:
* **XIAO: PIC Programmering:** Implementera PIC ICSP-protokollet i `pic_ota.cpp` för att möjliggöra OTA-uppdateringar.

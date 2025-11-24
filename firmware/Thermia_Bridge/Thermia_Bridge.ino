/*
 * Thermia I2C <-> Modbus Bridge
 * Hårdvara: Seeed Studio XIAO RA4M1
 * Bibliotek: Modbus-Master-Slave-for-Arduino (smarmengol)
 */

// ============================================================================
// *** KONFIGURATION AV SCENARIO ***
// Avkommentera raden nedan om du kör Scenario 2 (Piggyback med C6 direkt på).
// ============================================================================

// #define CONFIG_PIGGYBACK_MODE 

// ============================================================================

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ModbusRtu.h>   // Smarmengol biblioteket
#include <EEPROM.h>
#include <SoftwareSerial.h>

// --- Konfiguration beroende på Scenario ---
#ifdef CONFIG_PIGGYBACK_MODE
  // SCENARIO 2: PIGGYBACK (Hardware Serial1)
  #define COMM_SERIAL Serial1
  #define COMM_BAUD 9600
  #define RS485_DIR_PIN 0 // Används ej, sätt till 0
#else
  // SCENARIO 1 & 3: RS485 MODUL (SoftwareSerial)
  SoftwareSerial rs485Serial(D4, D5); // RX=D4, TX=D5
  #define COMM_SERIAL rs485Serial
  #define COMM_BAUD 9600
  #define RS485_DIR_PIN D2 
#endif

// --- Debug Serial ---
#define DEBUG_SERIAL Serial

// --- Gemensam Hårdvara ---
#define I2C_SLAVE_ADDR 0x2E   
#define MODBUS_SLAVE_ID 10    
#define ONE_WIRE_BUS D0        

// --- Minnesmappning ---
#define TOTAL_REGS 1020 
#define SENSOR_START_REG 1000
#define MAX_SENSORS 10 

// --- Globala Data ---
// Detta är "minnet" som Modbus och I2C delar på
uint16_t au16data[TOTAL_REGS];

// Modbus Objekt (ID, Port, TxEnablePin)
Modbus slave(MODBUS_SLAVE_ID, COMM_SERIAL, RS485_DIR_PIN);

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

volatile uint8_t i2cIndex = 0;
unsigned long lastTempRequest = 0;
const unsigned long tempInterval = 10000;

struct SavedSensor {
  DeviceAddress addr;
  bool active;
};

SavedSensor knownSensors[MAX_SENSORS];

void setup() {
  DEBUG_SERIAL.begin(115200);
  
  // Initiera Modbus Serial
  COMM_SERIAL.begin(COMM_BAUD);
  
  // Starta Modbus (Smarmengol biblioteket startar via konstruktorn och begin())
  slave.start();

  // Initiera OneWire
  sensors.begin();
  
  // Initiera I2C Slav på D8/D9 (Wire)
  Wire.setSDA(D9);
  Wire.setSCL(D8);
  Wire.begin(I2C_SLAVE_ADDR);
  Wire.onReceive(receiveEvent);
  Wire.onRequest(requestEvent);

  DEBUG_SERIAL.println("--- Thermia Bridge Started ---");
  #ifdef CONFIG_PIGGYBACK_MODE
    DEBUG_SERIAL.println("Mode: Piggyback");
  #else
    DEBUG_SERIAL.println("Mode: RS485 (Ctrl D2)");
  #endif
  
  initSensors();
}

void loop() {
  // Modbus Poll: Vi skickar med vår data-array
  slave.poll(au16data, TOTAL_REGS);
  
  yield();
  handleTemperature();
}

// --- Sensorlogik ---

void initSensors() {
  DEBUG_SERIAL.println("Scanning OneWire...");
  
  for (int i = 0; i < MAX_SENSORS; i++) {
    int eepromAddr = i * 8;
    for (int j = 0; j < 8; j++) {
      knownSensors[i].addr[j] = EEPROM.read(eepromAddr + j);
    }
    knownSensors[i].active = false;
  }

  int deviceCount = sensors.getDeviceCount();
  DEBUG_SERIAL.print("Found devices: ");
  DEBUG_SERIAL.println(deviceCount);

  for (int i = 0; i < deviceCount; i++) {
    DeviceAddress tempAddr;
    if (sensors.getAddress(tempAddr, i)) {
      int index = registerSensor(tempAddr);
      if (index >= 0) {
         printAddress(tempAddr);
         DEBUG_SERIAL.print(" -> Reg ");
         DEBUG_SERIAL.println(SENSOR_START_REG + index);
      }
    }
  }
}

int registerSensor(DeviceAddress addr) {
  for (int i = 0; i < MAX_SENSORS; i++) {
    if (matchAddress(addr, knownSensors[i].addr)) {
      knownSensors[i].active = true;
      return i;
    }
  }

  for (int i = 0; i < MAX_SENSORS; i++) {
    if (isEmptySlot(knownSensors[i].addr)) {
      memcpy(knownSensors[i].addr, addr, 8);
      knownSensors[i].active = true;
      DEBUG_SERIAL.print("New sensor saved to slot ");
      DEBUG_SERIAL.println(i);
      for (int j = 0; j < 8; j++) {
        EEPROM.write((i * 8) + j, addr[j]);
      }
      return i;
    }
  }
  return -1;
}

void handleTemperature() {
  unsigned long now = millis();
  if (now - lastTempRequest >= tempInterval) {
    lastTempRequest = now;
    sensors.requestTemperatures(); 
    
    for (int i = 0; i < MAX_SENSORS; i++) {
      if (knownSensors[i].active) {
        float tempC = sensors.getTempC(knownSensors[i].addr);
        if(tempC != DEVICE_DISCONNECTED_C) {
          int16_t storedTemp = (int16_t)(tempC * 100);
          // Uppdatera arrayen direkt
          au16data[SENSOR_START_REG + i] = storedTemp;
        }
      }
    }
  }
}

// --- Hjälpfunktioner ---

bool matchAddress(DeviceAddress a, DeviceAddress b) {
  for (uint8_t i = 0; i < 8; i++) if (a[i] != b[i]) return false;
  return true;
}

bool isEmptySlot(DeviceAddress a) {
  bool allZero = true, allFF = true;
  for (uint8_t i = 0; i < 8; i++) {
    if (a[i] != 0) allZero = false;
    if (a[i] != 0xFF) allFF = false;
  }
  return allZero || allFF;
}

void printAddress(DeviceAddress deviceAddress) {
  for (uint8_t i = 0; i < 8; i++) {
    if (deviceAddress[i] < 16) DEBUG_SERIAL.print("0");
    DEBUG_SERIAL.print(deviceAddress[i], HEX);
  }
}

// --- I2C Callbacks ---

void receiveEvent(int howMany) {
  if (howMany == 0) return;
  i2cIndex = Wire.read();
  while (Wire.available()) {
    uint8_t val = Wire.read();
    if (i2cIndex < 256) {
      int8_t signedVal = (int8_t)val;
      // Skriv direkt till den globala arrayen
      au16data[i2cIndex] = (int16_t)signedVal;
      i2cIndex++;
    } else { Wire.read(); }
  }
}

void requestEvent() {
  if (i2cIndex < 256) {
    // Läs direkt från den globala arrayen
    uint16_t val16 = au16data[i2cIndex];
    Wire.write((uint8_t)val16);
  } else { Wire.write(0x00); }
}
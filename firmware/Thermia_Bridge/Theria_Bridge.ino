/*
 * Thermia I2C <-> Modbus Bridge
 * Hårdvara: Seeed Studio XIAO RA4M1
 */

// ============================================================================
// *** KONFIGURATION AV SCENARIO ***
// Avkommentera raden nedan om du kör Scenario 2 (Piggyback med C6 direkt på).
// Låt den vara kommenterad om du kör Scenario 1 eller 3 (RS485-modul).
// ============================================================================

// #define CONFIG_PIGGYBACK_MODE 

// ============================================================================

#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ModbusRTU.h>
#include <EEPROM.h>
#include <SoftwareSerial.h>

// --- Konfiguration beroende på Scenario ---
#ifdef CONFIG_PIGGYBACK_MODE
  // --- SCENARIO 2: PIGGYBACK ---
  // Vi använder Hardware Serial (Serial1) på D6/D7.
  // Fördel: Bättre prestanda och möjliggör direkt stacking av korten.
  #define COMM_SERIAL Serial1
  #define COMM_BAUD 9600
  #define RS485_DIR_PIN -1 // Ingen flödeskontroll vid direkt UART
  
#else
  // --- SCENARIO 1 & 3: RS485 MODUL ---
  // Vi måste använda D4/D5 pga hur Seeed-skölden är byggd.
  // Eftersom RA4M1 inte har standard UART där kör vi SoftwareSerial.
  SoftwareSerial rs485Serial(D4, D5); // RX=D4, TX=D5
  #define COMM_SERIAL rs485Serial
  #define COMM_BAUD 9600
  #define RS485_DIR_PIN D2 // Skölden kräver styrning på D2
#endif

// --- Gemensam Hårdvara ---
#define I2C_SLAVE_ADDR 0x2E   
#define MODBUS_SLAVE_ID 10    
#define ONE_WIRE_BUS D0        // Skruvplint "INT" på RS485-kortet

// --- Minnesmappning ---
#define TOTAL_REGS 1020 
#define SENSOR_START_REG 1000
#define MAX_SENSORS 10 

// --- Globala Objekt ---
ModbusRTU mb;
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
  
  // Initiera Kommunikation (Hårdvara eller Mjukvara beroende på #define)
  COMM_SERIAL.begin(COMM_BAUD);
  
  // Konfigurera Flow Control (om RS485)
  if (RS485_DIR_PIN != -1) {
     pinMode(RS485_DIR_PIN, OUTPUT);
     digitalWrite(RS485_DIR_PIN, LOW); // Lyssna
  }
  
  mb.begin(&COMM_SERIAL, RS485_DIR_PIN);
  mb.slave(MODBUS_SLAVE_ID);
  mb.addHreg(0, 0, TOTAL_REGS); 

  // Initiera OneWire
  sensors.begin();
  
  // Initiera I2C Slav på D8/D9 (Wire1) - 5V Toleranta
  Wire1.begin(I2C_SLAVE_ADDR);
  Wire1.onReceive(receiveEvent);
  Wire1.onRequest(requestEvent);

  DEBUG_SERIAL.println("--- Thermia Bridge Started ---");
  #ifdef CONFIG_PIGGYBACK_MODE
    DEBUG_SERIAL.println("Mode: Piggyback (Hardware Serial1 on D6/D7)");
  #else
    DEBUG_SERIAL.println("Mode: RS485 (SoftwareSerial on D4/D5 + Ctrl D2)");
  #endif
  DEBUG_SERIAL.println("I2C: D9(SDA)/D8(SCL)");
  DEBUG_SERIAL.println("OneWire: D0");
  
  initSensors();
}

void loop() {
  mb.task();
  yield();
  handleTemperature();
}

// --- Sensorlogik ---

void initSensors() {
  DEBUG_SERIAL.println("Scanning OneWire bus on Pin D0...");
  
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
          mb.Hreg(SENSOR_START_REG + i, storedTemp);
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

// --- I2C Callbacks (Wire1) ---

void receiveEvent(int howMany) {
  if (howMany == 0) return;
  i2cIndex = Wire1.read();
  while (Wire1.available()) {
    uint8_t val = Wire1.read();
    if (i2cIndex < 256) {
      int8_t signedVal = (int8_t)val;
      mb.Hreg(i2cIndex, (int16_t)signedVal);
      i2cIndex++;
    } else { Wire1.read(); }
  }
}

void requestEvent() {
  if (i2cIndex < 256) {
    uint16_t val16 = mb.Hreg(i2cIndex);
    Wire1.write((uint8_t)val16);
  } else { Wire1.write(0x00); }
}
# **Thermia Bridge (I2C \<-\> Modbus)**

Detta projekt gör det möjligt att koppla upp äldre Thermia och Danfoss värmepumpar (Diplomat, Optima m.fl.) till Home Assistant.

Lösningen bygger på en **Seeed Studio XIAO RA4M1** som agerar säker 5V-tolk mot värmepumpen.

## **Konfigurera Hårdvara**

Det finns två huvudlägen att köra på. Du måste ställa in detta i koden\!

### **1\. Inställning i Thermia\_Bridge.ino (RA4M1)**

Öppna filen och titta högst upp.

* **Scenario 1 & 3 (RS485):** Låt raden \#define CONFIG\_PIGGYBACK\_MODE vara bortkommenterad (//). Detta aktiverar SoftwareSerial på D4/D5 för att matcha skölden.  
* **Scenario 2 (Piggyback):** Ta bort // framför raden. Detta aktiverar Serial1 (Hardware) på D6/D7 för direkt stacking.

### **2\. Inställning i thermia\_c6\_bridge.yaml (ESPHome)**

Om du använder C6:an, öppna filen och titta under substitutions.

* **Scenario 2 (Piggyback):** Avkommentera blocket för Alternativ A.  
* **Scenario 3 (RS485):** Avkommentera blocket för Alternativ B (och kom ihåg att aktivera flow\_control\_pin längre ner i filen).

## **Hårdvara & Inkoppling**

### **Pinout-tabell (RA4M1)**

| Funktion | Kopplas till XIAO Pin | Anslutningstyp | Anteckning |
| :---- | :---- | :---- | :---- |
| **I2C SDA** | **D9** | Lödpunkt | 5V Tolerant (Kritisk\!) |
| **I2C SCL** | **D8** | Lödpunkt | 5V Tolerant (Kritisk\!) |
| **OneWire** | **D0** | Skruvplint | Märkt **"INT"** på kortet |
| **GND** | **GND** | Skruvplint | Gemensam jord |
| **5V** | **5V** | Skruvplint | Drivning från pump |
| *RS485 RX* | *D4* | *Internt* | Används av RS485-modul |
| *RS485 TX* | *D5* | *Internt* | Används av RS485-modul |
| *RS485 Ctrl* | *D2* | *Internt* | Används av RS485-modul |
| *Piggy RX* | *D7* | *Internt* | Används vid Piggyback |
| *Piggy TX* | *D6* | *Internt* | Används vid Piggyback |

### **⚠️ VARNING: MAGISK RÖK (Endast Piggyback)**

Om du kör **Scenario 2 (Piggyback)** där du trycker fast en XIAO C6 ovanpå RA4M1:

* **DU MÅSTE KLIPPA BENET D0 PÅ C6-KORTET\!**  
* **Varför?** Vi matar sensorn med 5V för stabilitet. RA4M1 tål detta, men C6 tål max 3.3V. Om benet har kontakt skickar du 5V rakt in i processorn på C6 och bränner den omedelbart. Klipp benet så är du säker.

### **🌡️ OneWire & Pull-up (Enklaste lösningen)**

För att temperatursensorn (DS18B20) ska fungera behövs ett motstånd (4.7kΩ) mellan Data och 5V.  
På RS485-kortet kan du lösa detta helt utan att löda:

1. Stoppa in sensorns **Röda kabel (5V)** i skruvplinten märkt **5V**.  
2. Stoppa in sensorns **Gula/Vita kabel (Data)** i skruvplinten märkt **INT**.  
3. Ta ditt 4.7kΩ motstånd och skruva fast det **mellan samma plintar (5V och INT)** tillsammans med kablarna.  
4. Klart\!

## **Välj ditt Scenario**

### **Scenario 1: Trådbundet**

**RA4M1 \-\> RS485 Modul \-\> Kabel \-\> USB-adapter \-\> HA**

* Ladda upp Thermia\_Bridge.ino (Standardinställning).  
* Använd modbus\_thermia.yaml i HA.

### **Scenario 2: Trådlös Piggyback**

**RA4M1 \-\> XIAO C6 (Stackad) \-\> WiFi \-\> HA**

* Konfig: Avkommentera \#define CONFIG\_PIGGYBACK\_MODE i RA4M1.  
* **Hårdvara: Klipp ben D0 på C6\!**  
* ESPHome: Använd Alternativ A i konfigen.

### **Scenario 3: Trådlös RS485-brygga**

**RA4M1 \-\> RS485 Modul \-\> Kabel \-\> RS485 Modul \-\> XIAO C6 \-\> HA**

* Konfig: Låt \#define vara kommenterad i RA4M1.  
* ESPHome: Använd Alternativ B i konfigen \+ avkommentera flow\_control\_pin.

## **Länkar & Referenser**

### **Hårdvara (Seeed Studio)**

* **XIAO RA4M1:** [Produktsida](https://www.seeedstudio.com/Seeed-XIAO-RA4M1-p-5943.html) – Den 5V-toleranta hjärnan i projektet.  
* **XIAO ESP32-C6:** [Produktsida](https://www.seeedstudio.com/Seeed-Studio-XIAO-ESP32C6-p-5884.html) – För WiFi/Thread/Matter (Scenario 2 & 3).  
* **RS485 Expansion Board:** [Produktsida](https://www.seeedstudio.com/RS485-Breakout-Board-for-XIAO-p-6306.html) – Modulen för robust kommunikation.  
* **RS485 Wiki:** [Officiell Wiki](https://wiki.seeedstudio.com/XIAO-RS485-Expansion-Board/) – Dokumentation om pinout och scheman.

### **Mjukvara & Inspiration**

* **ThermIQ-MQTT:** [GitHub Repo](https://github.com/ThermIQ/thermiq_mqtt-ha) – Originalprojektet som detta är kompatibelt med (namngivning av sensorer).  
* **ThermIQ:** [Hemsida](https://thermiq.net/) – Mer info om protokoll och hårdvara.

## **Credits**

* **ThermIQ-projektet:** Inspiration och registermappning.  
* **Gemini & mtornblad:** Kod, arkitektur och dokumentation.
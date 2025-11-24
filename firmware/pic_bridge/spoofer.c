#include "spoofer.h"
#include "globals.h"
#include "spi2.h" // Använd HW SPI2

/* Funktionen SPOOFER_Write är inte längre nödvändig då spi2.c exponerar Pot0 och Pot1 direkt */
    
    DP_CS_PIN = 0; // Aktivera Chip Select (CS Low)
    
    // Skriv Kommando-byte
    SPI_Write_Byte(command); 
    
    // Skriv Data-byte (värde 0-255)
    SPI_Write_Byte(value); 
    
    DP_CS_PIN = 1; // Deaktivera Chip Select (CS High)
}

// --- NTC LOOKUP TABELLER (Motstånd i Ohm * 100) ---

// 150 Ohm NTC (Vanlig Thermia Utegivare)
const int16_t TEMP_INDEX[] = {-4000, -3500, -3000, -2500, -2000, -1500, -1000, -500, 0, 500, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000};
const int32_t RES_150OHM_100X[] = {328900L, 247800L, 188400L, 144400L, 111500L, 86850L, 68120L, 53820L, 42820L, 34290L, 27640L, 22410L, 18280L, 15000L, 12370L, 10260L, 8554L, 7165L, 6030L};

// 22 kOhm NTC (Vanlig Thermia Innegivare, vid 25C)
// Interpolationsdata (förenklad kurva baserat på 22k NTC Beta=3950K)
const int32_t RES_22KOHM_100X[] = {1715000L, 1200000L, 850000L, 600000L, 430000L, 310000L, 225000L, 165000L, 122000L, 91000L, 68000L, 51000L, 38500L, 29200L, 22200L, 17000L, 13100L, 10100L, 7800L};

// Konverterar motstånd (Ohm * 100) till temperatur (°C * 100)
int16_t ResistanceToTemp_100x(int32_t resistance_100x) {
    const int32_t *res_table;
    uint8_t i;
    
    // Välj kurva baserat på globalt register
    if (registerMap[REG_NTC_CURVE_SELECT] == 0) { // 0 = 22 kOhm
        res_table = RES_22KOHM_100X;
    } else { // 1 = 150 Ohm (Default)
        res_table = RES_150OHM_100X;
    }

    // Leta upp motståndet i tabellen (tabellen är fallande)
    for (i = 0; i < sizeof(TEMP_INDEX) / sizeof(TEMP_INDEX[0]) - 1; i++) {
        if (resistance_100x >= res_table[i+1] && resistance_100x <= res_table[i]) {
            // Linjär interpolation mellan index i och i+1
            int16_t t1 = TEMP_INDEX[i];
            int16_t t2 = TEMP_INDEX[i+1];
            int32_t r1 = res_table[i];
            int32_t r2 = res_table[i+1];

            // Temp = T1 + (T2 - T1) * (R - R1) / (R2 - R1)
            int32_t diff_t = t2 - t1;
            int32_t diff_r = r2 - r1;
            int32_t res_diff = resistance_100x - r1;
            
            // Undvik division med noll om R1 = R2 (borde inte hända)
            if (diff_r == 0) return t1;
            
            return (int16_t)(t1 + (diff_t * res_diff) / diff_r);
        }
    }
    // Utanför intervall (-40.00C)
    if (resistance_100x > res_table[0]) return TEMP_INDEX[0];
    // Utanför intervall (+50.00C)
    if (resistance_100x < res_table[sizeof(TEMP_INDEX) / sizeof(TEMP_INDEX[0]) - 1]) return TEMP_INDEX[sizeof(TEMP_INDEX) / sizeof(TEMP_INDEX[0]) - 1];

    return 9999; // Felvärde
}

// Konverterar temperatur (°C * 100) till Wiper-värde (0-255)
// Använder omvänd interpolation för att hitta motsvarande motstånd
// Antar en 10kOhm Digipot (256 steg) i spänningsdelare mot 10kOhm (R_fix)
static uint8_t TempToWiper(int16_t temp_100x) {
    const int32_t R_FIX_OHM = 10000;
    int32_t target_res_100x;
    const int32_t *res_table;
    
    // Välj kurva baserat på globalt register
    if (registerMap[REG_NTC_CURVE_SELECT] == 0) { // 0 = 22 kOhm
        res_table = RES_22KOHM_100X;
    } else { // 1 = 150 Ohm (Default)
        res_table = RES_150OHM_100X;
    }

    // Använd interpolationslogiken från ResistanceToTemp_100x (med temp_100x som input)
    // Denna del är förenklad för att undvika dubbel interpolation och använder en direkt lookup:
    
    // Sök efter motståndet som matchar måltemperaturen (förenklat)
    target_res_100x = 15000L; // <-- Plachållare, kräver full omvänd interpolation

    // Antag att digipoten simulerar NTC-givaren direkt (R_ntc = R_pot).
    // R_pot = 10000 Ohm. Wiper_Value = R_target * 256 / 10000 Ohm
    
    // WIPER_VALUE = R_target_ohm * 256 / 10000
    // Eftersom R_target är *100, blir formeln:
    // WIPER_VALUE = (R_target_100x / 100) * 256 / 10000
    // WIPER_VALUE = (R_target_100x * 256) / 1000000
    
    // *Observera: Denna fullständiga omvända interpolation saknas fortfarande.*
    // För att få igång funktionen används en linjär approximation för 150 Ohm NTC (max 1000 Ohm @ -20C)
    // Wiper value = (10000 - R_target) / 10000 * 255.
    // Enkel Approximation:
    int32_t wiper_val = 128; // Standard 25C (150 Ohm)
    
    return (uint8_t)wiper_val;
}

void SPOOFER_Init(void) {
    SPI2_Init(); // Initiera HW SPI2
    
    RELAY_HEAT_PUMP_ON = 0;
    RELAY_PUMP_1_ON = 0;
    RELAY_PUMP_2_ON = 0;
    
    DP_SHDN_PIN = 0; // Shutdown Low (Normal drift)
    DP_CS_PIN = 1;   // CS High (Inaktiv)
    
    // Sätt initiala värden (50% resistans)
    SPI2_WriteWiper_Pot0(128);
    SPI2_WriteWiper_Pot1(128);
}

void SPOOFER_Process(void) {
    uint8_t wiper_out, wiper_in;
    int16_t target_out_temp, target_in_temp;
    
    // --- 1. Uppdatera Reläer ---
    uint8_t relay_state = registerMap[REG_RELAY_CONTROL];
    
    RELAY_HEAT_PUMP_ON = (relay_state & 0x01);
    RELAY_PUMP_1_ON    = (relay_state & 0x02) >> 1;
    RELAY_PUMP_2_ON    = (relay_state & 0x04) >> 2;
    
    // --- 2. Uppdatera Spoofing (Digipots) ---
    if (registerMap[REG_SPOOFING_ENABLED] == 1) {
        
        // Hämta måltemperaturer (16-bitars, * 100)
        target_out_temp = (registerMap[REG_TARGET_OUTDOOR_TEMP_HI] << 8) | registerMap[REG_TARGET_OUTDOOR_TEMP_LO];
        target_in_temp = (registerMap[REG_TARGET_INDOOR_TEMP_HI] << 8) | registerMap[REG_TARGET_INDOOR_TEMP_LO];

        // Konvertera temperatur till wiper-värde
        // *OBS: TempToWiper behöver fullständig implementering*
        wiper_out = TempToWiper(target_out_temp);
        wiper_in  = TempToWiper(target_in_temp);
        
        // Skriv till Digipots
        SPI2_WriteWiper_Pot1(wiper_out); // Utetemp (Pot 1)
        SPI2_WriteWiper_Pot0(wiper_in);  // Innetemp (Pot 0)
    } else {
        // Om Spoofing är av, sätt Digipots till max resistans för att 
        // minimera påverkan (eller sätt dem till ett säkert defaultvärde)
        // Eller, om hårdvaran tillåter: koppla bort Digipots helt
        // För nu: sätt till säkert default
        SPI2_WriteWiper_Pot1(128); 
        SPI2_WriteWiper_Pot0(128);
    }
}

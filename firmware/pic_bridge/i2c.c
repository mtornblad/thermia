#include "i2c.h"
#include "globals.h"
#include <xc.h>
#include <stdio.h> 

// Intern variabel för att hålla koll på vilket register master vill läsa/skriva till.
static volatile uint8_t register_index = 0;

// Tillståndsvariabel för att hantera den flerstegade Master Write-sekvensen
typedef enum {
    STATE_WAITING_FOR_INDEX = 0,
    STATE_WAITING_FOR_SUB_COMMAND = 1,
    STATE_WAITING_FOR_TARGET_ADDR = 2,
    STATE_WAITING_FOR_DATA_HI = 3,
    STATE_WAITING_FOR_DATA_LO = 4
} i2c_write_state_t;

static i2c_write_state_t i2c_write_state = STATE_WAITING_FOR_INDEX;

// Kommando-ID som Mastern skickar FÖRST (0xFE)
#define COMMAND_ID_START 0xFE
// Sub-kommando som Mastern skickar EFTER 0xFE (t.ex. 0x5D)
#define COMMAND_ID_SUB   0x5D 

void I2C_Init(void) {
    SSP1CON1bits.SSPEN = 0; 
    SSP1CON1bits.SSPM = 0b0110; // I2C Slave, 7-bitars Adress med Start/Stop-avbrott
    SSP1ADD = I2C_SLAVE_ADDR; 
    
    PIR1bits.SSP1IF = 0;   
    PIE1bits.SSP1IE = 1;   
    
    SSP1CON1bits.SSPEN = 1; 
}

/**
 * @brief Hanterar Master Write (Thermia Master skriver data till PIC:en/RegisterMap).
 * Denna funktion hanterar både standard loggning och specialkommandon (0xFE).
 */
static void handle_master_write(uint8_t received_data) {
    // Kontrollera I2C-status (REG_I2C_ENABLE_CONTROL == 0 disablar avbrott i ISR:en)
    
    // 1. Master skickar Register Index/Kommando (Adresseringsfas)
    if (!SSP1STATbits.D_nA) {
        
        // Återställ state-maskinen vid ny transaktion
        i2c_write_state = STATE_WAITING_FOR_INDEX; 

        // Check: Är det COMMAND_ID_START (0xFE)?
        if (received_data == COMMAND_ID_START) {
            i2c_write_state = STATE_WAITING_FOR_SUB_COMMAND;
            registerMap[REG_I2C_STATUS] = 0x03; // Debug: Entered CMD mode
            return; // Vänta på nästa byte i ISR
        }
        
        // Annars, standard Master Write (Loggning)
        register_index = received_data;
        i2c_write_state = STATE_WAITING_FOR_DATA_HI; // Gå till datafas
    } 
    // 2. Master skickar Data (Datafas, hanteras av tillståndsmaskinen)
    else {
        switch (i2c_write_state) {
            case STATE_WAITING_FOR_INDEX: 
                // Fallthrough till standardloggning nedan.
            
            case STATE_WAITING_FOR_DATA_HI:
                // Standard loggning: Logga data och inkrementera pekaren
                if (register_index < TOTAL_REGS) {
                    registerMap[register_index] = received_data;
                    register_index++;
                }
                break;

            // --- Kommando-logik (Specialprotokoll, t.ex. Rumstemp börvärde) ---
            
            case STATE_WAITING_FOR_SUB_COMMAND:
                // Expected: 0x5D
                if (received_data == COMMAND_ID_SUB) {
                    i2c_write_state = STATE_WAITING_FOR_TARGET_ADDR;
                    registerMap[REG_I2C_STATUS] = 0x04; // Debug: Got Sub CMD
                } else {
                    i2c_write_state = STATE_WAITING_FOR_INDEX; // Avbryt
                }
                break;
                
            case STATE_WAITING_FOR_TARGET_ADDR:
                // Nu kommer det register (0xB2/0x0F) som ska styras. Lagra det.
                registerMap[REG_TARGET_COMMAND_ADDR] = received_data;
                i2c_write_state = STATE_WAITING_FOR_DATA_HI;
                registerMap[REG_I2C_STATUS] = 0x05; // Debug: Got Target ADDR
                break;
                
            case STATE_WAITING_FOR_DATA_HI:
                // Skriv HI byte av värdet till XIAO-målregistret
                registerMap[REG_TARGET_COMMAND_VALUE_HI] = received_data;
                i2c_write_state = STATE_WAITING_FOR_DATA_LO;
                registerMap[REG_I2C_STATUS] = 0x06; // Debug: Got Data HI
                break;
                
            case STATE_WAITING_FOR_DATA_LO:
                // Skriv LO byte av värdet till XIAO-målregistret
                registerMap[REG_TARGET_COMMAND_VALUE_LO] = received_data;
                i2c_write_state = STATE_WAITING_FOR_INDEX; // Klart
                registerMap[REG_I2C_STATUS] = 0x07; // Debug: CMD Complete
                break;
        }
    }
    
    // Fortsätt klocka data / bekräfta mottagande (ACK)
    SSP1CON1bits.CKP = 1;
}


/**
 * @brief Hanterar Master Read (Thermia Master läser data från PIC:en/RegisterMap).
 * Använder en konfigurerbar Polling Hook för att simulera protokollhandskakning.
 */
static void handle_master_read(void) {
    uint8_t data_to_send;
    
    // Läs önskad Polling Address från XIAO:s kontrollregister (242)
    uint8_t polling_address = registerMap[REG_TARGET_COMMAND_ADDR];
    
    // 1. Polling Hook Check
    if (registerMap[REG_I2C_STATUS] != 0x00) { // Undvik overhead om inga kommandon väntar
        
        // Om Mastern läser från det register XIAO pekat ut som Polling Address (t.ex. 0xFE)...
        // ... OCH XIAO har laddat ett kommando i REG_I2C_COMMAND_RESPONSE (241)...
        if (register_index == polling_address && registerMap[REG_TARGET_COMMAND_VALUE_LO] != 0x00) {
            
            // Svara med den Kommando-byte som XIAO vill skicka
            data_to_send = registerMap[REG_TARGET_COMMAND_VALUE_LO]; // Använd LO-byten som kommando
            
            // Återställ kommandot efter att det skickats
            registerMap[REG_TARGET_COMMAND_VALUE_LO] = 0x00; 
            
            // Logga att Pollingen lyckades (för debug)
            registerMap[REG_I2C_STATUS] = 0x02; 
            
        } 
    }
    
    // 2. Standard Memory Mirror (eller fallback efter hook)
    data_to_send = registerMap[register_index];
    
    // Ladda data i bufferten
    SSP1BUF = data_to_send;
    
    // Inkrementera pekaren för nästa byte
    register_index++;
    if (register_index >= TOTAL_REGS) register_index = TOTAL_REGS - 1;
    
    // Fortsätt klocka ut data
    SSP1CON1bits.CKP = 1;
}

bool I2C_Slave_ISR_Handler(void) {
    // Kontrollera om I2C är aktiverat
    if (registerMap[REG_I2C_ENABLE_CONTROL] == 0) {
        return false; // I2C avstängd via Modbus
    }

    if (PIE1bits.SSP1IE && PIR1bits.SSP1IF) {
        
        SSP1CON1bits.WCOL = 0; 
        SSP1CON1bits.SSPOV = 0; 

        uint8_t buffer_data = SSP1BUF; 

        if (SSP1STATbits.R_nW) {
            // Master vill LÄSA (Slave Transmit)
            handle_master_read();

        } else {
            // Master vill SKRIVA (Slave Receive)
            handle_master_write(buffer_data);
        }

        PIR1bits.SSP1IF = 0;    
        
        return true;
    }
    return false;
}

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/gpio.h"
#include "esphome/components/logger/logger.h"
#include <string>

namespace esphome {
namespace pic_ota {

// PIC18F47Q43 har 14-bitars instruktioner och 6-bitars instruktionskod
#define PIC_DATA_BITS 14
#define PIC_INSTR_BITS 6

// ICSP-kommandon (Enligt Microchip Programmer's Specification)
#define ICSP_LOAD_DATA      0b000000
#define ICSP_READ_DATA      0b001000
#define ICSP_INC_ADDRESS    0b000010
#define ICSP_PROGRAM_WORD   0b100010

class PicOTA : public Component {
 public:
  void set_mclr_pin(GPIOPin *mclr) { mclr_pin_ = mclr; }
  void set_pgc_pin(GPIOPin *pgc) { pgc_pin_ = pgc; }
  void set_pgd_pin(GPIOPin *pgd) { pgd_pin_ = pgd; }
  void set_pic_firmware_file(std::string file) { pic_firmware_file_ = file; }

  void setup() override;
  void dump_config() override;

  // Funktion som anropas av loop() för att kontrollera PIC FW version
  void check_for_update(uint8_t current_major, uint8_t current_minor);

 protected:
  GPIOPin *mclr_pin_{}; 
  GPIOPin *pgc_pin_{};  
  GPIOPin *pgd_pin_{};  
  std::string pic_firmware_file_{"pic_firmware.hex"};

  // ICSP Bit-Banging & Programmeringsfunktioner (stubbar)
  void icsp_write(uint8_t instruction, uint16_t data);
  uint16_t icsp_read(uint8_t instruction);
  void icsp_set_programming_mode(bool enable);
  bool program_flash(const std::string& filename);
  uint16_t get_firmware_version_from_hex(const std::string& filename);
};

}  // namespace pic_ota
}  // namespace esphome

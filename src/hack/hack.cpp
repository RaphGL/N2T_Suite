#include "hack.hpp"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <sstream>
#include <string>
#include <vector>

std::uint16_t convert_input_to_hack(SDL_Keycode key) {
   switch (key) {
   case SDLK_SPACE:
      return 32;
   case SDLK_0:
      return 48;
   case SDLK_1:
      return 49;
   case SDLK_2:
      return 50;
   case SDLK_3:
      return 51;
   case SDLK_4:
      return 52;
   case SDLK_5:
      return 53;
   case SDLK_6:
      return 54;
   case SDLK_7:
      return 55;
   case SDLK_8:
      return 56;
   case SDLK_9:
      return 57;
   case SDLK_A:
      return 65;
   case SDLK_B:
      return 66;
   case SDLK_C:
      return 67;
   case SDLK_D:
      return 68;
   case SDLK_E:
      return 69;
   case SDLK_F:
      return 70;
   case SDLK_G:
      return 71;
   case SDLK_H:
      return 72;
   case SDLK_I:
      return 73;
   case SDLK_J:
      return 74;
   case SDLK_K:
      return 75;
   case SDLK_L:
      return 76;
   case SDLK_M:
      return 77;
   case SDLK_N:
      return 78;
   case SDLK_O:
      return 79;
   case SDLK_P:
      return 80;
   case SDLK_Q:
      return 81;
   case SDLK_R:
      return 82;
   case SDLK_S:
      return 83;
   case SDLK_T:
      return 84;
   case SDLK_U:
      return 85;
   case SDLK_V:
      return 86;
   case SDLK_W:
      return 87;
   case SDLK_X:
      return 88;
   case SDLK_Y:
      return 89;
   case SDLK_Z:
      return 90;

   case SDLK_RETURN:
      return 128;
   case SDLK_BACKSPACE:
      return 129;
   case SDLK_LEFT:
      return 130;
   case SDLK_UP:
      return 131;
   case SDLK_RIGHT:
      return 132;
   case SDLK_DOWN:
      return 133;
   case SDLK_HOME:
      return 134;
   case SDLK_END:
      return 135;
   case SDLK_PAGEUP:
      return 136;
   case SDLK_PAGEDOWN:
      return 137;
   case SDLK_INSERT:
      return 138;
   case SDLK_DELETE:
      return 139;
   case SDLK_ESCAPE:
      return 140;
   case SDLK_F1:
      return 141;
   case SDLK_F2:
      return 142;
   case SDLK_F3:
      return 143;
   case SDLK_F4:
      return 144;
   case SDLK_F5:
      return 145;
   case SDLK_F6:
      return 146;
   case SDLK_F7:
      return 147;
   case SDLK_F8:
      return 148;
   case SDLK_F9:
      return 149;
   case SDLK_F10:
      return 150;
   case SDLK_F11:
      return 151;
   case SDLK_F12:
      return 152;

   default:
      return 0;
   }
}

static void panic_on_invalid_instruction(std::uint16_t pc, std::uint16_t instruction) {
   auto inst_str = std::to_string(instruction);
   throw std::format("Invalid instruction reached at pc = {} with value: `{}`", pc, inst_str);
}

bool Hack::load_rom(std::vector<uint16_t> &instructions) {
   if (instructions.size() > instruction_mem.size()) {
      return false;
   }

   std::copy(instructions.begin(), instructions.end(), instruction_mem.begin());
   return true;
}

// retrieves a span of the memory mapped screen buffer
ScreenSpan Hack::get_screen_mmap() {
   ScreenSpan mmap { &data_mem.at(16384), 8192 };
   return mmap;
}

bool Hack::load_rom(std::string_view instructions) {
   std::vector<std::uint16_t> bin_insts { };
   std::stringstream inststream { std::string(instructions) };

   std::string tmp { };
   while (std::getline(inststream, tmp)) {
      try {
         auto inst = std::stoull(tmp, nullptr, 2);
         bin_insts.push_back(inst);
      } catch (...) {
         return false;
      }
   }

   return this->load_rom(bin_insts);
}

std::uint16_t &Hack::get_keyboard_mmap() { return data_mem.at(24576); }

// Throws an exception if an invalid instruction is ever reached
void Hack::tick() {
   auto inst = instruction_mem.at(pc);
   ++pc;

   bool is_a_instruction = (inst & (1 << 15)) == 0;
   std::uint16_t a_inst_mask = 0b0111111111111111;
   std::uint16_t c_inst_mask = 0b0001111111111111;

   if (is_a_instruction) {
      address_reg = inst & a_inst_mask;
      return;
   }

   inst &= c_inst_mask;
   std::uint16_t a = (inst >> 12) & 1;
   std::uint16_t comp = (inst >> 6) & 0b111111;
   std::uint16_t dest = (inst >> 3) & 0b111;
   std::uint16_t jump = inst & 0b111;

   std::uint16_t comp_result = 0;
   switch (comp) {
      // 0
   case 0b101010:
      if (!a) {
         comp_result = 0;
      } else {
         panic_on_invalid_instruction(pc, inst);
      }
      break;

      // 1
   case 0b111111:
      if (!a) {
         comp_result = 1;
      } else {
         panic_on_invalid_instruction(pc, inst);
      }
      break;

      // -1
   case 0b111010:
      if (!a) {
         comp_result = -1;
      } else {
         panic_on_invalid_instruction(pc, inst);
      }
      break;

      // D
   case 0b001100:
      if (!a) {
         comp_result = data_reg;
      } else {
         panic_on_invalid_instruction(pc, inst);
      }
      break;

      // A or M
   case 0b110000:
      if (a) {
         comp_result = data_mem.at(address_reg);
      } else {
         comp_result = address_reg;
      }
      break;

   // !D
   case 0b001101:
      if (!a) {
         comp_result = ~data_reg;
      } else {
         panic_on_invalid_instruction(pc, inst);
      }
      break;

   // !A or !M
   case 0b110001:
      if (a) {
         comp_result = ~data_mem.at(address_reg);
      } else {
         comp_result = ~address_reg;
      }
      break;

   // -D
   case 0b001111:
      if (!a) {
         comp_result = -data_reg;
      } else {
         panic_on_invalid_instruction(pc, inst);
      }
      break;

   // -A or -M
   case 0b110011:
      if (a) {
         comp_result = -data_mem.at(address_reg);
      } else {
         comp_result = -address_reg;
      }
      break;

   // D+1
   case 0b011111:
      if (!a) {
         comp_result = data_reg + 1;
      } else {
         panic_on_invalid_instruction(pc, inst);
      }
      break;

   // A+1 or M+1
   case 0b110111:
      if (a) {
         comp_result = data_mem.at(address_reg) + 1;
      } else {
         comp_result = address_reg + 1;
      }
      break;

   // D-1
   case 0b001110:
      if (!a) {
         comp_result = data_reg - 1;
      } else {
         panic_on_invalid_instruction(pc, inst);
      }
      break;

   // A-1 or M-1
   case 0b110010:
      if (a) {
         comp_result = data_mem.at(address_reg) - 1;
      } else {
         comp_result = address_reg - 1;
      }
      break;

   // D+A or D+M
   case 0b000010:
      if (a) {
         comp_result = data_reg + data_mem.at(address_reg);
      } else {
         comp_result = data_reg + address_reg;
      }
      break;

   // D-A or D-M
   case 0b010011:
      if (a) {
         comp_result = data_reg - data_mem.at(address_reg);
      } else {
         comp_result = data_reg - address_reg;
      }
      break;

   // A-D or M-D
   case 0b000111:
      if (a) {
         comp_result = data_mem.at(address_reg) - data_reg;
      } else {
         comp_result = address_reg - data_reg;
      }
      break;

   // D&A or D&M
   case 0b000000:
      if (a) {
         comp_result = data_reg & data_mem.at(address_reg);
      } else {
         comp_result = data_reg & address_reg;
      }
      break;

   // D|A or D|M
   case 0b010101:
      if (a) {
         comp_result = data_reg | data_mem.at(address_reg);
      } else {
         comp_result = data_reg | address_reg;
      }
      break;

   default:
      panic_on_invalid_instruction(pc, inst);
      break;
   }

   switch (dest) {
   // NULL
   case 0b000:
      break;

   // M
   case 0b001:
      data_mem.at(address_reg) = comp_result;
      break;

   // D
   case 0b010:
      data_reg = comp_result;
      break;

   // MD
   case 0b011:
      data_mem.at(address_reg) = comp_result;
      data_reg = comp_result;
      break;

   // A
   case 0b100:
      address_reg = comp_result;
      break;

   // AM
   case 0b101:
      data_mem.at(address_reg) = comp_result;
      address_reg = comp_result;
      break;

   // AD
   case 0b110:
      address_reg = comp_result;
      data_reg = comp_result;
      break;

   // AMD
   case 0b111:
      data_mem.at(address_reg) = comp_result;
      address_reg = comp_result;
      data_reg = comp_result;
      break;
   }

   // the negative bit is on the penultimate bit because the last bit
   // is used to indicate whether a 16-bit number is an A-instruction
   std::uint16_t is_negative = comp_result & (1 << 15);
   switch (jump) {
   // NULL
   case 0b000:
      break;

   // JEQ
   case 0b010:
      if (comp_result == 0) {
         pc = address_reg;
      }
      break;

   // JGT
   case 0b001:
      if (comp_result != 0 && !is_negative) {
         pc = address_reg;
      }
      break;

   // JGE
   case 0b011:
      if (comp_result == 0 || !is_negative) {
         pc = address_reg;
      }
      break;

   // JLT
   case 0b100:
      if (comp_result != 0 && is_negative) {
         pc = address_reg;
      }
      break;

   // JNE
   case 0b101:
      if (comp_result != 0) {
         pc = address_reg;
      }
      break;

   // JLE
   case 0b110:
      if (comp_result == 0 || is_negative) {
         pc = address_reg;
      }
      break;

   // JMP
   case 0b111:
      pc = address_reg;
      break;
   }
}

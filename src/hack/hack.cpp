#include "hack.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <vector>

static void panic_on_invalid_instruction(std::uint16_t instruction) {
  auto inst_str = std::to_string(instruction);
  throw std::format("Invalid instruction reached with value: `{}`", inst_str);
}

bool Hack::load_rom(std::vector<uint16_t> &instructions) {
  if (instructions.size() > instruction_mem.size()) {
    return false;
  }

  std::copy(instructions.begin(), instructions.end(), instruction_mem.begin());
  return true;
}

// retrieves a span of the memory mapped screen buffer
std::span<std::uint16_t, 8192> Hack::get_screen_mmap() {
  std::span<std::uint16_t, 8192> mmap{&data_mem.at(0x4000),
                                      &data_mem.at(0x5FFF)};
  return mmap;
}

std::uint16_t &Hack::get_keyboard_mmap() { return data_mem.at(0x6000); }

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
    bool a = (inst >> 12) & 1;
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
        panic_on_invalid_instruction(inst);
      }
      break;

      // 1
    case 0b111111:
      if (!a) {
        comp_result = 1;
      } else {
        panic_on_invalid_instruction(inst);
      }
      break;

      // -1
    case 0b111010:
      if (!a) {
        comp_result = -1;
      } else {
        panic_on_invalid_instruction(inst);
      }
      break;

      // D
    case 0b001100:
      if (!a) {
        comp_result = data_reg;
      } else {
        panic_on_invalid_instruction(inst);
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
        comp_result = !data_reg;
      } else {
        panic_on_invalid_instruction(inst);
      }
      break;

    // !A or !M
    case 0b110001:
      if (a) {
        comp_result = !data_mem.at(address_reg);
      } else {
        comp_result = !address_reg;
      }
      break;

    // -D
    case 0b001111:
      if (!a) {
        comp_result = -data_reg;
      } else {
        panic_on_invalid_instruction(inst);
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
        panic_on_invalid_instruction(inst);
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
      if (a) {
        comp_result = data_reg - 1;
      } else {
        panic_on_invalid_instruction(inst);
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
      panic_on_invalid_instruction(inst);
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
      address_reg = comp_result;
      data_mem.at(address_reg) = comp_result;
      break;

    // AD
    case 0b110:
      address_reg = comp_result;
      data_reg = comp_result;
      break;

    // AMD
    case 0b111:
      address_reg = comp_result;
      data_mem.at(address_reg) = comp_result;
      data_reg = comp_result;
      break;
    }

    switch (jump) {
    // NULL
    case 0b000:
      break;

    // JGT
    case 0b001:
      if (comp_result > 0) {
        pc = address_reg;
      }
      break;

    // JEQ
    case 0b010:
      if (comp_result == 0) {
        pc = address_reg;
      }
      break;

    // JGE
    case 0b11:
      if (comp_result >= 0) {
        pc = address_reg;
      }
      break;

    // JLT
    case 0b100:
      if (comp_result < 0) {
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
      if (comp_result <= 0) {
        pc = address_reg;
      }
      break;

    // JMP
    case 0b111:
      pc = address_reg;
      break;
    }
}

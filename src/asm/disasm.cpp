#include <cstdint>
#include <format>
#include <optional>
#include <sstream>
#include <string>

namespace assembly {

std::optional<std::string> disassemble(std::uint16_t instruction) {

   constexpr std::uint16_t cinstr_flag = 0b111 << 13;
   constexpr std::uint16_t ainst_flag = 1 << 15;

   if ((instruction & cinstr_flag) == cinstr_flag) {
      std::uint16_t a = (instruction >> 12) & 1;
      std::uint16_t comp = (instruction >> 6) & 0b111111;
      std::uint16_t dest = (instruction >> 3) & 0b111;
      std::uint16_t jump = instruction & 0b111;

      std::string inst_str;
      if (dest) {
         switch (dest) {
         case 0b001:
            inst_str += 'M';
            break;
         case 0b010:
            inst_str += 'D';
            break;
         case 0b011:
            inst_str += "MD";
            break;
         case 0b100:
            inst_str += 'A';
            break;
         case 0b101:
            inst_str += "AM";
            break;
         case 0b110:
            inst_str += "AD";
            break;
         case 0b111:
            inst_str += "AMD";
            break;
         }
      }

      std::string comp_str;
      bool valid_comp = true;
      switch (comp) {
      case 0b101010:
         comp_str += '0';
         break;
      case 0b111111:
         comp_str += '1';
         break;
      case 0b111010:
         comp_str += "-1";
         break;
      case 0b001100:
         comp_str += 'D';
         break;
      case 0b110000:
         comp_str += a ? 'M' : 'A';
         break;
      case 0b001101:
         comp_str += "!D";
         break;
      case 0b110001:
         comp_str += a ? "!M" : "!A";
         break;
      case 0b001111:
         comp_str += "-D";
         break;
      case 0b110011:
         comp_str += a ? "-M" : "-A";
         break;
      case 0b011111:
         comp_str += "D+1";
         break;
      case 0b110111:
         comp_str += a ? "M+1" : "A+1";
         break;
      case 0b001110:
         comp_str += "D-1";
         break;
      case 0b110010:
         comp_str += a ? "M-1" : "A-1";
         break;
      case 0b000010:
         comp_str += a ? "D+M" : "D+A";
         break;
      case 0b010011:
         comp_str += a ? "D-M" : "D-A";
         break;
      case 0b000111:
         comp_str += a ? "M-D" : "A-D";
         break;
      case 0b000000:
         comp_str += a ? "D&M" : "D&A";
         break;
      case 0b010101:
         comp_str += a ? "D|M" : "D|A";
         break;
      default:
         valid_comp = false;
         break;
      }

      if (valid_comp) {
         if (dest) {
            inst_str += '=';
         }
         inst_str += comp_str;
      }

      if (jump) {
         inst_str += ';';
         switch (jump) {
         case 0b001:
            inst_str += "JGT";
            break;
         case 0b010:
            inst_str += "JEQ";
            break;
         case 0b011:
            inst_str += "JGE";
            break;
         case 0b100:
            inst_str += "JLT";
            break;
         case 0b101:
            inst_str += "JNE";
            break;
         case 0b110:
            inst_str += "JLE";
            break;
         case 0b111:
            inst_str += "JMP";
            break;
         }
      }
      return inst_str;
   }

   if ((instruction & ainst_flag) == 0) {
      std::uint16_t label = 0b0111111111111111 & instruction;
      return std::format("@{}", label);
   }

   return std::nullopt;
}

std::optional<std::string> disassemble(std::string_view instructions) {
   std::stringstream inststream { instructions.data() };
   std::string asm_str;

   std::string tmp { };
   while (std::getline(inststream, tmp)) {
      try {
         auto inst = std::stoull(tmp, nullptr, 2);
         auto inst_str = disassemble(inst);
         if (!inst_str.has_value()) {
            return std::nullopt;
         }
         asm_str += inst_str.value();
         asm_str += '\n';
      } catch (...) {
         return std::nullopt;
      }
   }

   return asm_str;
}
};

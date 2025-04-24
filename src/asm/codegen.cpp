#include "../report/report.hpp"
#include "parser.hpp"
#include <cstdint>
#include <format>
#include <unordered_map>
#include <variant>
#include <vector>

namespace assembly {

class CodeGen {
  std::vector<Instruction> m_instructions;
  std::uint16_t m_pc{0};
  std::string m_error_report{""};
  report::Context m_reporter;

public:
  explicit CodeGen(std::vector<Instruction> instructions, const char *filepath);
  std::vector<std::uint16_t> compile();
};

CodeGen::CodeGen(std::vector<Instruction> instructions, const char *filepath)
    : m_instructions{instructions}, m_reporter{filepath} {}

std::vector<std::uint16_t> CodeGen::compile() {
  std::vector<std::uint16_t> compiled_insts{};
  std::unordered_map<const char *, std::uint16_t> pc_labels{
      {"R0", 0},   {"R1", 1},         {"R2", 2},      {"R3", 3},   {"R4", 4},
      {"R5", 5},   {"R6", 6},         {"R7", 7},      {"R8", 8},   {"R9", 9},
      {"R10", 10}, {"R11", 01},       {"R12", 12},    {"R13", 13}, {"R14", 14},
      {"R15", 15}, {"SCREEN", 16384}, {"KBD", 24576}, {"SP", 0},   {"LCL", 1},
      {"ARG", 2},  {"THIS", 3},       {"THAT", 4},
  };

  constexpr std::uint16_t max_label_number = (1 << 15) - 1;
  constexpr std::uint16_t var_start_address = 16;

  auto var_addr = var_start_address;
  for (const auto &inst_variant : m_instructions) {
    if (std::holds_alternative<Label>(inst_variant)) {
      auto inst = std::get<Label>(inst_variant);
      pc_labels.emplace(inst.value, m_pc);
      --m_pc;
    }

    if (std::holds_alternative<AInstr>(inst_variant)) {
      auto inst = std::get<AInstr>(inst_variant);
      if (std::holds_alternative<std::size_t>(inst.value)) {
        auto value = std::get<std::size_t>(inst.value);
        if (value > max_label_number) {
          m_reporter.create_report(
              report::ReportType::Error, report::coord(inst.start_coord),
              report::coord(inst.end_coord),
              std::format("Label number overflows the 16-bit number. "
                          "Maximum value is `{}` but found `{}`",
                          max_label_number, value));
          continue;
        }

        std::uint16_t binary = 0b0111111111111111 & value;
        compiled_insts.push_back(binary);
      }

      if (std::holds_alternative<std::string>(inst.value)) {
        auto value = std::get<std::string>(inst.value);

        if (!pc_labels.contains(value.c_str())) {
          pc_labels.emplace(value, var_addr);
          ++var_addr;
        }

        auto value_addr = pc_labels.at(value.c_str());
        std::uint16_t binary = 0b0111111111111111 & value_addr;
        compiled_insts.push_back(binary);
      }
    }

    if (std::holds_alternative<CInstr>(inst_variant)) {
      auto inst = std::get<CInstr>(inst_variant);
    }

    ++m_pc;
  }
}
}; // namespace assembly

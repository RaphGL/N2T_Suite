#include "codegen.hpp"
#include "../report/report.hpp"
#include "parser.hpp"
#include <cmath>
#include <cstdint>
#include <format>
#include <unordered_map>
#include <variant>
#include <vector>

namespace assembly {

CodeGen::CodeGen(std::vector<Instruction> instructions, const char *filepath)
    : m_instructions{instructions}, m_reporter{filepath} {}

std::string CodeGen::get_error_report() { return m_error_report; }

inline void CodeGen::emit_error(TokenCoordinate start, TokenCoordinate end,
                                std::string_view error_msg) {
  m_reporter.create_report(report::ReportType::Error, report::coord(start),
                           report::coord(end), error_msg);
}

std::optional<std::uint16_t> CodeGen::compile_cinstr_comp(CInstr ctx) {
  constexpr std::uint16_t comp_mask = 0b0001111111000000;
  const auto comp = ctx.comp;

  std::uint16_t inst = 0;

  if (std::holds_alternative<UnaryComp>(comp)) {
    auto unary = std::get<UnaryComp>(comp);
    switch (unary.op) {
    case Operator::None:
      if (std::holds_alternative<std::size_t>(unary.operand)) {
        auto digit = std::get<std::size_t>(unary.operand);
        if (digit) {
          inst = 0b0111111;
        } else {
          inst = 0b0101010;
        }
      }

      if (std::holds_alternative<Address>(unary.operand)) {
        auto addr = std::get<Address>(unary.operand);
        switch (addr) {
        case Address::D:
          inst = 0b0001100;
          break;
        case Address::A:
          inst = 0b0110000;
          break;
        case Address::M:
          inst = 0b1110000;
          break;
        default:
          this->emit_error(ctx.start, ctx.end,
                           "Invalid address. Expected A, D or M.");
          return std::nullopt;
          break;
        }
      }
      break;

    case Operator::Neg:
      if (std::holds_alternative<std::size_t>(unary.operand)) {
        auto digit = std::get<std::size_t>(unary.operand);
        if (digit) {
          inst = 0b0111010;
        } else {
          this->emit_error(
              ctx.start, ctx.end,
              "`0` cannot be negated. No such instruction exists.");
          return std::nullopt;
        }
      }

      if (std::holds_alternative<Address>(unary.operand)) {
        auto addr = std::get<Address>(unary.operand);
        switch (addr) {
        case Address::D:
          inst = 0b0001111;
          break;
        case Address::A:
          inst = 0b0110011;
          break;
        case Address::M:
          inst = 0b1110011;
          break;
        default:
          this->emit_error(ctx.start, ctx.end,
                           "Invalid address. Address cannot be negated. "
                           "Expected D, A or M.");
          return std::nullopt;
          break;
        }
      }
      break;

    case Operator::Not:
      if (std::holds_alternative<std::size_t>(unary.operand)) {
        this->emit_error(ctx.start, ctx.end,
                         "Notting a binary number is an invalid operation");
        return std::nullopt;
      }

      if (std::holds_alternative<Address>(unary.operand)) {
        auto addr = std::get<Address>(unary.operand);
        switch (addr) {
        case Address::D:
          inst = 0b0001111;
          break;
        case Address::A:
          inst = 0b0110011;
          break;
        case Address::M:
          inst = 0b1110011;
          break;
        default:
          this->emit_error(ctx.start, ctx.end,
                           "Invalid address. Expected D, A or M.");
          return std::nullopt;
          break;
        }
      }
      break;

    default:
      this->emit_error(ctx.start, ctx.end,
                       "Invalid operator. Expected `|`, `&`, `!`, `+` or `-`.");
      return std::nullopt;
      break;
    }
  }

  if (std::holds_alternative<BinaryComp>(comp)) {
    auto binary = std::get<BinaryComp>(comp);
    switch (binary.op) {
    case Operator::Sub:
      [[fallthrough]];
    case Operator::Add:
      if (std::holds_alternative<std::size_t>(binary.right)) {
        auto digit = std::get<std::size_t>(binary.right);
        if (!digit) {
          this->emit_error(ctx.start, ctx.end,
                           std::format("Expected `1` but found `0`", digit));
          return std::nullopt;
        }

        switch (binary.left) {
        case Address::D:
          inst = binary.op == Operator::Add ? 0b0011111 : 0b0001110;
          break;
        case Address::A:
          inst = binary.op == Operator::Add ? 0b0110111 : 0b0110010;
          break;
        case Address::M:
          inst = binary.op == Operator::Add ? 0b1110111 : 0b1110010;
          break;
        default:
          this->emit_error(ctx.start, ctx.end,
                           "Invalid Address. Expected A, D or M.");
          return std::nullopt;
          break;
        }
      }

      if (std::holds_alternative<Address>(binary.right)) {
        auto addr = std::get<Address>(binary.right);
        if (binary.left == Address::D) {
          switch (addr) {
          case Address::A:
            inst = binary.op == Operator::Add ? 0b0000010 : 0b0010011;
            break;
          case Address::M:
            inst = binary.op == Operator::Add ? 0b1000010 : 0b1010011;
            break;
          default:
            this->emit_error(ctx.start, ctx.end,
                             "Invalid address. Expected A or M");
            return std::nullopt;
            break;
          }
        } else if (addr == Address::D && binary.op == Operator::Sub) {
          switch (binary.left) {
          case Address::A:
            inst = 0b0000111;
            break;
          case Address::M:
            inst = 0b1000111;
            break;
          default:
            this->emit_error(ctx.start, ctx.end,
                             "Invalid address. Expected A or M");
            return std::nullopt;
            break;
          }
        } else {
          this->emit_error(ctx.start, ctx.end,
                           "Invalid address. Expected A or M");
          return std::nullopt;
        }
      }
      break;

    case Operator::Or:
      [[fallthrough]];
    case Operator::And:
      if (binary.left != Address::D) {
        this->emit_error(ctx.start, ctx.end,
                         "Invalid address. Left-hand side has to be D.");
        return std::nullopt;
      }

      if (!std::holds_alternative<Address>(binary.right)) {
        // emit error: and can only be used with addresses
        auto addr = std::get<Address>(binary.right);
        switch (addr) {
        case Address::A:
          inst = binary.op == Operator::And ? 0b0000000 : 0b0010101;
          break;
        case Address::M:
          inst = binary.op == Operator::And ? 0b1000000 : 0b1010101;
          break;
        default:
          this->emit_error(ctx.start, ctx.end,
                           "Invalid address. Expected A or M");
          return std::nullopt;
          break;
        }
      } else {
        this->emit_error(ctx.start, ctx.end,
                         "Right-hand has to be either A or M address.");
        return std::nullopt;
      }

      break;

    default:
      this->emit_error(ctx.start, ctx.end,
                       "Invalid operator. Expected `|`, `&`, `!`, `+` or `-`.");
      return std::nullopt;
      break;
    }
  }

  return (inst << 6) & comp_mask;
}

std::uint16_t CodeGen::compile_cinstr_dest(CInstr ctx) const noexcept {
  constexpr std::uint16_t dest_mask = 0b0000000000111000;
  const auto dest = ctx.dest;

  std::uint16_t inst = 0;
  switch (dest) {
  case Destination::None:
    break;
  case Destination::M:
    inst = 0b001;
    break;
  case Destination::D:
    inst = 0b010;
    break;
  case Destination::MD:
    inst = 0b011;
    break;
  case Destination::A:
    inst = 0b100;
    break;
  case Destination::AM:
    inst = 0b101;
    break;
  case Destination::AD:
    inst = 0b110;
    break;
  case Destination::AMD:
    inst = 0b111;
    break;
  }

  return (inst << 3) & dest_mask;
}

std::uint16_t CodeGen::compile_cinstr_jump(CInstr ctx) const noexcept {
  std::uint16_t inst = 0;
  const auto jump = ctx.jump;

  switch (jump) {
  case Jump::None:
    break;
  case Jump::JGT:
    inst = 0b001;
    break;
  case Jump::JEQ:
    inst = 0b010;
    break;
  case Jump::JGE:
    inst = 0b011;
    break;
  case Jump::JLT:
    inst = 0b100;
    break;
  case Jump::JNE:
    inst = 0b101;
    break;
  case Jump::JLE:
    inst = 0b110;
    break;
  case Jump::JMP:
    inst = 0b111;
    break;
  }

  return inst;
}

std::optional<std::vector<std::uint16_t>> CodeGen::compile() {
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
      pc_labels.emplace(inst.value.c_str(), m_pc);
      --m_pc;
    }

    if (std::holds_alternative<AInstr>(inst_variant)) {
      auto inst = std::get<AInstr>(inst_variant);
      if (std::holds_alternative<std::size_t>(inst.value)) {
        auto value = std::get<std::size_t>(inst.value);
        if (value > max_label_number) {
          this->emit_error(
              inst.start_coord, inst.end_coord,
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
          pc_labels.emplace(value.c_str(), var_addr);
          ++var_addr;
        }

        auto value_addr = pc_labels.at(value.c_str());
        std::uint16_t binary = 0b0111111111111111 & value_addr;
        compiled_insts.push_back(binary);
      }
    }

    if (std::holds_alternative<CInstr>(inst_variant)) {
      auto inst = std::get<CInstr>(inst_variant);
      if (auto comp_opt = this->compile_cinstr_comp(inst);
          comp_opt.has_value()) {
        auto comp = comp_opt.value();
        std::uint16_t binary = 0b1110000000000000 | comp |
                               this->compile_cinstr_dest(inst) |
                               this->compile_cinstr_jump(inst);
        compiled_insts.push_back(binary);
      } else {
        continue;
      }
    }

    ++m_pc;
  }

  auto error_report = m_reporter.generate_final_report();
  if (error_report.has_value()) {
    m_error_report = error_report.value();
    return std::nullopt;
  }

  return compiled_insts;
}

}; // namespace assembly

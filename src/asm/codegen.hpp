#ifndef ASM_CODEGEN_HPP
#define ASM_CODEGEN_HPP

#include "../report/report.hpp"
#include "parser.hpp"
#include <cmath>
#include <cstdint>
#include <variant>
#include <vector>

namespace assembly {

class CodeGen {
  std::vector<Instruction> m_instructions;
  std::uint16_t m_pc{0};
  std::string m_error_report{""};
  report::Context m_reporter;

  std::uint16_t compile_cinstr_dest(Destination dest) const noexcept;
  std::uint16_t compile_cinstr_comp(std::variant<UnaryComp, BinaryComp> comp);
  std::uint16_t compile_cinstr_jump(Jump jump) const noexcept;

public:
  explicit CodeGen(std::vector<Instruction> instructions, const char *filepath);
  std::vector<std::uint16_t> compile();
};

}; // namespace assembly

#endif

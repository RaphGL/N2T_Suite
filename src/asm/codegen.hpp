#ifndef ASM_CODEGEN_HPP
#define ASM_CODEGEN_HPP

#include "../report/report.hpp"
#include "lexer.hpp"
#include "parser.hpp"
#include <cmath>
#include <cstdint>
#include <vector>

namespace assembly {

class CodeGen {
  std::vector<Instruction> m_instructions;
  std::uint16_t m_pc{0};
  std::string m_error_report{""};
  report::Context m_reporter;

  std::uint16_t compile_cinstr_dest(CInstr ctx) const noexcept;
  std::optional<std::uint16_t> compile_cinstr_comp(CInstr ctx);
  std::uint16_t compile_cinstr_jump(CInstr ctx) const noexcept;
  inline void emit_error(TokenCoordinate start, TokenCoordinate end,
                         std::string_view error_msg);

public:
  explicit CodeGen(std::vector<Instruction> instructions, const char *filepath);
  std::optional<std::vector<std::uint16_t>> compile();
  std::string get_error_report();
};

std::string to_string(std::vector<std::uint16_t> asm_instructions);

}; // namespace assembly

#endif

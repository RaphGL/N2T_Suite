#include "asm/codegen.hpp"
#include "asm/lexer.hpp"
#include "asm/parser.hpp"
#include "hdl/lexer.hpp"
#include "hdl/parser.hpp"
#include <iostream>
#include <variant>

int main() {
  // hdl::Lexer tk{"test.hdl"};
  // auto tokens = tk.tokenize();

  // hdl::Parser parser{tokens, "test.hdl"};
  // auto ast = parser.parse();
  // if (!ast.has_value()) {
  //   std::cerr << parser.get_error_report();
  //   return 1;
  // }

  // auto chips = ast.value();
  // std::cout << chips[0].parts[2].name;

  assembly::Lexer lex{"test.asm"};
  auto tokens = lex.tokenize();
  assembly::Parser parser{tokens, "test.asm"};
  auto insts_opt = parser.parse();

  if (!insts_opt.has_value()) {
    std::cout << parser.get_error_report();
    return 1;
  }

  auto instructions = insts_opt.value();
  assembly::CodeGen codegen{instructions, "test.asm"};
  auto asm_output = codegen.compile();
  if (!asm_output.has_value()) {
    std::cout << codegen.get_error_report();
    return 1;
  }

  auto output = asm_output.value();
  std::cout << "success?" << '\n';

  std::cout << assembly::to_string(output) << '\n';
}

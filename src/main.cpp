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
  auto instructions = parser.parse();

  std::cout << parser.get_error_report();
}

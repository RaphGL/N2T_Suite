#include "hdl/lexer.hpp"
#include "hdl/parser.hpp"
#include <iostream>

int main() {
  hdl::Lexer tk{"test.hdl"};
  auto tokens = tk.tokenize();
  for (auto &token : tokens) {
    std::cout << token.string() << '\n';
  }

  hdl::Parser parser{tokens, "test.hdl"};
  auto ast = parser.parse();
  if (!ast.has_value()) {
    std::cerr << parser.get_error_report();
    return 1;
  }

  hdl::print_ast(ast.value());
}

#include "asm/lexer.hpp"
#include "hdl/lexer.hpp"
#include "hdl/parser.hpp"
#include <iostream>

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

  assembly::Lexer lex{"test.asm"};
  auto tokens = lex.tokenize();
  std::cout << "starting tokenizer\n";

  for (const auto &tok : tokens) {
    std::cout << tok.start_coord.col << ':' << tok.start_coord.row << ' ';
    if (tok.type == assembly::TokenType::Number) {
      std::cout << "number: " << std::get<std::size_t>(tok.value);
    } else if (tok.type == assembly::TokenType::Label) {
      std::cout << "label " << std::get<std::string>(tok.value);
    } else if (tok.type == assembly::TokenType::Unknown) {
      std::cout << "unknown: " << (char)std::get<int>(tok.value);
    } else {
      std::cout << "char: " << (char)std::get<int>(tok.value);
    }

    std::cout << '\n';
  }
}

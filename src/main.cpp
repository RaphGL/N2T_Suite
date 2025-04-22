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

  std::cout << instructions.value().size() << '\n';

  for (auto inst : instructions.value()) {
    if (std::holds_alternative<assembly::AInstr>(inst)) {
      auto i =  std::get<assembly::AInstr>(inst).value;

      if (std::holds_alternative<std::size_t>(i)) {
        std::cout << "A: " << std::get<std::size_t>(i) << '\n';
      }
      else if (std::holds_alternative<std::string>(i)) {
        std::cout << "A: " << std::get<std::string>(i) << '\n';
      }
    }

    if (std::holds_alternative<assembly::Label>(inst)) {
      auto i = std::get<assembly::Label>(inst);
      std::cout << "L: " << i.value << '\n';
    }
  }
}

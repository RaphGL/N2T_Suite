#include "lexer.hpp"
#include <iostream>

int main() {
  hdl::Lexer tk{"test.hdl"};
  auto tokens = tk.tokenize();
  for (auto &token : tokens) {
    std::cout << token.string() << '\n';
  }
}

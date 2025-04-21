#ifndef ASM_LEXER_HPP
#define ASM_LEXER_HPP

#include <fstream>
#include <variant>
#include <vector>

namespace assembly {
  
enum class TokenType {
  Unknown,
  // [0-9].*
  Number,
  // [A-z]|[0-9].*
  Label,
  D,
  A,
  M,
  Exclamation,
  Minus,
  Plus,
  Pipe,
  Ampersand,
  AtSymbol,
  Semicolon,
  OpenParen,
  CloseParen,
  Equal,
};

struct TokenCoordinate {
  std::size_t row, col;
};

struct Token {
  TokenType type;
  TokenCoordinate start_coord, end_coord;
  std::variant<int, std::size_t, std::string> value;
};

class Lexer final {
  std::ifstream m_asm_file;
  std::vector<Token> m_tokens{};
  std::size_t m_curr_x{0}, m_curr_y{0};

  Token lex_number();
  Token lex_label();
  void ignore_comment();

public:
  explicit Lexer(const char *file_path) : m_asm_file{file_path} {}
  std::vector<Token> tokenize();
};

};

#endif

#ifndef HDL_LEXER_H
#define HDL_LEXER_H

#include <fstream>
#include <variant>
#include <vector>

namespace hdl {

struct TokenCoordinate {
  std::size_t row, col;
};

enum class TokenType {
  Ident,
  Equal,
  Comma,
  Colon,
  Semicolon,
  Number,
  RangeOp, // `..`
  OpenBrace,
  CloseBrace,
  OpenParen,
  CloseParen,
  OpenBracket,
  CloseBracket,
};

struct Token {
  TokenType type;
  TokenCoordinate start_coord, end_coord;

  std::variant<std::string, std::size_t, int> value;

  // creates a Number token
  explicit Token(std::size_t n, TokenCoordinate start, TokenCoordinate end);

  // creates symbol tokens of specified type
  explicit Token(int s, TokenType tt, TokenCoordinate start,
                 TokenCoordinate end);

  explicit Token(std::string id, TokenType tt, TokenCoordinate start,
                 TokenCoordinate end);

  std::string string() const;
};

class Lexer final {
  std::ifstream m_hdl_file;
  std::vector<Token> m_tokens;
  std::size_t m_curr_x{0}, m_curr_y{0};

  Token lex_number();
  Token lex_ident();
  void ignore_inline_comment();
  void ignore_multiline_comment();

public:
  explicit Lexer(const char *file_path);
  std::vector<Token> tokenize();
};

} // namespace hdl

#endif

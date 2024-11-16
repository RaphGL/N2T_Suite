#ifndef HDL_PARSER_H
#define HDL_PARSER_H

#include "../report/report.hpp"
#include "lexer.hpp"
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace hdl {

struct Range {
  std::size_t from, to;
};

struct Arg {
  std::string name;
  std::optional<Range> range;
  std::string output;
};

struct Part {
  std::string name;
  std::vector<Arg> args;
};

struct InOut {
  // output if false
  bool input;
  std::string name;
  std::size_t size;
};

struct Chip {
  std::string name;
  std::vector<Part> parts;
  std::vector<InOut> inouts;
};

class Parser {
  std::vector<Token> m_tokens;
  std::size_t m_idx{0};
  std::string m_error_report{""};
  report::Context m_reporter;

  bool eof();
  bool peek_expected(TokenType tt);
  Token peek();
  std::optional<Token> eat();
  void uneat();

  std::optional<Chip> parse_chip();
  std::optional<std::vector<InOut>> parse_inout();
  std::optional<Part> parse_part();
  std::optional<Arg> parse_arg();
  std::optional<Range> parse_range();

  void emit_error(const Token &tok, std::string_view error);

public:
  explicit Parser(std::vector<Token> tokens, const char *filepath);
  std::optional<std::vector<Chip>> parse();
  std::string get_error_report();
};
void print_ast(std::vector<Chip> ast);
} // namespace hdl

#endif

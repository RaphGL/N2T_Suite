#include "lexer.hpp"
#include <vector>

namespace hdl {

// chip = "CHIP" ident "{" (inout)* "PARTS:" (part)* "}"
// ident = A-z | 0-9 | "_"
// inout = ("IN" | "OUT") (ident ("[" 0-9 "]")?),* ";"
// part = ident "("(arg),* ")";
// arg = ident (range)? "=" ident
// range = (0-9)* ".." (0-9)*

struct Range {
  std::size_t from, to;
};

struct Arg {
  std::string argname;
  Range range;
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

  void parse_ident();
  void parse_inout();
  void parse_part();
  void parse_arg();
  void parse_range();

public:
  Parser(std::vector<Token> tokens);
  void parse();
};

Parser::Parser(std::vector<Token> tokens) : m_tokens{tokens} {}

}; // namespace hdl

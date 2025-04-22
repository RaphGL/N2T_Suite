#ifndef ASM_PARSER_HPP
#define ASM_PARSER_HPP

#include "../base_parser.hpp"
#include "lexer.hpp"

namespace assembly {

struct AInstr {
  TokenCoordinate start_coord;
  TokenCoordinate end_coord;
  std::variant<std::size_t, std::string> value;
};

enum class Operator {
  // unary
  Neg,
  Not,

  // binary
  Add,
  Sub,
  And,
  Or,
};

enum class Address {
  A,
  D,
  M,
};

// operand can either be in the interval {-1, 0, 1} or an address
using Operand = std::variant<Address, char>;

struct UnaryComp {
  TokenCoordinate start;
  TokenCoordinate end;
  Operator op;
  Operand operand;
};

struct BinaryComp {
  TokenCoordinate start;
  TokenCoordinate end;
  Address left;
  Operator op;
  Operand right;
};

enum class Jump {
  None,

  JGT,
  JEQ,
  JLT,
  JGE,
  JNE,
  JMP,
};

enum class Destination {
  None,

  A,
  D,
  M,
  MD,
  AM,
  AD,
  AMD
};

struct CInstr {
  TokenCoordinate start;
  TokenCoordinate end;
  Destination dest;
  std::variant<UnaryComp, BinaryComp> comp;
  Jump jump;
};

struct Label {
  TokenCoordinate start_coord;
  TokenCoordinate end_coord;
  std::string value;
};

using Instruction = std::variant<Label, AInstr, CInstr>;

class Parser final : public BaseParser<Token, TokenType> {
  std::vector<Instruction> m_instructions;

  std::optional<CInstr> parse_cinstr();
  std::optional<AInstr> parse_ainstr();
  std::optional<Label> parse_label();

public:
  using BaseParser::BaseParser;
  std::optional<std::vector<Instruction>> parse();
};

} // namespace assembly

#endif

#ifndef ASM_PARSER_HPP
#define ASM_PARSER_HPP

#include "../base_parser.hpp"
#include "lexer.hpp"
#include <cstdint>

namespace assembly {

struct AInstr {
  TokenCoordinate start_coord;
  TokenCoordinate end_coord;
  std::variant<std::size_t, std::string> value;
};

enum class Operator {
  None, 

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
  None, 
  A,
  D,
  M,
};

// operand can either be in the interval 0, 1 or an address name
using Operand = std::variant<Address, std::size_t>;

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
  JLE,
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

  std::optional<Jump> parse_jump();
  std::optional<Destination> parse_dest();
  std::optional<std::variant<UnaryComp, BinaryComp>> parse_comp();

  std::optional<CInstr> parse_cinstr();
  std::optional<AInstr> parse_ainstr();
  std::optional<Label> parse_label();

public:
  using BaseParser::BaseParser;
  std::optional<std::vector<Instruction>> parse();
};

} // namespace assembly

#endif

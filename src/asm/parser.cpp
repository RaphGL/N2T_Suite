#include "../base_parser.hpp"
#include "../report/report.hpp"
#include "lexer.hpp"
#include <string>
#include <variant>

namespace assembly {

struct AInstr {
  TokenCoordinate start;
  TokenCoordinate end;
  std::variant<std::string, std::size_t> value;
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
  TokenCoordinate start;
  TokenCoordinate end;
  std::string value;
};

class Parser final : public BaseParser<Token, TokenType> {
  std::variant<Label, AInstr, CInstr> instructions;
  using BaseParser::BaseParser;

public:
  std::variant<Label, AInstr, CInstr> parse();
};

}; // namespace assembly

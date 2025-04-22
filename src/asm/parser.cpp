#include "../base_parser.hpp"
#include "lexer.hpp"
#include <format>
#include <optional>
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

using Instruction = std::variant<Label, AInstr, CInstr>;

class Parser final : public BaseParser<Token, TokenType> {
  std::vector<Instruction> m_instructions;
  using BaseParser::BaseParser;

  std::optional<CInstr> parse_cinstr();
  std::optional<AInstr> parse_ainstr();
  std::optional<Label> parse_label();

public:
  std::optional<std::vector<Instruction>> parse();
};

std::string get_stringified_token(decltype(Token::value) token) {
  if (std::holds_alternative<int>(token)) {
    return std::to_string(std::get<int>(token));
  }

  if (std::holds_alternative<std::size_t>(token)) {
    return std::to_string(std::get<std::size_t>(token));
  }

  return std::get<std::string>(token);
}

std::optional<Label> Parser::parse_label() {
  auto curr_token = this->curr_token();
  if (curr_token.type != TokenType::OpenParen) {
    this->emit_error(curr_token,
                     std::format("Expected '(', found '{}'",
                                 get_stringified_token(curr_token.value)));
    return std::nullopt;
  }
  this->eat();

  if (!this->peek_expected(TokenType::Label)) {
    this->emit_error(
        this->peek(),
        std::format("Expected a label name or an number, found '{}'",
                    get_stringified_token(this->peek().value)));
    return std::nullopt;
  }
  auto label_option = this->eat();

  if (!this->peek_expected(TokenType::CloseParen)) {
    this->emit_error(this->peek(),
                     std::format("Expected ')', found '{}'",
                                 get_stringified_token(this->peek().value)));
    return std::nullopt;
  }

  Label label{};
  if (label_option.has_value()) {
    auto label_token = label_option.value();
    if (!std::holds_alternative<std::string>(label_token.value)) {
      this->emit_error(
          label_token,
          std::format(
              "Expected label to be a valid string but instead found '{}'",
              get_stringified_token(label_token.value)));
      return std::nullopt;
    }

    label.value = std::get<std::string>(label_token.value);
    label.start = label_token.start_coord;
    label.end = label_token.end_coord;
  }

  return label;
}

CInstr Parser::parse_cinstr() {}

AInstr Parser::parse_ainstr() {}

std::optional<std::vector<Instruction>> Parser::parse() {
  while (this->eof()) {
    auto token_opt = this->eat();
    if (!token_opt.has_value()) {
      break;
    }

    auto token = token_opt.value();

    std::optional<Label> label;
    std::optional<AInstr> ainstr;
    std::optional<CInstr> cinstr;

    // todo: handle failing case for parsing
    switch (token.type) {
    case TokenType::OpenParen:
      label = this->parse_label();
      if (label.has_value()) {
        m_instructions.push_back(label.value());
      }
      break;

    case TokenType::AtSymbol:
      ainstr = this->parse_ainstr();
      if (ainstr.has_value()) {
        m_instructions.push_back(ainstr.value());
      }
      break;

    default:
      cinstr = this->parse_cinstr();
      if (cinstr.has_value()) {
        m_instructions.push_back(cinstr.value());
      }
      break;
    }
  }

  auto error_report = m_reporter.generate_final_report();
  if (error_report.has_value()) {
    m_error_report = error_report.value();
    return std::nullopt;
  }

  return m_instructions;
}

}; // namespace assembly

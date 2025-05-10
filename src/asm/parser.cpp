#include "parser.hpp"
#include "lexer.hpp"
#include <cctype>
#include <format>
#include <optional>
#include <string>
#include <variant>

namespace assembly {

std::string get_stringified_token(decltype(Token::value) token) {
  if (std::holds_alternative<int>(token)) {
    return std::string(1, std::get<int>(token));
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
  this->eat();

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
    label.start_coord = curr_token.start_coord;
    label.end_coord = label_token.end_coord;
  }

  return label;
}

std::optional<Destination> Parser::parse_dest() {
  auto curr_token = this->curr_token();
  auto dest = Destination::None;

  // valid destinations:
  // - A, AM, AD, AMD
  // - D
  // - M, MD

  if (curr_token.type == TokenType::Label &&
      std::holds_alternative<std::string>(curr_token.value)) {
    auto token_value = std::get<std::string>(curr_token.value);

    if (token_value == "A") {
      dest = Destination::A;
    } else if (token_value == "AM") {
      dest = Destination::AM;
    } else if (token_value == "AD") {
      dest = Destination::AD;
    } else if (token_value == "AMD") {
      dest = Destination::AMD;
    } else if (token_value == "D") {
      dest = Destination::D;
    } else if (token_value == "M") {
      dest = Destination::M;
    } else if (token_value == "MD") {
      dest = Destination::MD;
    } else {
      this->emit_error(curr_token,
                       std::format("Invalid destination. Expected A, M, D, AM, "
                                   "AD, AMD, M, or MD. But found `{}`",
                                   token_value));
      return std::nullopt;
    }
  } else {
    this->emit_error(curr_token,
                     std::format("Invalid destination. Expected A, M, D, AM, "
                                 "AD, AMD, M, or MD. But found `{}`",
                                 get_stringified_token(curr_token.value)));
    return std::nullopt;
  }

  return dest;
}

std::optional<std::variant<UnaryComp, BinaryComp>> Parser::parse_comp() {
  auto curr_token = this->curr_token();

  // unary

  UnaryComp unary_comp;
  unary_comp.start = curr_token.start_coord;

  auto parse_single_dest =
      [this](const Token curr,
             TokenCoordinate &token_end) -> std::optional<Address> {
    if (std::holds_alternative<std::string>(curr.value)) {
      auto curr_value = std::get<std::string>(curr.value);
      token_end = curr.end_coord;

      if (curr_value == "D") {
        return Address::D;
      } else if (curr_value == "A") {
        return Address::A;
      } else if (curr_value == "M") {
        return Address::M;
      }
    }

    this->emit_error(curr, std::format("Expected one of the valid addresses "
                                       "(A, M, D), but found `{}`",
                                       get_stringified_token(curr.value)));
    return std::nullopt;
  };

  // !D, !A, !M
  if (curr_token.type == TokenType::Exclamation) {
    unary_comp.op = Operator::Not;

    if (this->peek_expected(TokenType::Label)) {
      auto curr = this->eat().value();
      auto dest = parse_single_dest(curr, unary_comp.end);
      if (dest.has_value()) {
        unary_comp.operand = dest.value();
      } else {
        return std::nullopt;
      }
    } else {
      auto curr = this->peek();
      this->emit_error(
          curr,
          std::format(
              "Expected one of the valid addresses (A, M, D), but found `{}`",
              get_stringified_token(curr.value)));
      return std::nullopt;
    }
  }

  // -D, -A, -1, -M
  if (curr_token.type == TokenType::Minus) {
    unary_comp.op = Operator::Neg;

    if (this->peek_expected(TokenType::Label)) {
      auto curr = this->eat().value();
      auto dest = parse_single_dest(curr, unary_comp.end);
      if (dest.has_value()) {
        unary_comp.operand = dest.value();
      } else {
        return std::nullopt;
      }
    } else if (this->peek_expected(TokenType::Number)) {
      auto curr = this->eat().value();
      unary_comp.end = curr.end_coord;
      if (std::holds_alternative<std::size_t>(curr.value)) {
        auto num = std::get<std::size_t>(curr.value);
        if (num > 1) {
          this->emit_error(curr,
                           "Only numbers 0 and 1 are valid in C-instructions");
          return std::nullopt;
        }
        unary_comp.operand = num;
      } else {
        this->emit_error(curr, std::format("Expected a number, but found `{}`",
                                           get_stringified_token(curr.value)));
        return std::nullopt;
      }
    }
  }

  // 0, 1
  if (curr_token.type == TokenType::Number) {
    unary_comp.op = Operator::None;

    if (std::holds_alternative<std::size_t>(curr_token.value)) {
      auto num = std::get<std::size_t>(curr_token.value);
      if (num > 1) {
        this->emit_error(curr_token,
                         std::format("Expected either 0 or 1 but found `{}`",
                                     get_stringified_token(curr_token.value)));
        return std::nullopt;
      }
      unary_comp.end = curr_token.end_coord;
      unary_comp.operand = num;
    } else {
      this->emit_error(curr_token,
                       std::format("Expected a number, found `{}`",
                                   get_stringified_token(curr_token.value)));
      return std::nullopt;
    }
  }

  // D, A, M
  if (curr_token.type == TokenType::Label) {
    auto dest = parse_single_dest(curr_token, unary_comp.end);
    if (dest.has_value()) {
      unary_comp.operand = dest.value();
    } else {
      return std::nullopt;
    }
  }

  if (!this->peek_expected(TokenType::Plus) &&
      !this->peek_expected(TokenType::Minus) &&
      !this->peek_expected(TokenType::Ampersand) &&
      !this->peek_expected(TokenType::Pipe)) {
    return unary_comp;
  }

  // binary
  // D+1, A+1, D-1, A-1, M+1, M-1,
  // D+A, D-A, D&A, D|A
  // A-D, M-D,
  // D+M, D-M, D&M, D|M

  if (!std::holds_alternative<Address>(unary_comp.operand)) {
    this->emit_error(curr_token,
                     std::format("Expected an address but found `{}`",
                                 get_stringified_token(std::get<std::size_t>(
                                     unary_comp.operand))));
    return std::nullopt;
  }

  BinaryComp binary_comp;
  binary_comp.start = curr_token.start_coord;
  binary_comp.left = std::get<Address>(unary_comp.operand);
  binary_comp.right = Address::None;

  auto next_token = this->peek();
  switch (next_token.type) {
  case TokenType::Plus:
    binary_comp.op = Operator::Add;
    break;
  case TokenType::Minus:
    binary_comp.op = Operator::Sub;
    break;
  case TokenType::Pipe:
    binary_comp.op = Operator::Or;
    break;
  case TokenType::Ampersand:
    binary_comp.op = Operator::And;
    break;
  default:
    this->emit_error(
        next_token,
        std::format(
            "Expected one of the valid operators (+, -, | and &), found `{}`",
            get_stringified_token(next_token.value)));
    return std::nullopt;
  }
  curr_token = this->eat().value();

  if (this->peek_expected(TokenType::Number)) {
    curr_token = this->eat().value();
    if (std::holds_alternative<std::size_t>(curr_token.value)) {
      auto num = std::get<std::size_t>(curr_token.value);
      if (num != 1) {
        this->emit_error(curr_token,
                         std::format("Expected `1` but found `{}`",
                                     get_stringified_token(curr_token.value)));
        return std::nullopt;
      }
      binary_comp.right = num;
    } else {
      this->emit_error(curr_token,
                       std::format("Expected a valid number, found `{}`",
                                   get_stringified_token(curr_token.value)));
      return std::nullopt;
    }
  } else if (this->peek_expected(TokenType::Label)) {
    auto curr = this->eat().value();
    auto dest = parse_single_dest(curr, binary_comp.end);
    if (dest.has_value()) {
      binary_comp.right = dest.value();
    } else {
      return std::nullopt;
    }
  } else {
    next_token = this->peek();
    this->emit_error(
        next_token,
        std::format(
            "Expected a valid right hand side (A, D, M or 1) but found `{}`",
            get_stringified_token(next_token.value)));
    return std::nullopt;
  }

  binary_comp.end = this->curr_token().end_coord;
  return binary_comp;
}

std::optional<Jump> Parser::parse_jump() {
  auto curr_token = this->curr_token();
  if (curr_token.type != TokenType::Label) {
    this->emit_error(curr_token,
                     std::format("expected a jump instruction but found `{}`",
                                 get_stringified_token(curr_token.value)));
    return std::nullopt;
  }

  auto label = std::get<std::string>(curr_token.value);
  for (auto &c : label) {
    c = std::toupper(c);
  }

  auto jump = Jump::None;

  if (label == "JGT") {
    jump = Jump::JGT;
  } else if (label == "JEQ") {
    jump = Jump::JEQ;
  } else if (label == "JLT") {
    jump = Jump::JLT;
  } else if (label == "JGE") {
    jump = Jump::JGE;
  } else if (label == "JNE") {
    jump = Jump::JNE;
  } else if (label == "JLE") {
    jump = Jump::JLE;
  } else if (label == "JMP") {
    jump = Jump::JMP;
  }

  if (jump == Jump::None) {
    this->emit_error(
        curr_token, std::format("Found invalid jump instruction `{}`, expected "
                                "one of: JGT, JEQ, JLT, JGE, JNE, JMP",
                                label));
    return std::nullopt;
  }

  return jump;
}

std::optional<CInstr> Parser::parse_cinstr() {
  auto curr_token = this->curr_token();
  CInstr cinstr;
  cinstr.start = curr_token.start_coord;

  bool has_dest = false;
  for (auto i = 0; i < 3; i++) {
    if (m_idx + i >= m_tokens.size()) {
      break;
    }
    if (m_tokens[m_idx + i].type == TokenType::Equal) {
      has_dest = true;
    }
  }

  std::optional<Destination> dest = Destination::None;
  if (has_dest) {
    dest = this->parse_dest();
    if (dest.has_value()) {
      cinstr.dest = dest.value();
      if (!this->peek_expected(TokenType::Equal)) {
        auto next_token = this->peek();
        this->emit_error(next_token,
                         std::format("Expected a `=`, found `{}`",
                                     get_stringified_token(next_token.value)));
        return std::nullopt;
      }
      this->eat();
      // second eat is necessary cause otherwise parse_comp would see Equal
      this->eat();
    } else {
      cinstr.dest = Destination::None;
    }
  }

  auto comp = this->parse_comp();
  if (comp.has_value()) {
    cinstr.comp = comp.value();
  } else {
    auto curr = this->curr_token();
    this->emit_error(curr, "Expected a valid computation such as 0, 1 or A+1");
    return std::nullopt;
  }

  auto jump = Jump::None;
  if (this->peek_expected(TokenType::Semicolon)) {
    this->eat();
    this->eat();
    auto curr = this->curr_token();
    auto jump_opt = this->parse_jump();
    if (!jump_opt.has_value()) {
      this->emit_error(curr, "Expected a valid jump instruction");
      return std::nullopt;
    }
    jump = jump_opt.value();
  }

  cinstr.jump = jump;
  cinstr.end = this->curr_token().end_coord;
  return cinstr;
}

std::optional<AInstr> Parser::parse_ainstr() {
  auto curr_token = this->curr_token();
  if (curr_token.type != TokenType::AtSymbol) {
    this->emit_error(curr_token,
                     std::format("Expected `@`, found `{}`",
                                 get_stringified_token(curr_token.value)));
    return std::nullopt;
  }

  if (!this->peek_expected(TokenType::Label) &&
      !this->peek_expected(TokenType::Number)) {
    auto next_token = this->peek();
    this->emit_error(next_token,
                     std::format("Expected `@`, found `{}`",
                                 get_stringified_token(next_token.value)));
    return std::nullopt;
  }
  auto inst_value = this->eat().value();

  AInstr ainstr;
  ainstr.start_coord = curr_token.start_coord;
  ainstr.end_coord = inst_value.end_coord;

  if (std::holds_alternative<std::string>(inst_value.value)) {
    ainstr.value = std::get<std::string>(inst_value.value);
  } else if (std::holds_alternative<std::size_t>(inst_value.value)) {
    ainstr.value = std::get<std::size_t>(inst_value.value);
  }

  return ainstr;
}

std::optional<std::vector<Instruction>> Parser::parse() {
  std::optional<Label> label;
  std::optional<AInstr> ainstr;
  std::optional<CInstr> cinstr;

  while (!this->eof()) {
    auto token = this->curr_token();
    if (token.type == TokenType::Newline) {
      this->eat();
      continue;
    }

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

    auto end_token = this->eat();
    if (end_token.has_value() && end_token.value().type != TokenType::Newline) {
      auto next_token = this->peek();
      this->emit_error(next_token,
                       std::format("Expected a new line, found `{}`",
                                   get_stringified_token(next_token.value)));
      break;
    }
    this->eat();
  }

  auto error_report = m_reporter.generate_final_report();
  if (error_report.has_value()) {
    m_error_report = error_report.value();
    return std::nullopt;
  }

  return m_instructions;
}

}; // namespace assembly

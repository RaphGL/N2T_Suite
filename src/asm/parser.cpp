#include "parser.hpp"
#include "lexer.hpp"
#include <format>
#include <optional>
#include <string>
#include <variant>

namespace assembly {

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

// todo
std::optional<CInstr> Parser::parse_cinstr() {
  auto curr_token = this->curr_token();
  return std::nullopt;
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

    auto end_token = this->eat();
    if (end_token.has_value() && end_token.value().type != TokenType::Newline) {
      auto next_token = this->peek();
      this->emit_error(next_token, std::format("Expected a new line, found `{}`", get_stringified_token(next_token.value)));
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

#include "parser.hpp"
#include "lexer.hpp"
#include <format>
#include <iostream>
#include <optional>
#include <vector>

namespace hdl {

Parser::Parser(std::vector<Token> tokens) : m_tokens{tokens}, m_idx{0} {}

bool Parser::peek_expected(TokenType tt) {
  std::size_t next_idx = m_idx + 1;
  if (next_idx >= m_tokens.size()) {
    return false;
  }

  auto next_token = m_tokens.at(next_idx);
  if (next_token.type != tt) {
    return false;
  }

  return true;
}

Token Parser::peek() { return m_tokens.at(m_idx + 1); }

bool Parser::eof() { return m_idx >= m_tokens.size(); }

std::optional<Token> Parser::eat() {
  std::size_t next_idx = m_idx + 1;
  if (next_idx < m_tokens.size()) {
    m_idx = next_idx;
    return m_tokens[m_idx];
  }

  return std::nullopt;
}

void Parser::uneat() {
  if (m_idx > 0) {
    --m_idx;
  }
}

// chip = "CHIP" ident "{" (inout)* "PARTS:" (part)* "}"
// ident = A-z | 0-9 | "_"
// inout = ("IN" | "OUT") (ident ("[" 0-9* "]")?),* ";"
// part = ident "("(arg),* ")";
// arg = ident (range)? "=" ident
// range = "[" (0-9)* ".." (0-9)* "]"
std::optional<Range> Parser::parse_range() {
  if (!this->peek_expected(TokenType::OpenBracket)) {
    m_errors.push_back(
        std::format("expected `[` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  this->eat();

  if (!this->peek_expected(TokenType::Number)) {
    m_errors.push_back(
        std::format("expected a number found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }

  Range range{};
  range.from = std::get<std::size_t>(this->eat().value().value);

  if (!this->peek_expected(TokenType::RangeOp)) {
    m_errors.push_back(std::format("expected `..` number found {}",
                                   m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  this->eat();

  if (!this->peek_expected(TokenType::Number)) {
    m_errors.push_back(
        std::format("expected a number found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  range.to = std::get<std::size_t>(this->eat().value().value);

  if (!this->peek_expected(TokenType::CloseBracket)) {
    m_errors.push_back(
        std::format("expected `]` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  this->eat();

  return range;
}

std::optional<Arg> Parser::parse_arg() {
  if (!this->peek_expected(TokenType::Ident)) {
    m_errors.push_back(std::format("expected identifier found {}",
                                   m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  Arg arg{};
  arg.name = std::get<std::string>(this->eat().value().value);

  if (this->peek_expected(TokenType::OpenBracket)) {
    arg.range = this->parse_range();
  } else {
    arg.range = std::nullopt;
  }

  if (!this->peek_expected(TokenType::Equal)) {
    m_errors.push_back(
        std::format("expected `=` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  this->eat();

  if (!this->peek_expected(TokenType::Ident)) {
    m_errors.push_back(std::format("expected identifier found {}",
                                   m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  arg.output = std::get<std::string>(this->eat().value().value);

  return arg;
}

std::optional<Part> Parser::parse_part() {
  if (!this->peek_expected(TokenType::Ident)) {
    m_errors.push_back(std::format("expected identifier found {}",
                                   m_tokens.at(m_idx).string()));
    return std::nullopt;
  }

  Part part{};
  part.name = std::get<std::string>(this->eat().value().value);

  if (!this->peek_expected(TokenType::OpenParen)) {
    m_errors.push_back(
        std::format("expected `(` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  this->eat();

  while (!this->peek_expected(TokenType::CloseParen)) {
    auto arg = this->parse_arg();
    if (arg.has_value()) {
      part.args.push_back(arg.value());
    }

    if (!this->peek_expected(TokenType::Comma)) {
      break;
    }
    this->eat();
  }

  if (!this->peek_expected(TokenType::CloseParen)) {
    m_errors.push_back(
        std::format("expected `)` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  this->eat();

  if (!this->peek_expected(TokenType::Semicolon)) {
    m_errors.push_back(
        std::format("expected `;` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  this->eat();

  return part;
}

std::optional<std::vector<InOut>> Parser::parse_inout() {
  if (!this->peek_expected(TokenType::Ident)) {
    m_errors.push_back(std::format("expected identifier found {}",
                                   m_tokens.at(m_idx).string()));
    return std::nullopt;
  }

  InOut inout{};
  auto keyword = std::get<std::string>(this->eat().value().value);
  if (keyword == "IN") {
    inout.input = true;
  } else if (keyword == "OUT") {
    inout.input = false;
  } else {
    m_errors.push_back(std::format("expected `in` or `out` found {}",
                                   m_tokens.at(m_idx).string()));
    return std::nullopt;
  }

  std::vector<InOut> ports;
  while (!this->eof() && this->peek().type != TokenType::Semicolon) {
    if (!this->peek_expected(TokenType::Ident)) {
      m_errors.push_back(std::format("expected identifier found {}",
                                     m_tokens.at(m_idx).string()));
      return std::nullopt;
    }

    InOut port = inout;
    port.name = std::get<std::string>(this->eat().value().value);
    port.size = 1;

    if (this->peek_expected(TokenType::OpenBracket)) {
      this->eat();

      if (!this->peek_expected(TokenType::Number)) {
        m_errors.push_back(std::format("expected a number found {}",
                                       m_tokens.at(m_idx).string()));
        return std::nullopt;
      }
      port.size = std::get<std::size_t>(this->eat().value().value);

      if (!this->peek_expected(TokenType::CloseBracket)) {
        m_errors.push_back(std::format("expected a `}}` found {}",
                                       m_tokens.at(m_idx).string()));
        return std::nullopt;
      }
      this->eat();
    }

    ports.push_back(port);

    if (!this->peek_expected(TokenType::Comma)) {
      break;
    }
    this->eat();
  }

  if (!this->peek_expected(TokenType::Semicolon)) {
    m_errors.push_back(
        std::format("expected a `;` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  this->eat();

  return ports;
}

// TODO: improve error printing
std::optional<Chip> Parser::parse_chip() {
  Chip chip{};
  if (!this->peek_expected(TokenType::Ident)) {
    m_errors.push_back(std::format("expected identifier found {}",
                                   m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  auto curr_token = this->eat().value();

  chip.name = std::get<std::string>(curr_token.value);

  if (!this->peek_expected(TokenType::OpenBrace)) {
    m_errors.push_back(
        std::format("expected `{{` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  this->eat();

  curr_token = this->peek();
  while (!this->eof() &&
         !(curr_token.type == TokenType::Ident &&
           std::get<std::string>(curr_token.value) == "PARTS")) {
    auto inout = this->parse_inout();

    if (inout.has_value()) {
      for (const auto &port : inout.value()) {
        chip.inouts.push_back(port);
      }
    }
    curr_token = this->peek();
  }

  if (!this->peek_expected(TokenType::Ident)) {
    m_errors.push_back(std::format("expected identifier found {}",
                                   m_tokens.at(m_idx).string()));
    return std::nullopt;
  }

  curr_token = this->eat().value();
  if (std::get<std::string>(curr_token.value) != "PARTS") {
    m_errors.push_back(
        std::format("expected `PARTS` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }

  if (!this->peek_expected(TokenType::Colon)) {
    m_errors.push_back(
        std::format("expected `:` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  this->eat();

  while (!this->eof() && this->peek().type != TokenType::CloseBrace) {
    auto part = this->parse_part();
    if (part.has_value()) {
      chip.parts.push_back(part.value());
    }
  }

  if (!this->peek_expected(TokenType::CloseBrace)) {
    m_errors.push_back(
        std::format("expected `}}` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  this->eat();

  return chip;
}

std::optional<std::vector<Chip>> Parser::parse() {
  std::vector<Chip> chips{};
  while (!this->eof()) {
    Token token{m_tokens.at(m_idx)};
    if (token.type != TokenType::Ident) {
      // print error
      break;
    }

    if (std::get<std::string>(token.value) == "CHIP") {
      auto chip = this->parse_chip();
      if (chip.has_value()) {
        chips.push_back(chip.value());
      }
    } else {
      // make an error as the global parser should only be in charge of calling
      // parse_chip and putting everything together
    }
  }

  if (!m_errors.empty()) {
    for (auto &error : m_errors) {
      std::cerr << error << '\n';
    }

    return std::nullopt;
  }

  return chips;
}

namespace {
std::string get_inout_string(const std::vector<InOut> &inouts) {
  std::string res{};
  for (const auto &inout : inouts) {
    res += std::format("\t{} {}[{}];\n", inout.input ? "IN" : "OUT", inout.name,
                       inout.size);
  }

  return res;
}

std::string get_parts_string(const std::vector<Part> &parts) {
  std::string res{};
  for (const auto &part : parts) {
    std::string args{};
    for (const auto &arg : part.args) {
      args += std::format("{}={},", arg.name, arg.output);
    }
    res += std::format("\t{}({});\n", part.name, args);
  }

  return res;
}

} // namespace

void print_ast(std::vector<Chip> ast) {
  for (const auto &chip : ast) {
    std::cout << std::format("CHIP {} {{\n {}\n PARTS:\n {}\n}}", chip.name,
                             get_inout_string(chip.inouts),
                             get_parts_string(chip.parts));
  }
}

}; // namespace hdl

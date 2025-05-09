#include "parser.hpp"
#include "../report/report.hpp"
#include "lexer.hpp"
#include <format>
#include <optional>
#include <vector>

namespace hdl {

std::optional<Range> Parser::parse_range() {
  if (!this->peek_expected(TokenType::OpenBracket)) {
    auto token = this->peek();
    this->emit_error(token,
                     std::format("expected `[`, found `{}`", token.string()));
    return std::nullopt;
  }
  auto token = this->eat().value();
  Range range{};
  range.start_coord = token.start_coord;

  if (!this->peek_expected(TokenType::Number)) {
    auto token = this->peek();
    this->emit_error(
        token, std::format("expected a number, found `{}`", token.string()));
    return std::nullopt;
  }

  range.from = std::get<std::size_t>(this->eat().value().value);

  if (this->peek_expected(TokenType::CloseBracket)) {
    auto token = this->eat().value();
    range.to = range.from;
    range.end_coord = token.end_coord;
    return range;
  }

  if (!this->peek_expected(TokenType::RangeOp)) {
    auto token = this->peek();
    this->emit_error(token,
                     std::format("expected `..`, found `{}`", token.string()));
    return std::nullopt;
  }
  this->eat();

  if (!this->peek_expected(TokenType::Number)) {
    auto token = this->peek();
    this->emit_error(
        token, std::format("expected a number, found `{}`", token.string()));
    return std::nullopt;
  }
  range.to = std::get<std::size_t>(this->eat().value().value);

  if (!this->peek_expected(TokenType::CloseBracket)) {
    auto token = this->peek();
    this->emit_error(token,
                     std::format("expected `]`, found `{}`", token.string()));
    return std::nullopt;
  }

  token = this->eat().value();
  range.end_coord = token.end_coord;

  return range;
}

std::optional<Arg> Parser::parse_arg() {
  if (!this->peek_expected(TokenType::Ident)) {
    auto token = this->peek();
    this->emit_error(token,
                     std::format("expected a part parameter but found `{}`",
                                 token.string()));
    return std::nullopt;
  }
  Arg arg{};
  auto token = this->eat().value();
  arg.name = std::get<std::string>(token.value);
  arg.start_coord = token.start_coord;

  if (this->peek_expected(TokenType::OpenBracket)) {
    arg.range = this->parse_range();
  } else {
    arg.range = std::nullopt;
  }

  if (!this->peek_expected(TokenType::Equal)) {
    auto token = this->peek();
    this->emit_error(token,
                     std::format("expected `=`, found `{}`", token.string()));
    return std::nullopt;
  }
  this->eat();

  if (!this->peek_expected(TokenType::Ident)) {
    auto token = this->peek();
    this->emit_error(
        token, std::format("expected an argument, found `{}`", token.string()));
    return std::nullopt;
  }

  token = this->eat().value();
  arg.output = std::get<std::string>(token.value);
  arg.end_coord = token.end_coord;

  return arg;
}

std::optional<Part> Parser::parse_part() {
  if (!this->peek_expected(TokenType::Ident)) {
    auto token = this->peek();
    this->emit_error(
        token, std::format("expected part name, found `{}`", token.string()));
    return std::nullopt;
  }

  Part part{};
  auto token = this->eat().value();
  part.name = std::get<std::string>(token.value);
  part.start_coord = token.start_coord;

  if (!this->peek_expected(TokenType::OpenParen)) {
    auto token = this->peek();
    this->emit_error(token,
                     std::format("expected `(`, found `{}`", token.string()));
    return std::nullopt;
  }
  this->eat();

  if (!this->peek_expected(TokenType::Ident)) {
    auto token = this->peek();
    this->emit_error(token, std::format("expected an argument in the form "
                                        "`a=b` or `a[x..y]=b` but found {}",
                                        token.string()));
    return std::nullopt;
  }

  while (!this->peek_expected(TokenType::CloseParen)) {
    auto arg = this->parse_arg();
    if (arg.has_value()) {
      part.args.push_back(arg.value());
    } else {
      break;
    }

    if (!this->peek_expected(TokenType::Comma)) {
      break;
    }
    this->eat();
  }

  if (!this->peek_expected(TokenType::CloseParen)) {
    auto token = this->peek();
    this->emit_error(token,
                     std::format("expected `)`, found `{}`", token.string()));
    return std::nullopt;
  }
  this->eat();

  if (!this->peek_expected(TokenType::Semicolon)) {
    auto token = this->peek();
    this->emit_error(token,
                     std::format("expected `;`, found `{}`", token.string()));
    return std::nullopt;
  }
  token = this->eat().value();
  part.end_coord = token.end_coord;

  return part;
}

std::optional<std::vector<InOut>> Parser::parse_inout() {
  if (!this->peek_expected(TokenType::Ident)) {
    auto token = this->peek();
    this->emit_error(token, std::format("expected `IN` or `OUT`, found `{}`",
                                        token.string()));
    return std::nullopt;
  }

  InOut inout{};
  auto keyword = std::get<std::string>(this->eat().value().value);
  if (keyword == "IN") {
    inout.input = true;
  } else if (keyword == "OUT") {
    inout.input = false;
  } else {
    auto token = this->curr_token();
    this->emit_error(token, std::format("expected `IN` or `OUT`, found `{}`",
                                        token.string()));
    return std::nullopt;
  }

  if (this->peek_expected(TokenType::Semicolon)) {
    auto token = this->peek();
    this->emit_error(
        token, std::format("expected pin name, found `{}`", token.string()));
    return std::nullopt;
  }

  std::vector<InOut> ports;
  while (!this->eof() && this->peek().type != TokenType::Semicolon) {
    if (!this->peek_expected(TokenType::Ident)) {
      auto token = this->peek();
      this->emit_error(
          token, std::format("expected pin name, found `{}`", token.string()));
      return std::nullopt;
    }

    InOut port = inout;
    auto token = this->eat().value();
    port.name = std::get<std::string>(token.value);
    port.size = 1;
    port.start_coord = token.start_coord;
    port.end_coord = token.end_coord;

    if (this->peek_expected(TokenType::OpenBracket)) {
      this->eat();

      if (!this->peek_expected(TokenType::Number)) {
        auto token = this->peek();
        this->emit_error(token, std::format("expected a number, found `{}`",
                                            token.string()));
        return std::nullopt;
      }
      port.size = std::get<std::size_t>(this->eat().value().value);

      if (!this->peek_expected(TokenType::CloseBracket)) {
        auto token = this->peek();
        this->emit_error(
            token, std::format("expected `}}`, found `{}`", token.string()));
        return std::nullopt;
      }
      auto token = this->eat().value();
      port.end_coord = token.end_coord;
    }

    ports.push_back(port);

    if (this->peek_expected(TokenType::Ident)) {
      auto token = this->peek();
      this->emit_error(token,
                       std::format("missing `,` before `{}`", token.string()));
      this->eat();
      break;
    }

    if (!this->peek_expected(TokenType::Comma)) {
      break;
    }
    this->eat();
  }

  if (!this->peek_expected(TokenType::Semicolon)) {
    auto token = this->peek();
    this->emit_error(token,
                     std::format("expected a `;`, found `{}`", token.string()));
    return std::nullopt;
  }
  this->eat();

  return ports;
}

std::optional<Chip> Parser::parse_chip() {
  Chip chip{};
  if (!this->peek_expected(TokenType::Ident)) {
    auto token = this->peek();
    this->emit_error(token, std::format("expected an identifier, found `{}`",
                                        token.string()));
    return std::nullopt;
  }
  auto curr_token = this->eat().value();

  chip.name = std::get<std::string>(curr_token.value);

  if (!this->peek_expected(TokenType::OpenBrace)) {
    auto token = this->peek();
    this->emit_error(token,
                     std::format("expected `{{`, found `{}`", token.string()));
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
    } else {
      break;
    }
    curr_token = this->peek();
  }

  if (!this->peek_expected(TokenType::Ident)) {
    auto token = this->peek();
    this->emit_error(
        token, std::format("expected `PARTS`, found `{}`", token.string()));
    return std::nullopt;
  }

  curr_token = this->eat().value();
  if (std::get<std::string>(curr_token.value) != "PARTS") {
    auto token = this->curr_token();
    this->emit_error(token, std::format("expected `PARTS`, found `{}`",
                                        this->curr_token().string()));
    return std::nullopt;
  }

  if (!this->peek_expected(TokenType::Colon)) {
    auto token = this->peek();
    this->emit_error(token,
                     std::format("expected `:`, found `{}`", token.string()));
    return std::nullopt;
  }
  this->eat();

  while (!this->eof() && this->peek().type != TokenType::CloseBrace) {
    auto part = this->parse_part();
    if (part.has_value()) {
      chip.parts.push_back(part.value());
    } else {
      break;
    }
  }

  if (!this->peek_expected(TokenType::CloseBrace)) {
    auto token = this->peek();
    this->emit_error(token,
                     std::format("expected `}}`, found `{}`", token.string()));
    return std::nullopt;
  }
  this->eat();

  return chip;
}

std::optional<std::vector<Chip>> Parser::parse() {
  std::vector<Chip> chips{};
  Token token{this->curr_token()};
  while (!this->eof()) {
    if (token.type != TokenType::Ident) {
      this->emit_error(token, std::format("expected an identifier, found `{}`",
                                          token.string()));
      break;
    }

    if (std::get<std::string>(token.value) == "CHIP") {
      auto chip = this->parse_chip();
      if (chip.has_value()) {
        chips.push_back(chip.value());
      } else {
        break;
      }
    } else {
      this->emit_error(
          token, std::format("expected `CHIP`, found `{}`", token.string()));
      break;
    }

    if (m_idx + 1 < m_tokens.size()) {
      token = this->curr_token();
    } else {
      break;
    }
  }

  auto error_report = m_reporter.generate_final_report();
  if (error_report.has_value()) {
    m_error_report = error_report.value();
    return std::nullopt;
  }

  return chips;
}

}; // namespace hdl

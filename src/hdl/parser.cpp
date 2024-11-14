#include "lexer.hpp"
#include <format>
#include <optional>
#include <unordered_set>
#include <vector>

namespace hdl {

static std::unordered_set<std::string_view> g_keywords{
    {"CHIP", "IN", "OUT", "PART"}};

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
  std::size_t m_idx;
  std::vector<std::string> m_errors;

  bool eof();

  bool peek_expected(TokenType tt);
  Token peek();
  std::optional<Token> eat();
  void uneat();

  std::optional<Chip> parse_chip();
  std::optional<std::vector<InOut>> parse_inout();
  std::optional<Part> parse_part();
  Arg parse_arg();
  Range parse_range();

public:
  explicit Parser(std::vector<Token> tokens);
  void parse();
};

Parser::Parser(std::vector<Token> tokens) : m_tokens{tokens} {}

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
// range = (0-9)* ".." (0-9)*
std::optional<Part> Parser::parse_part() {
  
}

std::optional<std::vector<InOut>> Parser::parse_inout() {
  this->uneat();
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
  while (!this->eof() && this->peek().type == TokenType::Semicolon) {
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
  }

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
  this->eat();

  if (!this->peek_expected(TokenType::Colon)) {
    m_errors.push_back(
        std::format("expected `:` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }
  this->eat();

  curr_token = this->peek();
  while (!this->eof() && curr_token.type != TokenType::CloseBrace) {
    auto part = this->parse_part();
    chip.parts.push_back(part);
    curr_token = this->peek();
  }

  if (!this->peek_expected(TokenType::CloseBrace)) {
    m_errors.push_back(
        std::format("expected `}}` found {}", m_tokens.at(m_idx).string()));
    return std::nullopt;
  }

  return chip;
}

void Parser::parse() {

  Token token{m_tokens.at(0)};
  while (!this->eof()) {
    if (std::get<std::string>(token.value) == "CHIP") {
      this->parse_chip();
    } else {
      // make an error as the global parser should only be in charge of calling
      // parse_chip and putting everything together
    }
  }
}

}; // namespace hdl

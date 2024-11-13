#include "lexer.hpp"
#include <format>
#include <fstream>
#include <vector>

// todo: ignore comments

namespace hdl {

Token::Token(std::size_t n, TokenCoordinate start, TokenCoordinate end)
    : type{TokenType::Number}, start_coord{start}, end_coord{end}, number{n} {}
Token::Token(int s, TokenType tt, TokenCoordinate start, TokenCoordinate end)
    : type{tt}, start_coord{start}, end_coord{end}, symbol{s} {}
Token::Token(std::string id, TokenType tt, TokenCoordinate start,
             TokenCoordinate end)
    : type{tt}, start_coord{start}, end_coord{end}, ident{id} {}

std::string Token::string() {
  switch (type) {
  case TokenType::OpenBrace:
    return std::format("{{{}, {}}}", "OpenBrace", symbol);
  case TokenType::CloseBrace:
    return std::format("{{{}, {}}}", "CloseBrace", symbol);
  case TokenType::Semicolon:
    return std::format("{{{}, {}}}", "Semicolon", symbol);
  case TokenType::Colon:
    return std::format("{{{}, {}}}", "Colon", symbol);
  case TokenType::Comma:
    return std::format("{{{}, {}}}", "Comma", symbol);
  case TokenType::Equal:
    return std::format("{{{}, {}}}", "Equal", symbol);
  case TokenType::OpenParen:
    return std::format("{{{}, {}}}", "OpenParen", symbol);
  case TokenType::CloseParen:
    return std::format("{{{}, {}}}", "CloseParen", symbol);

  case TokenType::Ident:
    return std::format("{{{}, {}}}", "Ident", ident);
  case TokenType::Number:
    return std::format("{{{}, {}}}", "Number", number);
  case TokenType::RangeOp:
    return std::format("{{{}, {}}}", "RangeOp", ident);

  default:
    return "invalid token";
  }
}

Lexer::Lexer(const char *file_path) : m_hdl_file(file_path) {}
Token Lexer::lex_number() {
  TokenCoordinate start{
      .row = m_curr_x,
      .col = m_curr_y,
  };

  std::string numstr{};
  m_hdl_file.unget();
  numstr += m_hdl_file.get();

  while (!m_hdl_file.eof() && std::isdigit(m_hdl_file.peek())) {
    numstr += m_hdl_file.get();
  }

  TokenCoordinate end{
      .row = m_curr_x + numstr.length(),
      .col = m_curr_y,
  };

  int num{std::stoi(numstr)};

  return Token(static_cast<std::size_t>(num), start, end);
}

Token Lexer::lex_ident() {
  TokenCoordinate start{
      .row = m_curr_x,
      .col = m_curr_y,
  };

  std::string ident{};
  m_hdl_file.unget();
  ident += m_hdl_file.get();

  while (!m_hdl_file.eof()) {
    auto ch = m_hdl_file.peek();
    if (!std::isalnum(ch) && ch != '_') {
      break;
    }

    ident += m_hdl_file.get();
  }

  TokenCoordinate end{
      .row = m_curr_x + ident.length(),
      .col = m_curr_y,
  };
  return Token(ident, TokenType::Ident, start, end);
}

std::vector<Token> Lexer::tokenize() {
  m_hdl_file.peek();
  while (!m_hdl_file.eof()) {
    auto ch = m_hdl_file.get();

    if (std::isspace(ch)) {
      ++m_curr_y;
      m_curr_x = 0;
      continue;
    }

    TokenCoordinate coord{
        .row = m_curr_x,
        .col = m_curr_y,
    };

    if (ch == '{') {
      Token tok{ch, TokenType::OpenBrace, coord, coord};
      m_tokens.push_back(tok);
    } else if (ch == '}') {
      Token tok{ch, TokenType::CloseBrace, coord, coord};
      m_tokens.push_back(tok);
    } else if (ch == ';') {
      Token tok{ch, TokenType::Semicolon, coord, coord};
      m_tokens.push_back(tok);
    } else if (ch == '=') {
      Token tok{ch, TokenType::Equal, coord, coord};
      m_tokens.push_back(tok);
    } else if (ch == ',') {
      Token tok{ch, TokenType::Comma, coord, coord};
      m_tokens.push_back(tok);
    } else if (ch == ':') {
      Token tok{ch, TokenType::Colon, coord, coord};
      m_tokens.push_back(tok);
    } else if (ch == '(') {
      Token tok{ch, TokenType::OpenParen, coord, coord};
      m_tokens.push_back(tok);
    } else if (ch == ')') {
      Token tok{ch, TokenType::CloseParen, coord, coord};
      m_tokens.push_back(tok);
    } else if (ch == '.') {
      if (m_hdl_file.peek() != '.') {
        continue;
      }
      (void)m_hdl_file.get();
      TokenCoordinate coord2{
          .row = m_curr_x,
          .col = m_curr_y,
      };

      Token tok{"..", TokenType::RangeOp, coord, coord2};
      m_tokens.push_back(tok);
    } else if (std::isdigit(ch)) {
      auto num = this->lex_number();
      m_tokens.push_back(num);
    } else if (std::isalpha(ch)) {
      auto ident = this->lex_ident();
      m_tokens.push_back(ident);
    }

    ++m_curr_x;
  }

  return m_tokens;
}
}; // namespace hdl

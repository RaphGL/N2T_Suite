#include "lexer.hpp"
#include <cctype>
#include <fstream>
#include <vector>

namespace assembly {

std::vector<Token> Lexer::tokenize() {
  for (;;) {
    auto ch = m_asm_file.get();

    if (m_asm_file.eof()) {
      break;
    }

    if (std::isspace(ch)) {
      if (ch == '\n') {
        Token newline{
          .type = TokenType::Newline,
          .start_coord = { .row = m_curr_y, .col = m_curr_x,},
          .end_coord = { .row = m_curr_y, .col = m_curr_x,},
          .value = ch,
        };
        m_tokens.push_back(newline);

        ++m_curr_y;
        m_curr_x = 0;
      } else {
        ++m_curr_x;
      }
      continue;
    }

    if (ch == '/' && m_asm_file.peek() == '/') {
      this->ignore_comment();
      continue;
    }

    auto type = TokenType::Unknown;
    switch (ch) {
    case 'A':
      type = TokenType::A;
      break;
    case 'M':
      type = TokenType::M;
      break;
    case 'D':
      type = TokenType::D;
      break;
    case '@':
      type = TokenType::AtSymbol;
      break;
    case '+':
      type = TokenType::Plus;
      break;
    case '-':
      type = TokenType::Minus;
      break;
    case '!':
      type = TokenType::Exclamation;
      break;
    case '|':
      type = TokenType::Pipe;
      break;
    case '&':
      type = TokenType::Ampersand;
      break;
    case '(':
      type = TokenType::OpenParen;
      break;
    case ';':
      type = TokenType::Semicolon;
      break;
    case ')':
      type = TokenType::CloseParen;
      break;
    case '=':
      type = TokenType::Equal;
      break;
    }

    TokenCoordinate token_coord{m_curr_y, m_curr_x};
    Token tok{type, token_coord, token_coord, ch};

    if (type == TokenType::Unknown) {
      if (std::isalpha(ch)) {
        tok = this->lex_label();
      }

      if (std::isdigit(ch)) {
        tok = this->lex_number();
      }
    }

    m_tokens.push_back(tok);
    ++m_curr_x;
  }

  auto tok = m_tokens.back();
  return m_tokens;
}

Token Lexer::lex_label() {
  TokenCoordinate start{
      .row = m_curr_y,
      .col = m_curr_x,
  };

  std::string ident{};
  m_asm_file.unget();
  ident += m_asm_file.get();

  while (!m_asm_file.eof()) {
    auto ch = m_asm_file.peek();
    if (!std::isalnum(ch) && ch != '_') {
      break;
    }

    ident += m_asm_file.get();
    ++m_curr_x;
  }

  TokenCoordinate end{
      .row = m_curr_y,
      .col = m_curr_x,
  };

  return Token{
      .type = TokenType::Label,
      .start_coord = start,
      .end_coord = end,
      .value = ident,
  };
}

Token Lexer::lex_number() {
  TokenCoordinate start{
      .row = m_curr_y,
      .col = m_curr_x,
  };

  std::string numstr{};
  m_asm_file.unget();
  numstr += m_asm_file.get();

  while (!m_asm_file.eof() && std::isdigit(m_asm_file.peek())) {
    numstr += m_asm_file.get();
    ++m_curr_x;
  }

  TokenCoordinate end{
      .row = m_curr_y,
      .col = m_curr_x,
  };

  int num{std::stoi(numstr)};

  return Token{
      .type = TokenType::Number,
      .start_coord = start,
      .end_coord = end,
      .value = static_cast<std::size_t>(num),
  };
}

void Lexer::ignore_comment() {
  if (m_asm_file.peek() != '/') {
    return;
  }
  m_asm_file.get();

  while (!m_asm_file.eof() && m_asm_file.peek() != '\n') {
    m_asm_file.get();
    ++m_curr_x;
  }
}

}; // namespace assembly

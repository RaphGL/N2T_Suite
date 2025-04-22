#ifndef BASE_PARSER_HPP
#define BASE_PARSER_HPP

#include "report/report.hpp"
#include <optional>
#include <string>
#include <vector>

// Token and TokenType are the ones used by the language's lexer
template <typename Token, typename TokenType> class BaseParser {
protected:
  std::vector<Token> m_tokens;
  std::size_t m_idx{0};
  std::string m_error_report{""};
  report::Context m_reporter;

  bool eof() const;
  bool peek_expected(TokenType tt) const noexcept;
  Token curr_token() const;
  Token peek() const;
  std::optional<Token> eat() noexcept;
  void emit_error(const Token &tok, std::string_view error);

public:
  std::string get_error_report() const;
  explicit BaseParser(std::vector<Token> tokens, const char *filepath);
};

template <typename Token, typename TokenType>
BaseParser<Token, TokenType>::BaseParser(std::vector<Token> tokens,
                                         const char *filepath)
    : m_tokens{tokens}, m_reporter{filepath} {}

template <typename Token, typename TokenType>
bool BaseParser<Token, TokenType>::eof() const {
  return m_idx >= m_tokens.size();
}

template<typename Token, typename TokenType> 
Token BaseParser<Token, TokenType>::curr_token() const {
  return m_tokens.at(m_idx);
}

template <typename Token, typename TokenType>
bool BaseParser<Token, TokenType>::peek_expected(TokenType tt) const noexcept {
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

template <typename Token, typename TokenType>
Token BaseParser<Token, TokenType>::peek() const {
  return m_tokens.at(m_idx + 1);
}

template <typename Token, typename TokenType>
std::optional<Token> BaseParser<Token, TokenType>::eat() noexcept {
  ++m_idx;
  if (m_idx < m_tokens.size()) {
    return m_tokens[m_idx];
  }

  return std::nullopt;
}

template <typename Token, typename TokenType>
void BaseParser<Token, TokenType>::emit_error(const Token &tok,
                                              std::string_view error) {
  m_reporter.create_report(report::ReportType::Error,
                           report::coord(tok.start_coord),
                           report::coord(tok.end_coord), error);
}

template <typename Token, typename TokenType>
std::string BaseParser<Token, TokenType>::get_error_report() const {
  return m_error_report;
}

#endif

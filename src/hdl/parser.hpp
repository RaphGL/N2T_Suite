#ifndef HDL_PARSER_H
#define HDL_PARSER_H

#include "../base_parser.hpp"
#include "../report/report.hpp"
#include "lexer.hpp"
#include <optional>
#include <string>
#include <vector>

namespace hdl {

struct Range {
  std::size_t from, to;
  TokenCoordinate start_coord, end_coord;
};

struct Arg {
  std::string name;
  std::optional<Range> range;
  std::string output;
  TokenCoordinate start_coord, end_coord;
};

struct Part {
  std::string name;
  std::vector<Arg> args;
  TokenCoordinate start_coord, end_coord;
};

struct InOut {
  // output if false
  bool input;
  std::string name;
  std::size_t size;
  TokenCoordinate start_coord, end_coord;
};

struct Chip {
  std::string name;
  std::vector<Part> parts;
  std::vector<InOut> inouts;
};

class Parser final : public BaseParser<Token, TokenType> {
  std::optional<Chip> parse_chip();
  std::optional<std::vector<InOut>> parse_inout();
  std::optional<Part> parse_part();
  std::optional<Arg> parse_arg();
  std::optional<Range> parse_range();

public:
  using BaseParser::BaseParser;
  std::optional<std::vector<Chip>> parse();
};

} // namespace hdl

#endif

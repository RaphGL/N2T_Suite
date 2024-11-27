#ifndef REPORT_REPORT_HPP
#define REPORT_REPORT_HPP

#include <fstream>
#include <optional>
#include <vector>

namespace report {

struct Coord {
  std::size_t col, row;
};

template <typename T>
concept convertible_to_report_coord = requires(T a) {
  std::is_integral<T>();
  { a.col };
  { a.row };
};

Coord coord(convertible_to_report_coord auto foreign_coord) {
  return Coord{
      .col = foreign_coord.col,
      .row = foreign_coord.row,
  };
}

enum class ReportType { Error, Warning };

struct Report {
  std::size_t start_col, end_col;
  std::size_t start_row, end_row;
  std::string error;
  ReportType type;
};

class Context final {
  std::ifstream m_file;
  std::vector<Report> m_reports;
  std::string m_filepath;

public:
  explicit Context(const char *filepath);
  void create_report(ReportType type, Coord start, Coord end,
                     std::string_view error_msg);

  std::optional<std::string> generate_final_report();
};

}; // namespace report

#endif

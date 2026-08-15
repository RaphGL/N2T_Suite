#ifndef REPORT_REPORT_HPP
#define REPORT_REPORT_HPP

#include <filesystem>
#include <fstream>
#include <variant>
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
   return Coord {
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
   std::vector<Report> m_reports;
   std::filesystem::path m_filepath;
   std::istringstream m_stream;
   std::variant<std::filesystem::path, std::string_view> m_contents;

   public:
   explicit Context(const std::filesystem::path filepath);
   explicit Context(std::string_view contents);
   void create_report(ReportType type, Coord start, Coord end, std::string_view error_msg);

   std::optional<std::string> generate_final_report();
};

}; // namespace report

#endif

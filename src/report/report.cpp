#include "report.hpp"
#include <algorithm>
#include <format>
#include <fstream>
#include <optional>
#include <vector>

namespace report {

Context::Context(const char *filepath)
    : m_file{filepath}, m_filepath{filepath} {}

void Context::create_report(ReportType type, Coord start, Coord end,
                            std::string_view error_msg) {
  Report rep{
      .start_col = start.col,
      .end_col = end.col,
      .start_row = start.row,
      .end_row = end.row,
      .error = std::string(error_msg),
      .type = type,
  };

  m_reports.push_back(rep);
}

std::string generate_individual_report(std::string_view filepath,
                                       std::string_view line, Report report) {
  auto error_type_str = "";
  switch (report.type) {
  case report::ReportType::Error:
    error_type_str = "error";
    break;
  case report::ReportType::Warning:
    error_type_str = "warning";
    break;
  }

  // editors are 1-indexed so we have to display this
  auto editor_col = report.start_col + 1;
  auto editor_row = report.start_row + 1;

  std::string report_str =
      std::format("-> {} on {}:{}:{}\n", error_type_str, filepath,
                  editor_row, editor_col);

  const auto rownum = std::to_string(editor_row);
  std::string row_padding{};
  for (std::size_t i = 0; i < rownum.size(); i++) {
    row_padding += ' ';
  }

  const auto empty_row = std::format(" {} | ", row_padding);
  report_str += empty_row + '\n';

  // line where error occurred
  report_str += std::format(" {} | {}\n", rownum, line);

  // arrow pointing to where error occurred
  report_str += empty_row;
  for (std::size_t i = 0; i < report.start_col; i++) {
    report_str += ' ';
  }
  report_str += '^';

  if (report.end_col - report.start_col > 0) {
    for (std::size_t i = 0; i < report.end_col - report.start_col; i++) {
      report_str += '-';
    }

    report_str += '^';
  }
  report_str += '\n';

  // error that occurred
  report_str += empty_row;
  for (std::size_t i = 0; i < report.start_col / 2; i++) {
    report_str += ' ';
  }

  report_str += report.error + '\n';

  report_str += empty_row + '\n';

  return report_str;
}

std::optional<std::string> Context::generate_final_report() {
  std::string final_report{};

  std::sort(m_reports.begin(), m_reports.end(),
            [](Report a, Report b) { return a.start_row < b.start_row; });

  std::string current_line{};
  std::size_t curr_row{0}, curr_col{0}, curr_report{0};
  while (!m_file.eof() && curr_report < m_reports.size()) {
    current_line = "";
    std::getline(m_file, current_line);
    curr_col = 0;

    for (auto _ : current_line) {
      auto report = m_reports.at(curr_report);

      if (curr_row == report.start_row && curr_col == report.start_col) {
        final_report.append(
            generate_individual_report(m_filepath, current_line, report));

        ++curr_report;
        if (curr_report >= m_reports.size()) {
          break;
        }
      }

      ++curr_col;
    }

    ++curr_row;
  }

  if (m_reports.empty()) {
    return std::nullopt;
  } else {
    return final_report;
  }
}

}; // namespace report

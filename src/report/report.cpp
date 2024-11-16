#include "report.hpp"
#include <algorithm>
#include <fstream>
#include <optional>
#include <vector>

namespace report {

Context::Context(const char *filepath) : m_file{filepath} {}

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

std::string generate_individual_report(std::string_view line, Report report) {
  constexpr std::string_view error_str = "Error: ";
  constexpr std::string_view warning_str = "Warning: ";

  std::string_view error_type_str;
  switch (report.type) {
  case ReportType::Error:
    error_type_str = error_str;
    break;
  case ReportType::Warning:
    error_type_str = warning_str;
    break;
  }

  std::string report_str{error_type_str};
  report_str += std::string(line) + "\n";

  std::string left_padding{};
  left_padding.insert(0, error_type_str.length() + report.start_col, ' ');
  report_str += left_padding + '^' + '\n';

  left_padding = std::string();
  auto aligned_padding_len =
      error_type_str.length() + report.start_col - report.error.length() / 2;
  if (aligned_padding_len > report.error.length()) {
    aligned_padding_len = 0;
  }

  left_padding.insert(0, aligned_padding_len, ' ');
  report_str += left_padding + report.error + "\n\n";
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
        final_report.append(generate_individual_report(current_line, report));

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

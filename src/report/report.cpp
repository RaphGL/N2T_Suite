#include "report.hpp"
#include <algorithm>
#include <format>
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
  constexpr std::string_view error_str = "error";
  constexpr std::string_view warning_str = "warning";

  std::string_view error_type_str;
  switch (report.type) {
  case ReportType::Error:
    error_type_str = error_str;
    break;
  case ReportType::Warning:
    error_type_str = warning_str;
    break;
  }

  ++report.start_col;
  ++report.start_row;

  std::string report_str = std::format("--> {} {}:{}\n", error_type_str,
                                       report.start_row, report.start_col);
  auto row = std::to_string(report.start_row);
  std::string tab_pad{};
  tab_pad.insert(0, 5 + row.length(), ' ');
  tab_pad[1 + row.length()] = '|';
  report_str += tab_pad + '\n';
  report_str += row + " |  " + std::string(line) + "\n";

  std::string left_padding{};
  auto aligned_padding_len = report.start_col;
  left_padding.insert(0, aligned_padding_len, ' ');

  report_str += tab_pad + left_padding + '^' + '\n';

  left_padding = std::string();
  aligned_padding_len = report.start_col - report.error.length() / 2;
  if (aligned_padding_len >= line.length()) {
    aligned_padding_len = 0;
  }
  left_padding.insert(0, aligned_padding_len + row.length(), ' ');

  report_str += tab_pad + left_padding + report.error + '\n';
  report_str += tab_pad + "\n\n";

  std::replace(report_str.begin(), report_str.end(), '\t', ' ');
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

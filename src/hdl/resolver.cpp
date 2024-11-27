#include "../report/report.hpp"
#include "parser.hpp"
#include <string_view>
#include <unordered_map>
#include <vector>

namespace hdl {
enum class PinType {
  Input,
  Output,
  Internal,
};

struct Pin {
  PinType type;
  std::vector<char> bits;
};

class ChipResolver {
  report::Context m_reports;
  std::unordered_map<std::string_view, Pin> m_pins{};
  const Chip m_chip;

public:
  explicit ChipResolver(Chip chip, const char *filepath);
  std::vector<Pin> resolve();
};

ChipResolver::ChipResolver(Chip chip, const char *filepath)
    : m_reports{filepath}, m_chip{chip} {
  Pin pin_entry{};
  for (const auto &pin : chip.inouts) {
    pin_entry.type = pin.input ? PinType::Input : PinType::Output;
    m_pins.insert({pin.name, pin_entry});
  }
}

std::vector<Pin> ChipResolver::resolve() {
  for (const auto &part : m_chip.parts) {   
  }
}
}; // namespace hdl

#include "parser.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <unordered_map>

namespace hdl {

struct Pin {
  std::size_t size;
  std::uint16_t val;
  TokenCoordinate start_coord, end_coord;
};

using PinMap = std::unordered_map<const char *, Pin>;

namespace {
int16_t get_n_high_bits(int8_t n) {
  if (n == 64) {
    return -((uint16_t)1);
  }
  return (((uint16_t)1) << n) - 1;
}

bool parse_args_to_map(std::unordered_map<const char *, uint16_t> &internal_pins,
                  std::vector<Arg> args) {
  bool success{true};

  for (const auto &arg : args) {
    if (arg.name != "a" || arg.name != "b" || arg.name != "out") {
      // todo: emit error
      success = false;
      continue;
    }

    std::uint16_t val{};
    if (arg.output == "false") {
      val = 0;
    } else if (arg.output == "true") {
      // overflows on purpose to fill with 1s
      val = INT16_MAX;
    } else {
      val = internal_pins[arg.output.c_str()];
    }

    // todo: add range support for output
    if (arg.range.has_value()) {
      auto range = arg.range.value();
      size_t range_span = range.to - range.from;

      if (range_span > 16 || range_span < 0) {
        // todo: emit error
        success = false;
        continue;
      }

      int16_t bitflag = get_n_high_bits(range_span);
      val = (val >> range.from) & bitflag;
    } else {
    }

    // assign final result
    internal_pins[arg.name.c_str()] = val;
  }

  return success;
}

// todo: finish it and then extract the arg parsing logic into its own function
std::optional<std::unordered_map<const char *, uint16_t>>
builtin_nand(PinMap &internal_pins, std::vector<Arg> args) {
  std::unordered_map<const char *, uint16_t> chip_pins{};
  if (!parse_args_to_map(chip_pins, args)) {
    return {};
  };

  chip_pins["out"] = chip_pins["a"] & chip_pins["b"];
  return chip_pins;
}

// returns a map of the outputs
using BuiltinFunction =
    std::function<std::optional<std::unordered_map<const char *, uint16_t>>(
        PinMap &, std::vector<Arg>)>;

std::unordered_map<const char *, BuiltinFunction> builtin_chips{
    {"Nand", builtin_nand}};
}; // namespace

class ChipResolver {
  PinMap m_in{};
  PinMap m_out{};
  std::vector<Part> m_parts;
  const char *m_filepath{nullptr};

public:
  ChipResolver(const char *filepath, Chip chip)
      : m_parts{chip.parts}, m_filepath{filepath} {
    for (const auto &pin : chip.inouts) {
      Pin new_pin = {
          .size = pin.size,
          .val = {},
          .start_coord = pin.start_coord,
          .end_coord = pin.end_coord,
      };

      if (pin.input) {
        m_in[pin.name.c_str()] = new_pin;
      } else {
        m_out[pin.name.c_str()] = new_pin;
      }
    }
  }

  PinMap get_outputs() { return m_out; }

  void resolve() {
    PinMap internal_pins{};

    for (const auto &part : m_parts) {
      // todo: check for local implementation before using builtin
      const BuiltinFunction chip_func = builtin_chips[part.name.c_str()];
    }
  }
};

void resolve() {}
}; // namespace hdl

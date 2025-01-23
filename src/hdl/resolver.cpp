#include "parser.hpp"
#include <unordered_map>
#include <functional>
#include <optional>


namespace hdl {

struct Pin {
  std::size_t size;
  std::vector<bool> val;
  TokenCoordinate start_coord, end_coord;
};

using PinMap = std::unordered_map<const char *, Pin>;

namespace {
  // todo: finish it and then extract the arg parsing logic into its own function
  std::optional<PinMap> builtin_nand(PinMap &internal_pins, std::vector<Arg> args) {
    PinMap chip_pins{};

    bool had_error{false};

    for (const auto &arg: args) {
      if (arg.name != "a" || arg.name != "b" || arg.name != "out") {
        // emit error
        had_error=false;
        continue;
      }

      std::vector<bool> val{};
      // todo: parse numbers
      if (arg.output == "false" || arg.output == "0") {

      } else if (arg.output == "true" || arg.output == "1") {
        
      } else {        

      }

      // todo: parse range numbers
      if (arg.range.has_value()) {
        
      } else {
        
      }

      // todo: implement calculating size for input and verify that output fits in 

      // assign final result
      chip_pins[arg.name.c_str()] = {
        .val = val,
        .start_coord = arg.start_coord,
        .end_coord = arg.end_coord,
      }; 
    }

    if (had_error) {
      return {};
    }

    return chip_pins;
  }

  // returns a map of the outputs
  using BuiltinFunction = std::function<std::optional<PinMap>(PinMap&, std::vector<Arg>)>;

  std::unordered_map<const char *, BuiltinFunction> builtin_chips{
    {"Nand", builtin_nand }
  };
};

class ChipResolver {
  PinMap m_in{};
  PinMap m_out{};
  std::vector<Part> m_parts;
  const char *m_filepath{nullptr};

public:
  ChipResolver(const char *filepath, Chip chip) :  m_parts{chip.parts}, m_filepath{filepath} {
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

  PinMap get_outputs() {
    return m_out;
  }

  void resolve() {
    PinMap internal_pins{};

    for (const auto &part : m_parts) {
      // todo: check for local implementation before using builtin
      const BuiltinFunction chip_func = builtin_chips[part.name.c_str()];
    }
    
  }
};

void resolve() {
}
}; // namespace hdl

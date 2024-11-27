#include <string_view>
#include <unordered_map>
#include <vector>
#include "../report/report.hpp"

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
    std::unordered_map<std::string_view, Pin> pins;

    public:
      ChipResolver();
      std::vector<Pin> resolve();
  };

  
};

#ifndef N2T_WIDGET_MEMORY_VIEW_HPP
#define N2T_WIDGET_MEMORY_VIEW_HPP

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string_view>

namespace gui::widget {

class MemoryViewer {
   std::span<std::uint16_t> _memory;
   std::uint16_t &_curr_addr;
   std::optional<std::uint16_t> _next_scroll = std::nullopt;

   public:
   MemoryViewer(std::span<std::uint16_t> memory, std::uint16_t &curr_mem_addr);

   bool show_active_address = false;

   void set_scroll(int row);

   // `render_memory_address` should render a memory address in however way is fit.
   // for example it might dissamble memory to show the assembler, or it might convert memory into
   // virtual machine bytecode representations or it might allow choosing between different integer
   // representations like binary, octal, etc
   void show(std::string_view label, int default_height,
       std::function<void(std::uint16_t)> render_memory_address);
};

}

#endif

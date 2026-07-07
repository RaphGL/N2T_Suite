#ifndef GUI_WIDGETS_CPP
#define GUI_WIDGETS_CPP

#include <SDL3/SDL.h>
#include <filesystem>
#include <optional>
#include "hack/hack.hpp"

namespace fs = std::filesystem;

namespace gui {

enum class MemoryViewType : int {
   ROM,
   RAM,
};

class GuiContext final {
   SDL_Window *_window;
   std::optional<fs::path> _dialog_path = std::nullopt;
   bool _dialog_requested = false;

   Hack _hack{};
   std::string _program = "";
   std::string _script = "";

   void load_program();

   public:
   GuiContext(SDL_Window *window)
       : _window(window) { }
   void show_memory_view(MemoryViewType type);
   void show_top_bar();

   // asynchronously get files from the OS' native dialog
   void dialog_request_file();
   bool dialog_ready() const;
   // NOTE: this function should only be called if `dialog_ready()` is true
   fs::path dialog_get_file();
};

}

#endif

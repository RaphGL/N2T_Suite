#ifndef GUI_WIDGETS_CPP
#define GUI_WIDGETS_CPP

#include "hack/hack.hpp"
#include <SDL3/SDL.h>
#include <filesystem>
#include <mutex>
#include <optional>
#include <thread>

namespace fs = std::filesystem;

namespace gui {

enum class MemoryViewType : int {
   ROM = 0,
   RAM = 1,
};

class GuiContext final {
   SDL_Window *_window;
   std::atomic<bool> _running = true;

   std::optional<fs::path> _dialog_path = std::nullopt;
   std::atomic<bool> _dialog_requested = false;
   std::jthread _dialog_worker;
   std::mutex _dialog_mutex;

   Hack _hack { };
   std::string _program = "";
   std::string _script = "";

   void clear_hack_memory(MemoryViewType type);

   // asynchronously get files from the OS' native dialog
   void dialog_request_file();
   bool dialog_ready() const;
   // NOTE: this function should only be called if `dialog_ready()` is true
   std::optional<fs::path> dialog_get_file();

   public:
   explicit GuiContext(SDL_Window *window);
   void set_styling();

   void show_memory_view(MemoryViewType type);
   void show_menu_bar();
   void show_top_bar();

   void load_program();
};

}

#endif

#ifndef GUI_WIDGETS_CPP
#define GUI_WIDGETS_CPP

#include "hack/hack.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
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

enum class HackState {
   Running,
   Stopped,
   StepThrough,
   Reset,
};

enum class LogType {
   Error,
   Success,
};

struct LogMessage {
   LogType type;
   std::string msg;
};

class GuiContext final {
   SDL_Window *_window;
   GLuint _hack_screen_tex = -1;

   std::vector<LogMessage> _logs;
   std::mutex _logs_mutex;

   // ==== Virtual Machine Runtime
   Hack _hack { };
   std::atomic<HackState> _hack_state = HackState::Stopped;
   std::atomic<std::optional<SDL_Keycode>> _hack_key;
   // how fast the processor runs
   float _hack_speed = 1.0f;
   std::jthread _hack_worker;

   // ==== DIALOG API
   std::optional<fs::path> _dialog_path = std::nullopt;
   std::atomic<bool> _dialog_requested = false;
   std::jthread _dialog_worker;
   std::mutex _dialog_mutex;
   // asynchronously get files from the OS' native dialog
   void dialog_request_file();
   bool dialog_ready() const;
   std::optional<fs::path> dialog_get_file();

   void clear_hack_memory(MemoryViewType type);

   public:
   explicit GuiContext(SDL_Window *window);
   ~GuiContext();

   void set_styling();
   void set_keyboard_input(std::optional<SDL_Keycode> key);
   void push_log(LogType type, const char *msg);
   void load_program();

   // ==== Widgets API
   void show_memory_view(MemoryViewType type);
   void show_menu_bar();
   void show_top_bar();
   void show_hack_screen();
   void show_modal_message(const char *name, const char *msg);
   void show_logs();
};
}

#endif

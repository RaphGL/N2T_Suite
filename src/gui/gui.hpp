#ifndef N2T_GUI_HPP
#define N2T_GUI_HPP

#include "imgui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
namespace chrono = std::chrono;

namespace gui {

constexpr int FRAME_PER_SECOND = 60;
constexpr auto TIME_PER_FRAME = chrono::milliseconds(1000 / FRAME_PER_SECOND);

// begins tracking of frame time
void start_frame();
// sleeps thread until frame time is over
void end_frame();

consteval ImVec4 rgba_to_imvec4(uint32_t rgba) {
   float r = rgba >> 24 & 0xFF;
   float g = rgba >> 16 & 0xFF;
   float b = rgba >> 8 & 0xFF;
   float a = rgba >> 0 & 0xFF;
   return ImVec4(r / 0xFF, g / 0xFF, b / 0xFF, a / 0xFF);
}

consteval ImVec4 rgb_to_imvec4(uint32_t rgb) { return rgba_to_imvec4(rgb << 8 | 0x000000FF); }

namespace Color {
   constexpr auto RED = rgb_to_imvec4(0xFF5555);
   constexpr auto GREEN = rgb_to_imvec4(0x50FA7B);
}

class BaseView {
   public:
   virtual void show() = 0;
   virtual std::string_view view_name() const = 0;
   virtual ~BaseView() = default;
};

class Context final {
   SDL_Window *_window;
   std::vector<std::unique_ptr<BaseView>> _views;

   std::optional<fs::path> _dialog_path = std::nullopt;
   std::atomic<bool> _dialog_requested = false;
   std::jthread _dialog_worker;
   std::mutex _dialog_mutex;

   public:
   explicit Context(SDL_Window *window);

   ImFont *monofont = nullptr;
   // keyboard key captured
   std::atomic<std::optional<SDL_Keycode>> key;
   void set_styling();

   // ==== DIALOG API
   // asynchronously get files from the OS' native dialog
   // note: you need to poll API to get the file out
   void dialog_request_file();
   bool dialog_ongoing() const;
   bool dialog_ready() const;
   std::optional<fs::path> dialog_get_file();

   // ==== Global Widgets
   void show_menu_bar();
   void show_modal_message(const char *name, const char *msg);

   void show();
};
}

#endif

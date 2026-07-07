#include "gui_widgets.hpp"
#include "hack/hack.hpp"
#include "imgui.h"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <array>

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>
#include <thread>

namespace fs = std::filesystem;
namespace chrono = std::chrono;

namespace gui {

GuiContext::GuiContext(SDL_Window *window)
    : _window { window } {
   {
      glGenTextures(1, &_hack_screen_tex);
      glBindTexture(GL_TEXTURE_2D, _hack_screen_tex);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glBindTexture(GL_TEXTURE_2D, 0);
   }

   _hack_worker = std::jthread([this](std::stop_token token) {
      constexpr int ticks_per_frame = 1400000 / 60;
      while (!token.stop_requested()) {
         auto frame_start = chrono::high_resolution_clock::now();
         switch (_hack_state) {
         case HackState::Stopped:
            std::this_thread::sleep_for(chrono::milliseconds(30));
            break;

         case HackState::Running: {
            auto key = _hack_key.load(std::memory_order_seq_cst);
            auto &keyboard_mem = _hack.get_keyboard_mmap();
            if (key.has_value()) {
               keyboard_mem = convert_input_to_hack(key.value());
            } else {
               keyboard_mem = 0;
            }

            for (int i = 0; i < ticks_per_frame * _hack_speed; i++) {
               _hack.tick();
            }
         } break;

         case HackState::StepThrough:
            _hack.tick();
            _hack_state = HackState::Stopped;
            std::this_thread::sleep_for(chrono::milliseconds(30));
            break;

         case HackState::Reset:
            _hack.pc = 0;
            _hack_state = HackState::Stopped;
            break;
         }

         auto frame_end = chrono::milliseconds(0);
         while (frame_end < chrono::milliseconds(16)) {
            frame_end = chrono::duration_cast<chrono::milliseconds>(
                chrono::high_resolution_clock::now() - frame_start);
         }
      }
   });

   _dialog_worker = std::jthread([this](std::stop_token token) {
      while (!token.stop_requested()) {
         if (!dialog_ready()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
            continue;
         }

         // TODO: detect and compile if file is not raw hack bytes
         // otherwise this thread crashes silently and dialog stops working
         auto file = dialog_get_file();
         if (!file.has_value()) {
            continue;
         }

         std::ifstream filestream { file.value() };
         std::stringstream contents;
         contents << filestream.rdbuf();
         if (!_hack.load_rom(std::move(contents.str()))) {
            // TODO show error when file is not in a valid format
         }
      }
   });
}

GuiContext::~GuiContext() { glDeleteTextures(1, &_hack_screen_tex); }

void GuiContext::set_keyboard_input(std::optional<SDL_Keycode> key) { _hack_key = key; }

void GuiContext::set_styling() {
   const char *default_font_path =
#ifdef _WIN32
       "C:/Windows/Fonts/arial.ttf";
#elif __APPLE__
       "/System/Library/Fonts/Helvetica.ttc";
#else
       "/usr/share/fonts/TTF/DejaVuSans.ttf";
#endif

   auto &io = ImGui::GetIO();
   io.Fonts->AddFontFromFileTTF(default_font_path, 16);
}

void GuiContext::dialog_request_file() {
   SDL_DialogFileCallback callback = [](void *userdata, const char *const *filelist, int _) {
      auto gc = static_cast<GuiContext *>(userdata);

      if (filelist && *filelist) {
         gc->_dialog_mutex.lock();
         gc->_dialog_path = *filelist;
         gc->_dialog_mutex.unlock();
      }

      gc->_dialog_requested = false;
   };

   _dialog_mutex.lock();
   _dialog_path = std::nullopt;
   _dialog_mutex.unlock();
   _dialog_requested = true;
   SDL_ShowOpenFileDialog(callback, this, _window, NULL, 0, NULL, false);
}

bool GuiContext::dialog_ready() const { return !_dialog_requested && _dialog_path.has_value(); }

std::optional<fs::path> GuiContext::dialog_get_file() {
   _dialog_mutex.lock();
   auto filepath = _dialog_path;
   _dialog_path = std::nullopt;
   _dialog_mutex.unlock();
   return filepath;
}

// TODO: actually allow choosing the type when requesting dialog
void GuiContext::load_program() {
   if (!_dialog_requested) {
      dialog_request_file();
   }
}

void GuiContext::show_top_bar() {
   ImGui::BeginGroup();
   if (ImGui::BeginChild("top-bar", ImVec2(0, 40), ImGuiChildFlags_Borders)) {
      if (ImGui::Button("Load Program")) {
         load_program();
      }
      ImGui::SameLine();
      if (ImGui::Button("Single Step")) {
         _hack_state = HackState::StepThrough;
      }
      ImGui::SameLine();
      if (ImGui::Button("Run")) {
         _hack_state = HackState::Running;
      }
      ImGui::SameLine();
      if (ImGui::Button("Stop")) {
         _hack_state = HackState::Stopped;
      }
      ImGui::SameLine();
      if (ImGui::Button("Reset")) {
         _hack_state = HackState::Reset;
      }
      ImGui::SameLine();
      ImGui::Button("Load Script");

      ImGui::SameLine();
      ImGui::Text("CPU Speed:");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      if (ImGui::SliderFloat("##program-speed", &_hack_speed, 0.5f, 2.0f, "%.1f")) {
         constexpr float step = 0.1f;
         _hack_speed = std::round(_hack_speed / step) * step;
      }
   }
   ImGui::EndChild();
   ImGui::EndGroup();
}

void GuiContext::clear_hack_memory(MemoryViewType type) {
   switch (type) {
   case MemoryViewType::RAM:
      std::fill(_hack.data_mem.begin(), _hack.data_mem.end(), 0);
      break;
   case MemoryViewType::ROM:
      std::fill(_hack.instruction_mem.begin(), _hack.instruction_mem.end(), 0);
      break;
   }
}

void GuiContext::show_memory_view(MemoryViewType type) {
   std::string_view label;
   switch (type) {
   case MemoryViewType::RAM:
      label = "RAM";
      break;

   case MemoryViewType::ROM:
      label = "ROM";
      break;
   }

   ImGui::BeginGroup();
   {
      ImGui::BeginGroup();
      ImGui::Text("%s", label.data());
      ImGui::SameLine();

      ImGui::PushID(label.data());
      if (type != MemoryViewType::RAM && ImGui::Button("Load")) {
         load_program();
      }
      ImGui::SameLine();
      if (ImGui::Button("Clear")) {
         clear_hack_memory(type);
      }
      ImGui::SameLine();
      if (ImGui::Button("Search")) {
         // TODO
      }

      static std::array<std::size_t, 2> curr_view_opt = { 0, 1 };
      std::array<std::string_view, 4> view_options {
         "assembly",
         "binary",
         "decimal",
         "hexademical",
      };
      ImGui::SameLine();
      ImGui::SetNextItemWidth(-FLT_MIN);
      if (ImGui::BeginCombo(
              "##view-options", view_options.at(curr_view_opt.at(static_cast<int>(type))).data())) {
         std::size_t start_idx = type == MemoryViewType::RAM ? 1 : 0;
         for (std::size_t i = start_idx; i < view_options.size(); i++) {
            bool is_selected = curr_view_opt.at(static_cast<int>(type)) == i;
            if (ImGui::Selectable(view_options[i].data(), is_selected)) {
               curr_view_opt[static_cast<int>(type)] = i;
            }

            if (is_selected) {
               ImGui::SetItemDefaultFocus();
            }
         }

         ImGui::EndCombo();
      }
      ImGui::PopID();
      ImGui::EndGroup();

      if (ImGui::BeginTable(label.data(), 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter,
              ImVec2(0, 350))) {
         ImGui::TableSetupColumn("##address", ImGuiTableColumnFlags_WidthFixed, 50);
         ImGui::TableSetupColumn("##memory-contents", ImGuiTableColumnFlags_WidthStretch);

         std::span<std::uint16_t> hack_mem { };
         switch (type) {
         case MemoryViewType::RAM:
            hack_mem = _hack.data_mem;
            break;
         case MemoryViewType::ROM:
            hack_mem = _hack.instruction_mem;
            break;
         }

         // necessary otherwise the performance would crawl with 32K items to render
         ImGuiListClipper clipper;
         clipper.Begin(hack_mem.size());
         while (clipper.Step()) {
            for (auto i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
               ImGui::TableNextRow();

               ImGui::PushID(i);
               ImGui::TableNextColumn();
               ImGui::Text("%d", static_cast<int>(i));

               ImGui::TableNextColumn();
               ImGui::SetNextItemWidth(-FLT_MIN);
               ImGui::InputScalarN("##mem_address", ImGuiDataType_U16, &hack_mem[i], 1);
               ImGui::PopID();
            }
         }
         ImGui::EndTable();
      }
   }
   ImGui::EndGroup();
}

void GuiContext::show_menu_bar() {
   if (ImGui::BeginMenuBar()) {
      if (ImGui::BeginMenu("File")) {
         if (ImGui::MenuItem("Load Program")) {
            load_program();
         }
         ImGui::MenuItem("Load Test Script");
         ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Help")) {
         ImGui::MenuItem("Usage");
         ImGui::MenuItem("About");
         ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
   }
}

void GuiContext::show_hack_screen() {
   auto screen = _hack.get_screen_mmap();

   std::array<GLuint, 512 * 256> pixels;
   std::fill(pixels.begin(), pixels.end(), 0x000000FF);

   for (std::size_t y = 0; y < 256; ++y) {
      for (std::size_t x_chunk = 0; x_chunk < 32; ++x_chunk) {
         std::uint16_t chunk = screen[y * 32 + x_chunk];
         for (std::size_t i = 0; i < 16; ++i) {
            if (chunk & (1 << i)) {
               pixels.at(y * 512 + (x_chunk * 16 + i)) = 0xFFFFFFFF;
            }
         }
      }
   }

   glBindTexture(GL_TEXTURE_2D, _hack_screen_tex);
   glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 512, 256, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

   ImVec2 screen_size = ImVec2(0, 0);
   ImVec2 avail_size = ImGui::GetContentRegionAvail();
   screen_size.x = avail_size.x;
   screen_size.y = avail_size.y * 0.6f;

   float padding_x = (avail_size.x - screen_size.x) / 2;
   if (padding_x > 0.0f) {
      ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding_x);
   }
   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2, 2));

   if (ImGui::BeginChild(
           "##hack-screen", ImVec2(screen_size.x, screen_size.y + 5), ImGuiChildFlags_Borders)) {
      ImGui::Image(_hack_screen_tex, screen_size, ImVec2(0, 0), ImVec2(1, 1));
   }
   ImGui::EndChild();

   ImGui::PopStyleVar();
   ImGui::SetCursorPosX(ImGui::GetCursorPosX() + padding_x);
   ImGui::Dummy(ImVec2(0, 10));
}

}

#include "gui_widgets.hpp"
#include "asm/codegen.hpp"
#include "asm/lexer.hpp"
#include "asm/parser.hpp"
#include "hack/hack.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <algorithm>
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
         try {
            switch (_hack_state) {
            case HackState::Off:
               [[fallthrough]];
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
         } catch (std::out_of_range) {
            _hack_state = HackState::Stopped;
            push_log(LogType::Error, "Failed to run. Check if you have a valid program loaded.");
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

         auto file = dialog_get_file();
         if (!file.has_value()) {
            continue;
         }

         fs::path filepath = file.value();
         auto file_ext = filepath.extension();
         if (file_ext == ".asm") {
            assembly::Lexer lexer { filepath };
            auto tokens = lexer.tokenize();
            if (tokens.empty()) {
               push_log(LogType::Error, "Failed to lex file.");
               continue;
            }
            assembly::Parser parser { tokens, filepath };
            auto ast = parser.parse();
            if (!ast.has_value()) {
               auto report = parser.get_error_report();
               push_log(LogType::Error, report.data());
               continue;
            }
            assembly::CodeGen codegen { ast.value(), filepath };
            auto machine_code = codegen.compile();
            if (!machine_code.has_value()) {
               auto report = codegen.get_error_report();
               push_log(LogType::Error, report.data());
               continue;
            }

            _hack.load_rom(machine_code.value());
            push_log(LogType::Success, "ROM Loaded.");
            _hack_state = HackState::Stopped;
         } else if (file_ext == ".jack") {
            // TODO: compile once compiler is made
         } else if (file_ext == ".vm") {
            // TODO: compile once the vm translator is made
         } else if (file_ext == ".hack") {
            std::ifstream filestream { filepath };
            std::stringstream contents;
            contents << filestream.rdbuf();
            if (!_hack.load_rom(std::move(contents.str()))) {
               push_log(LogType::Error,
                   "Failed to load Hack ROM, please check that the file contains valid hack "
                   "machine code.");
            }
            _hack_state = HackState::Stopped;
         } else {
            push_log(LogType::Error, "File contains an invalid extension.");
         }
      }
   });
}

GuiContext::~GuiContext() { glDeleteTextures(1, &_hack_screen_tex); }

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
};

void GuiContext::set_keyboard_input(std::optional<SDL_Keycode> key) { _hack_key = key; }

// TODO: embed default fonts in application
static ImFont *MONOFONT = nullptr;
void GuiContext::set_styling() {
   constexpr auto default_font_path =
#ifdef _WIN32
       "C:/Windows/Fonts/arial.ttf";
#elif __APPLE__
       "/System/Library/Fonts/Helvetica.ttc";
#else
       "/usr/share/fonts/TTF/DejaVuSans.ttf";
#endif

   constexpr auto mono_font =
#ifdef _WIN32
       "C:/Windows/Fonts/consola.ttf";
#elif __APPLE__
       "/System/Library/Fonts/Monaco.ttf";
#else
       "/usr/share/fonts/TTF/DejaVuSansMono.ttf";
#endif
   auto &io = ImGui::GetIO();
   io.Fonts->AddFontFromFileTTF(default_font_path, 16);
   MONOFONT = io.Fonts->AddFontFromFileTTF(mono_font, 16.0f);
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

void GuiContext::load_program() {
   if (!_dialog_requested) {
      dialog_request_file();
   }
}

void GuiContext::show_top_bar() {
   ImGui::BeginGroup();
   if (ImGui::BeginChild("top-bar", ImVec2(0, 40))) {
      if (ImGui::Button("Load Program")) {
         load_program();
      }
      ImGui::SameLine();
      if (ImGui::Button("Single Step")) {
         _hack_state = HackState::StepThrough;
      }
      ImGui::SameLine();
      if (ImGui::Button(_hack_state != HackState::Running ? "Run" : "Stop")) {
         _hack_state = _hack_state == HackState::Running ? HackState::Stopped : HackState::Running;
      }
      ImGui::SameLine();
      if (ImGui::Button("Reset")) {
         _hack_state = HackState::Reset;
      }
      ImGui::SameLine();
      ImGui::Button("Load Script");

      ImGui::SameLine();
      ImGui::TextUnformatted("CPU Speed:");
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
   case MemoryViewType::Count:
      break;
   }
}

void GuiContext::set_memory_view_scroll(int row) {
   auto curr_style = ImGui::GetStyle();
   auto row_height = ImGui::GetTextLineHeightWithSpacing() + curr_style.CellPadding.y
       + curr_style.ItemSpacing.y;
   auto view_point = row;
   if (view_point > 3) {
      view_point -= 3;
   }
   ImGui::SetScrollY(row_height * view_point);
}

void GuiContext::show_memory_view(MemoryViewType type, int default_height) {
   std::string_view label;
   switch (type) {
   case MemoryViewType::RAM:
      label = "RAM";
      break;
   case MemoryViewType::ROM:
      label = "ROM";
      break;
   case MemoryViewType::Count:
      break;
   }

   ImGui::BeginGroup();
   {
      ImGui::BeginGroup();
      ImGui::PushID(label.data());
      if (type != MemoryViewType::RAM) {
         if (ImGui::Button("Load")) {
            load_program();
         }
         ImGui::SameLine();
      }
      if (ImGui::Button("Clear")) {
         clear_hack_memory(type);
      }
      ImGui::SameLine();
      if (ImGui::Button("Search")) {
         // TODO
      }

      static std::array<std::size_t, static_cast<int>(MemoryViewType::Count)> curr_view_opt
          = { 0, 1 };
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
      auto buttons_height = ImGui::GetItemRectSize().y + ImGui::GetStyle().ItemSpacing.y;

      if (ImGui::BeginTable(label.data(), 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter,
              ImVec2(0, default_height - buttons_height))) {
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
         case MemoryViewType::Count:
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

               int mem_addr = 0;
               switch (type) {
               case MemoryViewType::RAM:
                  mem_addr = _hack.data_reg;
                  break;
               case MemoryViewType::ROM:
                  mem_addr = _hack.pc;
                  break;
               case MemoryViewType::Count:
                  break;
               }

               if (_hack_state == HackState::StepThrough || _hack_state == HackState::Running) {
                  set_memory_view_scroll(mem_addr);
               }

               if (_hack_state == HackState::Reset) {
                  set_memory_view_scroll(0);
               }

               if (ImGui::TableGetHoveredRow() == i) {
                  ImGui::TableSetBgColor(
                      ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_HeaderHovered));
               } else if (ImGui::IsItemActive()) {
                  ImGui::TableSetBgColor(
                      ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_HeaderActive));
               } else if (mem_addr == i && _hack_state != HackState::Off) {
                  ImGui::TableSetBgColor(
                      ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(Color::RED));
               }

               ImGui::PopID();
            }
         }
         ImGui::EndTable();
      }
   }
   ImGui::EndGroup();
}

void GuiContext::show_modal_message(const char *name, const char *msg) {
   ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(40, 0));
   ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_None, ImVec2(0.5, 0.5));
   if (ImGui::BeginPopupModal(name, nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove)) {
      ImGui::Dummy(ImVec2(0, 20));
      ImGui::PushTextWrapPos(450);
      ImGui::TextWrapped("%s", msg);
      ImGui::TextLinkOpenURL("Open Repository", "https://github.com/RaphGL/N2T_Suite");
      ImGui::Spacing();
      ImGui::PopTextWrapPos();

      ImGui::Spacing();
      if (ImGui::Button("OK", ImVec2(100, 0))) {
         ImGui::CloseCurrentPopup();
      }
      ImGui::EndPopup();
   }
   ImGui::PopStyleVar();
}

void GuiContext::show_menu_bar() {

   bool open_about_popup = false;

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
         if (ImGui::MenuItem("About")) {
            open_about_popup = true;
         }

         ImGui::EndMenu();
      }
      ImGui::EndMenuBar();
   }

   if (open_about_popup) {
      ImGui::OpenPopup("About");
   }

   show_modal_message("About",
       "N2T Suite is free and open source software licensed under EUPL 1.2 and "
       "maintained by RaphGL. If you find any bugs or performance issues, report "
       "them in the repository.");
}

void GuiContext::push_log(LogType type, const char *msg) {
   _logs_mutex.lock();
   _logs.push_back({
       .type = type,
       .msg = msg,
   });
   _logs_mutex.unlock();
}

void GuiContext::show_logs(int default_height) {
   ImGui::BeginChild("logs", ImVec2(0, default_height), ImGuiChildFlags_Borders);
   if (ImGui::Button("Clear")) {
      _logs.clear();
   }

   ImGui::BeginChild("##log-console");
   ImGui::PushFont(MONOFONT);
   ImGuiListClipper clipper;
   clipper.Begin(_logs.size());

   while (clipper.Step()) {
      for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
         auto log = _logs.at(i);
         switch (log.type) {
         case LogType::Error:
            ImGui::PushStyleColor(ImGuiCol_Text, Color::RED);
            ImGui::TextUnformatted("Error: ");
            break;
         case LogType::Success:
            ImGui::PushStyleColor(ImGuiCol_Text, Color::GREEN);
            ImGui::TextUnformatted("Success: ");
            break;
         }
         ImGui::SameLine();
         ImGui::TextUnformatted(log.msg.data());
         ImGui::Spacing();
         ImGui::PopStyleColor();
      }
   }
   ImGui::PopFont();
   ImGui::EndChild();
   ImGui::EndChild();
}

void GuiContext::show_hack_screen() {
   auto screen = _hack.get_screen_mmap();

   static std::array<GLuint, 512 * 256> pixels;
   std::fill(pixels.begin(), pixels.end(), 0xFF000000);

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

#include "cpu.hpp"
#include "../asm/asm.hpp"
#include "gui.hpp"
#include "imgui.h"
#include <atomic>
#include <charconv>
#include <chrono>
#include <cmath>
#include <iostream>
#include <thread>

namespace chrono = std::chrono;

namespace gui::cpu {

ViewCtx::ViewCtx(gui::Context *ctx)
    : _ctx { ctx }
    , _logs { ctx->monofont }
    , _rom_viewer(_hack.instruction_mem, _hack.pc)
    , _ram_viewer(_hack.data_mem, _hack.address_reg) {

   /* init hack screen texture */ {
      glGenTextures(1, &_hack_screen_tex);
      glBindTexture(GL_TEXTURE_2D, _hack_screen_tex);

      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glBindTexture(GL_TEXTURE_2D, 0);
   }

   _hack_worker = std::jthread([this](std::stop_token token) {
      constexpr int frames_per_second = 60;
      constexpr int ticks_per_frame = 1400000 / frames_per_second;
      constexpr auto time_per_frame = chrono::milliseconds(1000 / frames_per_second);

      while (!token.stop_requested()) {
         auto frame_start = chrono::high_resolution_clock::now();
         try {
            switch (_hack_state.load(std::memory_order_relaxed)) {
            case State::Off:
               [[fallthrough]];
            case State::Stopped:
               std::this_thread::sleep_for(chrono::milliseconds(30));
               break;

            case State::Running: {
               auto key = _ctx->key.load(std::memory_order_acquire);
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

            case State::StepThrough:
               _hack.tick();
               _hack_state.store(State::Stopped, std::memory_order_relaxed);
               std::this_thread::sleep_for(time_per_frame);
               break;

            case State::Reset:
               _hack.pc = 0;
               _hack_state.store(State::Stopped, std::memory_order_relaxed);
               break;
            }
         } catch (std::out_of_range &) {
            _hack_state.store(State::Stopped, std::memory_order_relaxed);
            _logs.push(LogType::Error, "Failed to run. Check if you have a valid program loaded.");
         }

         auto frame_end = chrono::milliseconds(0);
         while (frame_end < time_per_frame) {
            frame_end = chrono::duration_cast<chrono::milliseconds>(
                chrono::high_resolution_clock::now() - frame_start);
         }
      }
   });

   _dialog_worker = std::jthread([this](std::stop_token token) {
      while (!token.stop_requested()) {
         if (!_ctx->dialog_ready()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
         }

         auto file = _ctx->dialog_get_file();
         if (!file.has_value()) {
            continue;
         }

         fs::path filepath = file.value();
         auto file_ext = filepath.extension();
         if (file_ext == ".asm") {
            assembly::Lexer lexer { filepath };
            auto tokens = lexer.tokenize();
            if (tokens.empty()) {
               _logs.push(LogType::Error, "Failed to lex file.");
               continue;
            }
            assembly::Parser parser { tokens, filepath };
            auto ast = parser.parse();
            if (!ast.has_value()) {
               auto report = parser.get_error_report();
               _logs.push(LogType::Error, report.data());
               continue;
            }
            assembly::CodeGen codegen { ast.value(), filepath };
            auto machine_code = codegen.compile();
            if (!machine_code.has_value()) {
               auto report = codegen.get_error_report();
               _logs.push(LogType::Error, report.data());
               continue;
            }

            _hack.load_rom(machine_code.value());
            _logs.push(LogType::Success, "ROM Loaded.");
            _hack_state = State::Stopped;
         } else if (file_ext == ".hack") {
            std::ifstream filestream { filepath };
            std::stringstream contents;
            contents << filestream.rdbuf();
            if (!_hack.load_rom(std::move(contents.str()))) {
               _logs.push(LogType::Error,
                   "Failed to load Hack ROM, please check that the file contains valid hack "
                   "machine code.");
            }
            _hack_state = State::Stopped;
         } else {
            _logs.push(LogType::Error, "File contains an invalid extension.");
         }
      }
   });
}

ViewCtx::~ViewCtx() { glDeleteTextures(1, &_hack_screen_tex); }

std::string_view ViewCtx::view_name() const { return "CPU Simulator"; }

void ViewCtx::show() {
   show_top_bar();

   if (ImGui::BeginTable("memory-view", 2)) {
      ImGui::TableSetupColumn("Memory View", ImGuiTableColumnFlags_WidthStretch, 0.3f);
      ImGui::TableSetupColumn("Screen", ImGuiTableColumnFlags_WidthStretch, 0.7f);

      ImGui::TableNextColumn();
      ImGui::SeparatorText("Registers");
      show_hack_registers();
      ImGui::SeparatorText("ROM");
      auto region_avail = ImGui::GetContentRegionAvail();
      show_memory_view(MemoryViewType::ROM, region_avail.y / 2);
      ImGui::SeparatorText("RAM");
      region_avail = ImGui::GetContentRegionAvail();
      show_memory_view(MemoryViewType::RAM, region_avail.y);

      ImGui::TableNextColumn();
      ImGui::SeparatorText("Screen");
      show_hack_screen();
      region_avail = ImGui::GetContentRegionAvail();
      ImGui::SeparatorText("Logs");
      _logs.show(region_avail.y - ImGui::GetItemRectSize().y - ImGui::GetStyle().ItemSpacing.y);

      ImGui::EndTable();
   }
}

void ViewCtx::show_hack_screen() {
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
   screen_size.y = screen_size.x * 0.5f;

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

void ViewCtx::clear_hack_memory(MemoryViewType type) {
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

std::string_view view_option_to_string(MemoryViewOption view_option) {
   std::string_view view_option_str;
   switch (view_option) {
   case MemoryViewOption::Asm:
      view_option_str = "Assembly";
      break;
   case MemoryViewOption::Bin:
      view_option_str = "Binary";
      break;
   case MemoryViewOption::Dec:
      view_option_str = "Decimal";
      break;
   case MemoryViewOption::Hex:
      view_option_str = "Hexadecimal";
      break;
   }
   return view_option_str;
}

void ViewCtx::show_memory_view(MemoryViewType type, int default_height) {
   ImGui::BeginGroup();
   std::string_view label;
   std::span<std::uint16_t> hack_mem { };

   switch (type) {
   case MemoryViewType::RAM:
      label = "RAM";
      hack_mem = _hack.data_mem;
      break;
   case MemoryViewType::ROM:
      label = "ROM";
      hack_mem = _hack.instruction_mem;
      break;
   case MemoryViewType::Count:
      break;
   }

   static std::array<MemoryViewOption, static_cast<int>(MemoryViewType::Count)> curr_view_opt
       = { MemoryViewOption::Asm, MemoryViewOption::Dec };

   /* Memory viewer controls */ {
      ImGui::PushID(label.data());

      if (type != MemoryViewType::RAM) {
         if (ImGui::Button("Load")) {
            if (!_ctx->dialog_ongoing()) {
               _ctx->dialog_request_file();
            }
         }
         ImGui::SameLine();
      }
      if (ImGui::Button("Clear")) {
         ImGui::OpenPopup("clear-confirmation");
      }
      if (ImGui::BeginPopup("clear-confirmation")) {
         switch (type) {
         case MemoryViewType::RAM:
            ImGui::Text("Are you sure you want to clear RAM?");
            break;
         case MemoryViewType::ROM:
            ImGui::Text("Are you sure you want to clear ROM?");
            break;
         case MemoryViewType::Count:
            break;
         }
         if (ImGui::Button("Clear")) {
            clear_hack_memory(type);
            ImGui::CloseCurrentPopup();
         }
         ImGui::SameLine();
         if (ImGui::Button("Cancel")) {
            ImGui::CloseCurrentPopup();
         }
         ImGui::EndPopup();
      }

      ImGui::SameLine();
      if (ImGui::Button("Search")) {
         // TODO
      }

      std::array<MemoryViewOption, 4> view_options {
         MemoryViewOption::Asm,
         MemoryViewOption::Bin,
         MemoryViewOption::Dec,
         MemoryViewOption::Hex,
      };
      ImGui::SameLine();
      ImGui::SetNextItemWidth(-FLT_MIN);

      if (ImGui::BeginCombo("##view-options",
              view_option_to_string(curr_view_opt[static_cast<int>(type)]).data())) {
         std::size_t start_idx = type == MemoryViewType::RAM ? 1 : 0;
         for (std::size_t i = start_idx; i < view_options.size(); i++) {
            bool is_selected = curr_view_opt.at(static_cast<int>(type)) == view_options.at(i);

            auto curr_view_opt_str = view_option_to_string(view_options[i]);
            if (ImGui::Selectable(curr_view_opt_str.data(), is_selected)) {
               curr_view_opt[static_cast<int>(type)] = view_options[i];
            }

            if (is_selected) {
               ImGui::SetItemDefaultFocus();
            }
         }

         ImGui::EndCombo();
      }

      ImGui::PopID();
   }

   auto render_memory = [hack_mem, type](std::uint16_t idx) {
      char input_buf[32 * 1024] = { };

      switch (curr_view_opt[static_cast<int>(type)]) {
      case MemoryViewOption::Asm: {
         auto inst_opt = assembly::disassemble(hack_mem[idx]);
         auto inst_val = inst_opt.has_value() ? inst_opt.value() : "(invalid asm)";
         // TODO: make it editable once edited, we automatically assemble it and inject it
         // to memory
         ImGui::InputText("%s", inst_val.data(), inst_val.size() + 1);
      } break;

      case MemoryViewOption::Bin: {
         const auto bin_lit = std::format("0b{:b}", hack_mem[idx]);
         std::copy(bin_lit.begin(), bin_lit.end(), input_buf);

         if (ImGui::InputText("##mem_address", input_buf, sizeof(input_buf))) {
            std::uint16_t num;
            std::size_t num_start = 0, num_end = sizeof(input_buf);
            std::string_view input_str { input_buf };
            if (input_str.starts_with("0b")) {
               num_start = 2;
               num_end -= num_start;
            }
            auto chars_result
                = std::from_chars(input_buf + num_start, input_buf + num_end, num, 16);
            if (chars_result.ec == std::errc()) {
               hack_mem[idx] = num;
            }
         }
      } break;

      case MemoryViewOption::Dec:
         ImGui::InputScalarN("##mem_address", ImGuiDataType_U16, &hack_mem[idx], 1, nullptr,
             nullptr, nullptr, ImGuiInputTextFlags_CharsDecimal);
         break;

      case MemoryViewOption::Hex: {
         const auto hex_lit = std::format("0x{:x}", hack_mem[idx]);
         std::copy(hex_lit.begin(), hex_lit.end(), input_buf);

         if (ImGui::InputText("##mem_address", input_buf, sizeof(input_buf))) {
            std::uint16_t num;
            std::size_t num_start = 0, num_end = sizeof(input_buf);
            std::string_view input_str { input_buf };
            if (input_str.starts_with("0x")) {
               num_start = 2;
               num_end -= num_start;
            }
            auto chars_result
                = std::from_chars(input_buf + num_start, input_buf + num_end, num, 16);
            if (chars_result.ec == std::errc()) {
               hack_mem[idx] = num;
            }
         }
      } break;
      }
   };

   static auto prev_pc = -1;
   static auto prev_addr = -1;
   switch (type) {
   case MemoryViewType::ROM:
      if (_hack_state != State::Running && prev_pc != _hack.pc) {
         _rom_viewer.set_scroll(_hack.pc);
         prev_pc = _hack.pc;
      }
      _rom_viewer.show("##rom-viewer", default_height, render_memory);
      break;
   case MemoryViewType::RAM:
      if (_hack_state != State::Running && prev_addr != _hack.address_reg) {
         _ram_viewer.set_scroll(_hack.address_reg);
         prev_addr = _hack.address_reg;
      }
      _ram_viewer.show("##ram-viewer", default_height, render_memory);
      break;
   case MemoryViewType::Count:
      break;
   }

   if (_hack_state == State::Reset) {
      _rom_viewer.set_scroll(0);
      _ram_viewer.set_scroll(0);
      prev_pc = -1;
      prev_addr = -1;
   }

   if (_hack_state != State::Off && _hack_state != State::Running) {
      _rom_viewer.show_active_address = true;
      _ram_viewer.show_active_address = true;
   } else {
      _rom_viewer.show_active_address = false;
      _ram_viewer.show_active_address = false;
   }

   ImGui::EndGroup();
}

void ViewCtx::show_hack_registers() {
   ImGui::BeginChild("##hack-registers", ImVec2(0, ImGui::GetTextLineHeightWithSpacing() + 20),
       ImGuiChildFlags_Borders);

   if (ImGui::BeginTable("hack-registers", 3, 0, ImVec2(300, 0))) {
      ImGui::TableNextRow();
      ImGui::TableNextColumn();
      ImGui::TextUnformatted("A:");
      ImGui::SameLine();
      ImGui::Text("%d", _hack.address_reg);

      ImGui::TableNextColumn();
      ImGui::TextUnformatted("D:");
      ImGui::SameLine();
      ImGui::Text("%d", _hack.data_reg);

      ImGui::TableNextColumn();
      ImGui::TextUnformatted("PC:");
      ImGui::SameLine();
      ImGui::Text("%d", _hack.pc);

      ImGui::EndTable();
   }
   ImGui::EndChild();
}

void ViewCtx::show_top_bar() {
   ImGui::BeginGroup();
   if (ImGui::Button("Load Program")) {
      if (!_ctx->dialog_ongoing()) {
         _ctx->dialog_request_file();
      }
   }
   ImGui::SameLine();
   if (ImGui::Button("Single Step")) {
      _hack_state = State::StepThrough;
   }
   ImGui::SameLine();
   if (ImGui::Button(_hack_state != State::Running ? "Run" : "Stop")) {
      _hack_state = _hack_state == State::Running ? State::Stopped : State::Running;
   }
   ImGui::SameLine();
   if (ImGui::Button("Reset")) {
      _hack_state = State::Reset;
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
   ImGui::EndGroup();
}
};

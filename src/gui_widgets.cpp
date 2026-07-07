#include "gui_widgets.hpp"
#include "imgui.h"
#include <SDL3/SDL_dialog.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <string_view>

namespace fs = std::filesystem;

namespace gui {

void GuiContext::dialog_request_file() {
   SDL_DialogFileCallback callback = [](void *userdata, const char *const *filelist, int _) {
      auto filepath = static_cast<std::optional<fs::path> *>(userdata);

      if (*filelist) {
         *filepath = *filelist;
      }
   };

   _dialog_path = std::nullopt;
   _dialog_requested = true;
   SDL_ShowOpenFileDialog(callback, &_dialog_path, _window, NULL, 0, NULL, false);
}

bool GuiContext::dialog_ready() const { return _dialog_requested && _dialog_path.has_value(); }

fs::path GuiContext::dialog_get_file() {
   fs::path filepath = _dialog_path.value();
   _dialog_path = std::nullopt;
   _dialog_requested = false;
   return filepath;
}

void GuiContext::load_program() {
   if (!_dialog_requested) {
      dialog_request_file();
   }

   if (!dialog_ready()) {
      return;
   }

   auto file = dialog_get_file();
   std::ifstream filestream { file };
   std::stringstream contents;
   contents << filestream.rdbuf();
   std::cout << contents.str();
   _hack.load_rom(std::move(contents.str()));
}

void GuiContext::show_top_bar() {
   ImGui::BeginGroup();
   if (ImGui::BeginChild("top-bar", ImVec2(0, 40), ImGuiChildFlags_Borders)) {
      if (ImGui::Button("Load Program")) {
         load_program();
      }
      ImGui::SameLine();
      ImGui::Button("Single Step");
      ImGui::SameLine();
      ImGui::Button("Run");
      ImGui::SameLine();
      ImGui::Button("Stop");
      ImGui::SameLine();
      ImGui::Button("Reset");
      ImGui::SameLine();
      ImGui::Button("Load Script");

      ImGui::SameLine();
      ImGui::Text("CPU Speed:");
      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      static float cpu_speed = 1;
      if (ImGui::SliderFloat("##program-speed", &cpu_speed, 0.5f, 2.0f, "%.1f")) {
         constexpr float step = 0.1f;
         cpu_speed = std::round(cpu_speed / step) * step;
      }

      ImGui::EndChild();
   }
   ImGui::EndGroup();
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
         // TODO
      }
      ImGui::SameLine();
      if (ImGui::Button("Search")) {
         // TODO
      }

      static std::array<std::size_t, 2> curr_view_opt = { };
      curr_view_opt[static_cast<int>(MemoryViewType::ROM)] = 0;
      // ram does not display assembly so it should start from binary
      curr_view_opt[static_cast<int>(MemoryViewType::RAM)] = 1;
      std::array<std::string_view, 4> view_options {
         "assembly",
         "binary",
         "decimal",
         "hexademical",
      };
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
      ImGui::EndGroup();
      ImGui::PopID();

      if (ImGui::BeginTable(label.data(), 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter,
              ImVec2(0, 200))) {
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
               ImGui::InputScalarN("##mem_address", ImGuiDataType_U16, &hack_mem[i], 1);
               ImGui::PopID();
            }
         }
         ImGui::EndTable();
      }
   }
   ImGui::EndGroup();
}

};

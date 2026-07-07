#include "gui_widgets.hpp"
#include "imgui.h"
#include <array>
#include <cstdint>
#include <string_view>

namespace gui {

void show_memory_view(MemoryViewType type) {
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
      if (ImGui::Button("Load")) {
         // TODO
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

         for (int i = 1000; i < 1100; i++) {
            ImGui::TableNextRow();

            ImGui::PushID(i);
            ImGui::TableNextColumn();
            ImGui::Text("%d", i);

            ImGui::TableNextColumn();
            std::uint16_t tmp = 0;
            ImGui::InputScalarN("##mem_address", ImGuiDataType_U16, &tmp, 1);
            ImGui::PopID();
         }
         ImGui::EndTable();
      }
   }
   ImGui::EndGroup();
}

};

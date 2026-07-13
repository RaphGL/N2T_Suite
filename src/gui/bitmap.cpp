#include "bitmap.hpp"
#include "gui.hpp"
#include "imgui.h"
#include <algorithm>
#include <array>
#include <cstdint>

namespace gui::bitmap {

ViewCtx::ViewCtx(gui::Context *ctx)
    : _ctx { ctx } { }

void ViewCtx::show() {
   constexpr std::uint16_t default_height = 16, default_width = 16;
   constexpr std::uint16_t max_height = 256, max_width = 512;

   static std::uint16_t pixel_height = default_height, pixel_width = default_width;
   static std::array<bool, max_width * max_height> pixels { };

   /* Bitmap controls */ {
      ImGui::BeginGroup();

      ImGui::SetNextItemWidth(100);
      ImGui::InputScalarN("##pixel-height", ImGuiDataType_U16, &pixel_height, 1);
      ImGui::SameLine();
      ImGui::TextUnformatted("x");

      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      ImGui::InputScalarN("##pixel-width", ImGuiDataType_U16, &pixel_width, 1);

      ImGui::SameLine();
      if (ImGui::Button("Reset")) {
         pixel_width = default_width;
         pixel_height = default_height;
      }

      ImGui::EndGroup();
   }

   // clamp to hack screen size
   pixel_height = std::clamp<decltype(pixel_height)>(pixel_height, 1, max_height);
   pixel_width = std::clamp<decltype(pixel_width)>(pixel_width, 1, max_width);

   ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
   ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
   ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
   ImGui::BeginChild("Bitmap Grid");
   if (ImGui::BeginTable("Bitmap", pixel_width,
           ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_NoHostExtendX
               | ImGuiTableFlags_Borders)) {
      ImGuiListClipper clipper;
      clipper.Begin(pixel_height);
      while (clipper.Step()) {
         for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
            ImGui::TableNextRow();
            for (int col = 0; col < pixel_width; col++) {
               ImGui::TableNextColumn();
               ImGui::PushID(row * pixel_width + col);

               auto &curr_pixel = pixels.at(row * pixel_width + col);
               bool should_pop_color = false;
               if (curr_pixel) {
                  should_pop_color = true;
                  // TODO: change color to something more pleasant
                  ImGui::PushStyleColor(ImGuiCol_Button, Color::RED);
               }

               ImGui::Button("##bitmap-pixel", ImVec2(15, 15));

               static bool first_click_state = false;
               if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem)) {
                  if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                     first_click_state = !curr_pixel;
                  }
                  if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                     curr_pixel = first_click_state;
                  }
               }

               if (should_pop_color) {
                  ImGui::PopStyleColor();
               }

               ImGui::PopID();
            }
         }
      }
      ImGui::EndTable();
   }
   ImGui::EndChild();
   ImGui::PopStyleVar(3);
}

std::string_view ViewCtx::view_name() const { return "Bitmap Editor"; }

}

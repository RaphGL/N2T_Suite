#include "bitmap.hpp"
#include "imgui.h"
#include <cstdint>

namespace gui::bitmap {

ViewCtx::ViewCtx(gui::Context *ctx)
    : _ctx { ctx } { }

void ViewCtx::show() {
   constexpr std::uint16_t default_height = 16, default_width = 16;
   static std::uint16_t pixel_height = default_height, pixel_width = default_width;

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

   if (pixel_height > 0 && pixel_width > 0) {
      ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
      ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
      if (ImGui::BeginTable("Bitmap", pixel_width,
              ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_NoHostExtendX
                  | ImGuiTableFlags_Borders)) {
         size_t item_id = 0;
         for (int row = 0; row < pixel_height; row++) {
            ImGui::TableNextRow();
            for (int col = 0; col < pixel_width; col++) {
               ImGui::TableNextColumn();
               ImGui::PushID(item_id++);
               if (ImGui::Button("##bitmap-pixel", ImVec2(15, 15))) {
                  // TODO: draw pixel
               }
               ImGui::PopID();
            }
         }
         ImGui::EndTable();
      }
      ImGui::PopStyleVar(3);
   }
}

std::string_view ViewCtx::view_name() const { return "Bitmap Editor"; }

}

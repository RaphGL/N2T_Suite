#include "bitmap.hpp"
#include "gui.hpp"
#include "imgui.h"
#include <algorithm>
#include <array>
#include <cstdint>

namespace gui::bitmap {

ViewCtx::ViewCtx(gui::Context *ctx)
    : _ctx { ctx } { }

std::string_view ViewCtx::view_name() const { return "Bitmap Editor"; }

bool &ViewCtx::bitmap_at(std::uint16_t x, std::uint16_t y) {
   if (x > _pixels_width) {
      throw "X index is bigger than the current bitmap width";
   }
   if (y > _pixels_height) {
      throw "Y index is bigger than the current bitmap height";
   }
   return _pixels.at(y * _pixels_width + x);
}

void ViewCtx::bitmap_invert() {
   for (std::size_t i = 0; i < _pixels_width * _pixels_height; i++) {
      _pixels.at(i) = !_pixels.at(i);
   }
}

void ViewCtx::bitmap_shift(ShiftDirection dir) {
   switch (dir) {
   case ShiftDirection::Left:
      // TODO: not properly handling shifts when left edge is reached
      for (std::uint16_t y = 0; y < _pixels_height; y++) {
         for (std::uint16_t x = 0; x < _pixels_width; x++) {
            auto &curr = bitmap_at(x, y);
            bool next = false;
            if (x != _pixels_width - 1) {
               next = bitmap_at(x + 1, y);
            }
            curr = next;
         }
      }
      break;

   case ShiftDirection::Right:
      for (std::uint16_t y = 0; y < _pixels_height; y++) {
         for (std::uint16_t x = _pixels_width - 1; x > 0; x--) {
            auto &curr = bitmap_at(x, y);
            bool prev = false;
            if (x != 0) {
               prev = bitmap_at(x - 1, y);
            }
            curr = prev;
         }
      }
      break;

   case ShiftDirection::Up:
      // TODO: not properly handling shifts when upper edge is reached
      for (std::uint16_t y = 0; y < _pixels_height; y++) {
         for (std::uint16_t x = 0; x < _pixels_width; x++) {
            auto &curr = bitmap_at(x, y);
            bool next = false;
            if (y != _pixels_height - 1) {
               next = bitmap_at(x, y + 1);
            }
            curr = next;
         }
      }
      break;

   case ShiftDirection::Down:
      for (std::uint16_t y = _pixels_height - 1; y > 0; y--) {
         for (std::uint16_t x = 0; x < _pixels_width; x++) {
            auto &curr = bitmap_at(x, y);
            bool next = false;
            if (y != 0) {
               next = bitmap_at(x, y - 1);
            }
            curr = next;
         }
      }
      break;
   }
}

void ViewCtx::show() {
   constexpr std::size_t button_size = 15;

   /* Bitmap controls */ {
      ImGui::BeginGroup();

      ImGui::SetNextItemWidth(100);
      bool height_changed = ImGui::InputScalarN("##pixel-height", ImGuiDataType_U16, &_pixels_height, 1);
      ImGui::SameLine();
      ImGui::TextUnformatted("x");

      ImGui::SameLine();
      ImGui::SetNextItemWidth(100);
      bool width_changed = ImGui::InputScalarN("##pixel-width", ImGuiDataType_U16, &_pixels_width, 1);

      if (height_changed || width_changed) {
         // TODO: shift pixels to match new dimensions
      }

      ImGui::SameLine();
      if (ImGui::Button("Reset")) {
         _pixels_width = default_width;
         _pixels_height = default_height;
      }

      ImGui::SameLine();
      if (ImGui::Button("Clear")) {
         std::fill(_pixels.begin(), _pixels.end(), false);
      }

      ImGui::EndGroup();

      if (ImGui::Button("Shift Left")) {
         bitmap_shift(ShiftDirection::Left);
      }
      ImGui::SameLine();
      if (ImGui::Button("Shift Right")) {
         bitmap_shift(ShiftDirection::Right);
      }
      ImGui::SameLine();
      if (ImGui::Button("Shift Up")) {
         bitmap_shift(ShiftDirection::Up);
      }
      ImGui::SameLine();
      if (ImGui::Button("Shift Down")) {
         bitmap_shift(ShiftDirection::Down);
      }
      ImGui::SameLine();
      if (ImGui::Button("Invert")) {
         bitmap_invert();
      }
   }

   // clamp to hack screen size
   _pixels_height = std::clamp<decltype(_pixels_height)>(_pixels_height, 1, max_height);
   _pixels_width = std::clamp<decltype(_pixels_width)>(_pixels_width, 1, max_width);

   ImGui::BeginChild("Bitmap Grid");
   ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0, 0));
   ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
   ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));

   // center drawing grid
   const auto table_width = _pixels_width * button_size;
   auto window_width = ImGui::GetWindowSize().x;
   ImGui::SetCursorPosX((window_width - table_width) / 2);

   if (ImGui::BeginTable("Bitmap", _pixels_width,
           ImGuiTableFlags_SizingFixedSame | ImGuiTableFlags_NoHostExtendX
               | ImGuiTableFlags_Borders)) {
      ImGuiListClipper clipper;
      clipper.Begin(_pixels_height);
      while (clipper.Step()) {
         for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
            ImGui::TableNextRow();
            for (int col = 0; col < _pixels_width; col++) {
               ImGui::TableNextColumn();
               ImGui::PushID(row * _pixels_width + col);

               auto &curr_pixel = bitmap_at(col, row);
               bool should_pop_color = false;
               if (curr_pixel) {
                  should_pop_color = true;
                  // TODO: change color to something more pleasant
                  ImGui::PushStyleColor(ImGuiCol_Button, Color::RED);
               }

               ImGui::Button("##bitmap-pixel", ImVec2(button_size, button_size));

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
   ImGui::PopStyleVar(3);
   ImGui::EndChild();
}

}

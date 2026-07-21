#include "memory_viewer.hpp"
#include "../gui.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdint>
#include <functional>
#include <span>

namespace gui::widget {

constexpr double MIN_ROW_HEIGHT = 28.0;

MemoryViewer::MemoryViewer(std::span<std::uint16_t> memory, std::uint16_t &curr_mem_addr)
    : _memory { memory }
    , _curr_addr { curr_mem_addr } { }

void MemoryViewer::set_scroll(int row) { _next_scroll = row; }

void MemoryViewer::show(std::string_view label, int default_height,
    std::function<void(std::uint16_t)> render_memory_address) {
   const auto buttons_height = ImGui::GetItemRectSize().y + ImGui::GetStyle().ItemSpacing.y;

   if (ImGui::BeginTable(label.data(), 2, ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter,
           ImVec2(0, default_height - buttons_height))) {
      ImGui::TableSetupColumn("##address", ImGuiTableColumnFlags_WidthFixed, 50);
      ImGui::TableSetupColumn("##memory-contents", ImGuiTableColumnFlags_WidthStretch);

      // necessary otherwise the performance would crawl with 32K items to render
      ImGuiListClipper clipper;
      clipper.Begin(_memory.size());
      clipper.IncludeItemByIndex(_curr_addr);
      while (clipper.Step()) {
         for (auto i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
            ImGui::PushID(i);
            ImGui::TableNextRow(ImGuiTableRowFlags_None, MIN_ROW_HEIGHT);
            ImGui::TableNextColumn();
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%d", static_cast<int>(i));

            ImGui::TableNextColumn();
            ImGui::SetNextItemWidth(-FLT_MIN);

            render_memory_address(i);

            if (_next_scroll.has_value() && _next_scroll.value() == i) {
               ImGui::SetScrollHereY(0.4f);
               _next_scroll = std::nullopt;
            }

            if (ImGui::IsItemHovered()) {
               ImGui::TableSetBgColor(
                   ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_HeaderHovered));
            } else if (ImGui::IsItemActive()) {
               ImGui::TableSetBgColor(
                   ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(ImGuiCol_HeaderActive));
            } else if (show_active_address && _curr_addr == i) {
               ImGui::TableSetBgColor(
                   ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(gui::Color::RED));
            }
            ImGui::PopID();
         }
      }

      ImGui::EndTable();
   }
}

}

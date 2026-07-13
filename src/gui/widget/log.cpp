#include "log.hpp"
#include "imgui.h"
#include "../gui.hpp"

namespace gui::widget {

Log::Log(ImFont *monofont)
    : _monofont { monofont } { }

void Log::push(LogType type, const char *msg) {
   _logs_mutex.lock();
   _logs.push_back({
       .type = type,
       .msg = msg,
   });
   _logs_mutex.unlock();
}

void Log::show(int default_height) {
   ImGui::BeginChild("logs", ImVec2(0, default_height), ImGuiChildFlags_Borders);
   if (ImGui::Button("Clear")) {
      _logs.clear();
   }

   ImGui::BeginChild("##log-console");
   ImGui::PushFont(_monofont);
   ImGuiListClipper clipper;
   clipper.Begin(_logs.size());

   while (clipper.Step()) {
      for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
         auto log = _logs.at(i);
         switch (log.type) {
         case LogType::Error:
            ImGui::PushStyleColor(ImGuiCol_Text, gui::Color::RED);
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

}

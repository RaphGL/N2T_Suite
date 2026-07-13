#include "gui.hpp"
#include "../asm/asm.hpp"
#include "bitmap.hpp"
#include "cpu.hpp"
#include "imgui.h"
#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

namespace gui {

Context::Context(SDL_Window *window)
    : _window { window } {
   // NOTE: the views are shown in the gui based on their insertion order here
   _views.push_back(std::make_unique<cpu::ViewCtx>(this));
   _views.push_back(std::make_unique<bitmap::ViewCtx>(this));
}

// TODO: embed default fonts in application
void Context::set_styling() {
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
   this->monofont = io.Fonts->AddFontFromFileTTF(mono_font, 16.0f);
}

void Context::dialog_request_file() {
   SDL_DialogFileCallback callback = [](void *userdata, const char *const *filelist, int _) {
      auto gc = static_cast<Context *>(userdata);

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

bool Context::dialog_ongoing() const { return _dialog_requested && !_dialog_path.has_value(); }

bool Context::dialog_ready() const { return !_dialog_requested && _dialog_path.has_value(); }

std::optional<fs::path> Context::dialog_get_file() {
   _dialog_mutex.lock();
   auto filepath = _dialog_path;
   _dialog_path = std::nullopt;
   _dialog_mutex.unlock();
   return filepath;
}

void Context::show_modal_message(const char *name, const char *msg) {
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

void Context::show_menu_bar() {

   bool open_about_popup = false;

   if (ImGui::BeginMenuBar()) {

      if (ImGui::BeginMenu("File")) {
         if (ImGui::MenuItem("Load Program")) {
            dialog_request_file();
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

void Context::show() {
   show_menu_bar();

   if (ImGui::BeginTabBar("N2T Tools", ImGuiTabBarFlags_DrawSelectedOverline)) {
      for (auto &view : _views) {
         if (ImGui::BeginTabItem(view->view_name().data())) {
            view->show();
            ImGui::EndTabItem();
         }
      }

      if (ImGui::BeginTabItem("Hardware Simulator")) {
         ImGui::TextUnformatted("TODO");
         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("VM Simulator")) {
         ImGui::TextUnformatted("TODO");
         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Compilers")) {
         ImGui::TextUnformatted("TODO");
         ImGui::EndTabItem();
      }

      if (ImGui::BeginTabItem("Converter")) {
         ImGui::TextUnformatted("TODO");
         ImGui::EndTabItem();
      }

      ImGui::EndTabBar();
   }
}

}

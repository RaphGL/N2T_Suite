#include "asm/asm.hpp"
#include "gui_widgets.hpp"
#include "hack/hack.hpp"
// #include "hdl/lexer.hpp"
// #include "hdl/parser.hpp"
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl3.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3/SDL_video.h>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <optional>
#include <span>
#include <sstream>

namespace chrono = std::chrono;
namespace fs = std::filesystem;

int hdl_cmd(std::span<char *> args) {
   std::cerr << "TODO: hdl command hasn't been implemented yet.\n";
   // hdl::Lexer tk{"test.hdl"};
   // auto tokens = tk.tokenize();

   // hdl::Parser parser{tokens, "test.hdl"};
   // auto ast = parser.parse();
   // if (!ast.has_value()) {
   //   std::cerr << parser.get_error_report();
   //   return 1;
   // }

   // auto chips = ast.value();
   // std::cout << chips[0].parts[2].name;
   return 0;
}

int asm_cmd(std::span<char *> args) {
   if (args.empty()) {
      std::cerr << "missing file argument.\n";
      return 1;
   }

   // === parse args ===
   const fs::path file = args[0];
   if (!fs::exists(file)) {
      std::cerr << "File does not exist.\n";
      return 1;
   }
   std::optional<const char *> output_flag { };

   if (args.size() >= 2) {
      if (args.size() == 3 && std::string_view(args[1]) == "-o") {
         output_flag = args[2];
      } else {
         std::cerr << "invalid flag. Expected `-o <output_file>`";
         return 1;
      }
   }

   // === assemble file ===
   assembly::Lexer lex { file };
   auto tokens = lex.tokenize();
   if (tokens.empty()) {
      std::cerr << "Failed to tokenize file. File is possibly not a valid "
                   "assembly file.\n";
      return 1;
   }
   assembly::Parser parser { tokens, file };
   auto insts_opt = parser.parse();

   if (!insts_opt.has_value()) {
      std::cerr << parser.get_error_report();
      return 1;
   }

   auto instructions = insts_opt.value();
   assembly::CodeGen codegen { instructions, file };
   auto asm_output = codegen.compile();
   if (!asm_output.has_value()) {
      std::cerr << codegen.get_error_report();
      return 1;
   }

   auto output = assembly::to_string(asm_output.value());

   // === write compiled artifact to file ===
   fs::path output_file { file };
   if (output_flag.has_value()) {
      output_file = output_flag.value();
   } else {
      output_file.replace_extension("hack");
   }

   std::ofstream asm_file { output_file.filename() };
   asm_file.write(output.c_str(), output.size());
   return 0;
}

int disasm_cmd(std::span<char *> args) {
   if (args.size() == 0) {
      std::cout << "Missing file argument.\n";
      return 1;
   }

   fs::path file { args[0] };
   std::ifstream input { file };
   std::stringstream ss;
   ss << input.rdbuf();

   auto disasm = assembly::disassemble(ss.str());
   if (!disasm.has_value()) {
      std::cerr << "Failed to disassemble file. It is possibly not a valid Hack ROM.\n";
      return 1;
   }

   file.replace_extension(".disasm");
   std::ofstream disasm_file { file };
   disasm_file << disasm.value();
   return 0;
}

int run_cmd(std::span<char *> args) {
   if (args.empty()) {
      std::cerr << "missing file argument.\n";
      return 1;
   }

   const fs::path file = args[0];
   if (!fs::exists(file)) {
      std::cerr << "File does not exist.\n";
      return 1;
   }

   // === load and validate ROM ===
   std::ifstream input_stream { file };
   std::string tmp_str { };
   std::string input { };
   while (std::getline(input_stream, tmp_str)) {
      input += tmp_str + '\n';
      for (const auto ch : tmp_str) {
         if (std::isdigit(ch) || std::isspace(ch)) {
            continue;
         }

         if (asm_cmd(args) != 0) {
            return 1;
         }

         auto compiled_filepath = fs::path(file).replace_extension("hack");

         std::ifstream compiled_file { compiled_filepath };
         if (!compiled_file.is_open()) {
            std::cerr << "Failed to open file.\n";
         }

         std::stringstream strbuf;
         strbuf << compiled_file.rdbuf();
         input = strbuf.str();
         goto load_rom;
      }
   }

load_rom:
   if (input.empty()) {
      std::cout << "The hack ROM is empty.\n";
      return 1;
   }

   Hack hack { };
   hack.load_rom(input);

   // === Run/Emulate ===
   if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
      std::cerr << SDL_GetError() << '\n';
      return 1;
   }

   SDL_Window *window;
   SDL_Renderer *renderer;
   if (!SDL_CreateWindowAndRenderer(
           "N2T Hack Emulator", 800, 400, SDL_WINDOW_RESIZABLE, &window, &renderer)) {
      std::cerr << SDL_GetError() << '\n';
      return 1;
   }
   constexpr float aspect_ratio = 512.0f / 256;
   SDL_SetWindowAspectRatio(window, aspect_ratio, aspect_ratio);

   auto &keyboard_input = hack.get_keyboard_mmap();
   auto texture = SDL_CreateTexture(
       renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 512, 256);
   SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

   constexpr int ticks_per_frame = 1000000 / 16.6;

   for (;;) {
      auto frame_start = chrono::high_resolution_clock::now();

      SDL_Event e { };
      while (SDL_PollEvent(&e)) {
         switch (e.type) {
         case SDL_EVENT_KEY_DOWN:
            keyboard_input = convert_input_to_hack(e.key.key);
            break;
         case SDL_EVENT_KEY_UP:
            keyboard_input = 0;
            break;
         case SDL_EVENT_QUIT:
            return 0;
         }
      }

      for (std::size_t i = 0; i < ticks_per_frame; i++) {
         try {
            hack.tick();
         } catch (std::string err) {
            std::cerr << err << '\n';
            return 1;
         }
      }

      float frame_end = 0.0f;
      while (frame_end < 16.0f) {
         frame_end = chrono::duration_cast<chrono::milliseconds>(
             chrono::high_resolution_clock::now() - frame_start)
                         .count();
      }
      hack.draw_screen(renderer, texture);
   }

   SDL_DestroyTexture(texture);
   SDL_DestroyWindow(window);
   SDL_DestroyRenderer(renderer);
   return 0;
}

int gui_cmd() {
   if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
      std::cerr << SDL_GetError() << '\n';
      return 1;
   }

   SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);

   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
   SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

   float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());

   SDL_Window *window
       = SDL_CreateWindow("N2T Suite", 1200, 800, SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
   if (!window) {
      std::cerr << SDL_GetError() << '\n';
      return 1;
   }

   SDL_GLContext gl_ctx = SDL_GL_CreateContext(window);
   if (!gl_ctx) {
      std::cerr << SDL_GetError() << '\n';
      return 1;
   }

   SDL_GL_MakeCurrent(window, gl_ctx);
   SDL_GL_SetSwapInterval(1);
   SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
   SDL_ShowWindow(window);

   IMGUI_CHECKVERSION();
   ImGui::CreateContext();
   auto &io = ImGui::GetIO();
   io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

   auto &style = ImGui::GetStyle();
   style.ScaleAllSizes(main_scale);
   style.FontScaleDpi = main_scale;

   ImGui_ImplSDL3_InitForOpenGL(window, gl_ctx);
   ImGui_ImplOpenGL3_Init();

   ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

   gui::GuiContext w { window };

   auto update_theme = [&w] {
      auto system_theme = SDL_GetSystemTheme();
      if (system_theme == SDL_SYSTEM_THEME_DARK || system_theme == SDL_SYSTEM_THEME_UNKNOWN) {
         ImGui::StyleColorsDark();
      } else {
         ImGui::StyleColorsLight();
      }
      // TODO: add light and dark theme variants
      w.set_styling();
   };

   update_theme();

   for (;;) {
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
         ImGui_ImplSDL3_ProcessEvent(&e);
         switch (e.type) {
         case SDL_EVENT_QUIT:
            goto cleanup;

         case SDL_EVENT_KEY_DOWN:
            w.set_keyboard_input(e.key.key);
            break;

         case SDL_EVENT_KEY_UP:
            w.set_keyboard_input(std::nullopt);
            break;

         case SDL_EVENT_SYSTEM_THEME_CHANGED:
            update_theme();
            break;
         }
      }

      if (SDL_GetWindowFlags(window) & SDL_WINDOW_MINIMIZED) {
         SDL_Delay(16);
      }

      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL3_NewFrame();
      ImGui::NewFrame();

      const ImGuiViewport *viewport = ImGui::GetMainViewport();
      ImGui::SetNextWindowPos(viewport->WorkPos);
      ImGui::SetNextWindowSize(viewport->WorkSize);
      ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);

      ImGuiWindowFlags winflags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove
          | ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoSavedSettings
          | ImGuiWindowFlags_NoScrollWithMouse;
      if (ImGui::Begin("N2T Suite", nullptr, winflags)) {

         w.show_menu_bar();

         if (ImGui::BeginTabBar("N2T Tools", ImGuiTabBarFlags_DrawSelectedOverline)) {
            if (ImGui::BeginTabItem("Hardware Simulator")) {
               ImGui::TextUnformatted("TODO");
               ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("CPU Simulator")) {
               // TODO: do refactor so that the main gui loop doesn't have to care about
               // layouting besides just defining the tabs
               w.show_top_bar();

               if (ImGui::BeginTable("memory-view", 2)) {
                  ImGui::TableSetupColumn("Memory View", ImGuiTableColumnFlags_WidthStretch, 0.3f);
                  ImGui::TableSetupColumn("Screen", ImGuiTableColumnFlags_WidthStretch, 0.7f);

                  ImGui::TableNextColumn();
                  ImGui::SeparatorText("Registers");
                  w.show_hack_registers();
                  ImGui::SeparatorText("ROM");
                  auto region_avail = ImGui::GetContentRegionAvail();
                  w.show_memory_view(gui::MemoryViewType::ROM, region_avail.y / 2);
                  ImGui::SeparatorText("RAM");
                  region_avail = ImGui::GetContentRegionAvail();
                  w.show_memory_view(gui::MemoryViewType::RAM, region_avail.y);

                  ImGui::TableNextColumn();
                  ImGui::SeparatorText("Screen");
                  w.show_hack_screen();
                  region_avail = ImGui::GetContentRegionAvail();
                  ImGui::SeparatorText("Logs");
                  w.show_logs(region_avail.y - ImGui::GetItemRectSize().y
                      - ImGui::GetStyle().ItemSpacing.y);

                  ImGui::EndTable();
               }

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

            if (ImGui::BeginTabItem("Bitmap Editor")) {
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
               ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Converter")) {
               ImGui::TextUnformatted("TODO");
               ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
         }
         ImGui::End();
      }

      ImGui::PopStyleVar();
      ImGui::Render();
      glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
      glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w,
          clear_color.z * clear_color.w, clear_color.w);
      glClear(GL_COLOR_BUFFER_BIT);
      ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
      SDL_GL_SwapWindow(window);
   }

cleanup:
   ImGui_ImplOpenGL3_Shutdown();
   ImGui_ImplSDL3_Shutdown();
   ImGui::DestroyContext();
   SDL_GL_DestroyContext(gl_ctx);
   SDL_DestroyWindow(window);
   SDL_Quit();
   return 0;
}

void print_help(const char *program) {
   std::cout << std::format("{} - nand2tetris development suite\n"
                            "\n"
                            "Usage: {} <COMMAND> [FILE]\n"
                            "\n"
                            "Commands:\n"
                            "\trun\tRun the hack emulator\n"
                            "\tasm\tCompile assembly into hack instructions\n"
                            "\tdisasm\tDisassemble hack instructions\n"
                            "\thdl\tResolve hdl circuit\n"
                            "\tgui\tRun N2T GUI suite\n"
                            "\thelp\tPrint this message\n",
       program, program);
}

int main(int argc, char *argv[]) {
   if (argc == 1) {
      return gui_cmd();
   }

   const std::string_view cmd { argv[1] };
   std::span<char *> cmd_args { &argv[2], static_cast<size_t>(argc - 2) };

   if (cmd == "help") {
      print_help(argv[0]);
      return 0;
   }

   if (cmd == "asm") {
      return asm_cmd(cmd_args);
   }

   if (cmd == "disasm") {
      return disasm_cmd(cmd_args);
   }

   if (cmd == "hdl") {
      return hdl_cmd(cmd_args);
   }

   if (cmd == "run") {
      return run_cmd(cmd_args);
   }

   if (cmd == "gui") {
      return gui_cmd();
   }

   std::cerr << std::format(
       "wrong argument. check `{} help` to see what commands are available.\n", argv[0]);
   return 1;
}

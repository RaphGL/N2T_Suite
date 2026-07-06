#include "asm/codegen.hpp"
#include "asm/lexer.hpp"
#include "asm/parser.hpp"
#include "hack/hack.hpp"
// #include "hdl/lexer.hpp"
// #include "hdl/parser.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_pixels.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3/SDL_video.h>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <span>

namespace chrono = std::chrono;
namespace fs = std::filesystem;

void draw_screen(SDL_Renderer *renderer, SDL_Texture *texture,
                 ScreenSpan screen) {
  std::array<Uint32, 512 * 256> pixels;
  std::fill(pixels.begin(), pixels.end(), 0x000000FF);

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

  SDL_UpdateTexture(texture, nullptr, pixels.data(), 512 * sizeof(Uint32));
  SDL_RenderClear(renderer);
  SDL_RenderTexture(renderer, texture, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}

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
  }
  std::optional<const char *> output_flag{};

  if (args.size() >= 2) {
    if (args.size() == 3 && std::string_view(args[1]) == "-o") {
      output_flag = args[2];
    } else {
      std::cerr << "invalid flag. Expected `-o <output_file>`";
      return 1;
    }
  }

  // === assemble file ===
  assembly::Lexer lex{file};
  auto tokens = lex.tokenize();
  if (tokens.size() == 0) {
    std::cerr << "Failed to tokenize file. File is possibly not a valid "
                 "assembly file.\n";
    return 1;
  }
  assembly::Parser parser{tokens, file};
  auto insts_opt = parser.parse();

  if (!insts_opt.has_value()) {
    std::cerr << parser.get_error_report();
    return 1;
  }

  auto instructions = insts_opt.value();
  assembly::CodeGen codegen{instructions, file};
  auto asm_output = codegen.compile();
  if (!asm_output.has_value()) {
    std::cerr << codegen.get_error_report();
    return 1;
  }

  auto output = assembly::to_string(asm_output.value());

  // === write compiled artifact to file ===
  fs::path output_file{file};
  if (output_flag.has_value()) {
    output_file = output_flag.value();
  } else {
    output_file.replace_extension("hack");
  }

  std::ofstream asm_file{output_file.filename()};
  asm_file.write(output.c_str(), output.size());
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
  std::ifstream input_stream{file};
  std::string tmp_str{};
  std::string input{};
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

      std::ifstream compiled_file{compiled_filepath};
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
  Hack hack{};
  hack.load_rom(input);

  // === Run/Emulate ===
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    std::cerr << SDL_GetError() << '\n';
    return 1;
  }

  SDL_Window *window;
  SDL_Renderer *renderer;
  if (!SDL_CreateWindowAndRenderer("N2T Hack Emulator", 512, 256,
                                   SDL_WINDOW_RESIZABLE, &window, &renderer)) {
    std::cerr << SDL_GetError() << '\n';
    return 1;
  }
  constexpr float aspect_ratio = 512.0f / 256;
  SDL_SetWindowAspectRatio(window, aspect_ratio, aspect_ratio);

  auto &keyboard_input = hack.get_keyboard_mmap();
  auto texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                   SDL_TEXTUREACCESS_STREAMING, 512, 256);
  SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

  constexpr int ticks_per_frame = 1000000 / 16.6;

  for (;;) {
    auto frame_start = chrono::high_resolution_clock::now();

    SDL_Event e{};
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
    draw_screen(renderer, texture, hack.get_screen_mmap());
  }

  SDL_DestroyTexture(texture);
  SDL_DestroyWindow(window);
  SDL_DestroyRenderer(renderer);
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
                           "\thdl\tResolve hdl circuit\n"
                           "\thelp\tPrint this message\n",
                           program, program);
}

int main(int argc, char *argv[]) {
  // NOTE: in the future we likely will want a ide command or just launching the
  // GUI when no arguments are passed
  if (argc == 1) {
    print_help(argv[0]);
    return 0;
  }

  const std::string_view cmd{argv[1]};

  if (cmd == "help") {
    print_help(argv[0]);
    return 0;
  }

  std::span<char *> cmd_args{&argv[2], static_cast<size_t>(argc - 2)};

  if (cmd == "asm") {
    return asm_cmd(cmd_args);
  }

  if (cmd == "hdl") {
    return hdl_cmd(cmd_args);
  }

  if (cmd == "run") {
    return run_cmd(cmd_args);
  }

  std::cerr << std::format(
      "wrong argument. check `{} help` to see what commands are available.\n",
      argv[0]);
  return 1;
}

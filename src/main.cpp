#include "asm/codegen.hpp"
#include "asm/lexer.hpp"
#include "asm/parser.hpp"
#include "hack/hack.hpp"
// #include "hdl/lexer.hpp"
// #include "hdl/parser.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <span>

// TODO: fix bugged drawing logic
void draw_screen(SDL_Renderer *renderer, ScreenSpan screen) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xFF);
  SDL_RenderClear(renderer);
  SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);

  std::size_t x{0}, y{0};
  for (std::size_t i = 0; i < screen.size(); i++) {
    auto chunk = screen[i];
    std::uint16_t mask{0b1000000000000000};
    y = i / 256;

    // if (chunk == UINT16_MAX) {
    //   x = i % 512;
    //   SDL_RenderLine(renderer, x, y, x + 16, y);
    //   continue;
    // }

    for (std::size_t j = 0; j < 16; j++) {
      auto pixel = chunk & mask;
      mask >>= 1;

      if (pixel) {
        x = (i * 16 + j) % 512;
        SDL_RenderPoint(renderer, x, y);
      }
    }
  }

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
  const auto file = args[0];
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
  assembly::Parser parser{tokens, file};
  auto insts_opt = parser.parse();

  if (!insts_opt.has_value()) {
    std::cout << parser.get_error_report();
    return 1;
  }

  auto instructions = insts_opt.value();
  assembly::CodeGen codegen{instructions, file};
  auto asm_output = codegen.compile();
  if (!asm_output.has_value()) {
    std::cout << codegen.get_error_report();
    return 1;
  }

  auto output = assembly::to_string(asm_output.value());

  // === write compiled artifact to file ===
  std::filesystem::path output_file{file};
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
  const auto file = args[0];

  // === load and validate ROM ===
  std::ifstream input_stream{file};
  std::string tmp_str{};
  std::string input{};
  while (std::getline(input_stream, tmp_str)) {
    input += tmp_str + '\n';
    for (const auto ch : tmp_str) {
      if (!std::isdigit(ch) && !std::isspace(ch)) {
        std::cerr << "Invalid Hack ROM. A Hack ROM should only contain 1s, 0s and whitespace.\n";
        return 1;
      }
    }
  }

  Hack hack{};
  hack.load_rom(input);

  // === Run/Emulate ===
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
    std::cerr << SDL_GetError() << '\n';
    return 1;
  }

  SDL_Window *window;
  SDL_Renderer *renderer;
  if (!SDL_CreateWindowAndRenderer("N2T Hack Emulator", 512, 256, 0, &window,
                                   &renderer)) {
    std::cerr << SDL_GetError() << '\n';
    return 1;
  }

  for (;;) {
    SDL_Event e{};
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_EVENT_QUIT) {
        return 0;
      }
    }

    try {
    hack.tick();

    } catch (std::string err) {
      std::cerr << err << '\n';
      return 0;
    }
    draw_screen(renderer, hack.get_screen_mmap());
  }

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

#include "asm/codegen.hpp"
#include "asm/lexer.hpp"
#include "asm/parser.hpp"
#include "hack/hack.hpp"
#include "hdl/lexer.hpp"
#include "hdl/parser.hpp"
#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_oldnames.h>
#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>
#include <cstdint>
#include <iostream>
#include <variant>

#include <SDL3/SDL.h>

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

int main() {

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

  assembly::Lexer lex{"test.asm"};
  auto tokens = lex.tokenize();
  assembly::Parser parser{tokens, "test.asm"};
  auto insts_opt = parser.parse();

  if (!insts_opt.has_value()) {
    std::cout << parser.get_error_report();
    return 1;
  }

  auto instructions = insts_opt.value();
  assembly::CodeGen codegen{instructions, "test.asm"};
  auto asm_output = codegen.compile();
  if (!asm_output.has_value()) {
    std::cout << codegen.get_error_report();
    return 1;
  }

  auto output = asm_output.value();

  Hack hack{};
  hack.load_rom(output);

  // for (;;) {
  //   try {
  //     hack.tick();
  //   } catch (...) {
  //     std::cout << "terminated\n";
  //     break;
  //   }
  // }

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

    hack.tick();
    draw_screen(renderer, hack.get_screen_mmap());
  }
}

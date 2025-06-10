#ifndef HACK_HPP
#define HACK_HPP

#include <array>
#include <cstdint>
#include <span>
#include <vector>

using ScreenSpan = std::span<std::uint16_t, 8160>;

struct Hack {
  // instruction memory
  std::array<std::uint16_t, 32768> instruction_mem{0};
  // data memory
  //
  // The following is the memory layout
  // RAM:                0-0x3FFF
  // Screen Buffer MMAP: 0x4000-0x5FFF
  // Keyboard MMAP:      0x6000
  std::array<std::uint16_t, 16384 + 8192 + 1> data_mem{0};

  std::uint16_t pc{0};
  std::uint16_t address_reg{0}, data_reg{0};

  bool load_rom(std::vector<uint16_t> &instructions);

  // retrieves a span of the memory mapped screen buffer
  ScreenSpan get_screen_mmap();

  std::uint16_t &get_keyboard_mmap();

  // Throws an exception if an invalid instruction is ever reached
  void tick();
};

#endif

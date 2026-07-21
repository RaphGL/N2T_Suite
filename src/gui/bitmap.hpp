#ifndef N2T_GUI_BITMAP_HPP
#define N2T_GUI_BITMAP_HPP

#include "gui.hpp"
#include <array>

namespace gui::bitmap {

constexpr std::uint16_t default_height = 32, default_width = 32;
constexpr std::uint16_t max_height = 256, max_width = 512;

enum class ShiftDirection { Left, Right, Up, Down };

class ViewCtx final : public gui::BaseView {
   gui::Context *_ctx = nullptr;
   std::uint16_t _pixels_height = default_height;
   std::uint16_t _pixels_width = default_width;
   std::array<bool, max_width * max_height> _pixels { };

   void bitmap_refit(std::uint16_t prev_width, std::uint16_t prev_height);

   public:
   ViewCtx(gui::Context *ctx);

   void bitmap_invert();
   void bitmap_shift(ShiftDirection);
   bool &bitmap_at(std::uint16_t x, std::uint16_t y);
   void show() override;
   std::string_view view_name() const override;
};

}

#endif

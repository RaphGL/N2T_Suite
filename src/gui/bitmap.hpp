#ifndef N2T_GUI_BITMAP_HPP
#define N2T_GUI_BITMAP_HPP

#include "gui.hpp"

namespace gui::bitmap {

class ViewCtx final : public gui::BaseView {
   gui::Context *_ctx = nullptr;

   public:
   ViewCtx(gui::Context *ctx);
   void show() override;
   std::string_view view_name() const override;
};

}

#endif

#ifndef N2T_GUI_CPU_HPP
#define N2T_GUI_CPU_HPP

#include "../hack/hack.hpp"
#include "gui.hpp"
#include "widget/log.hpp"
#include "widget/memory_viewer.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <thread>

namespace gui::cpu {

enum class MemoryViewType : int {
   ROM,
   RAM,

   Count,
};

enum class MemoryViewOption {
   Asm,
   Hex,
   Bin,
   Dec,
};

enum class State {
   Off,
   Running,
   Stopped,
   StepThrough,
   Reset,
};

class ViewCtx final : public gui::BaseView {
   gui::Context *_ctx;
   widget::Log _logs;
   widget::MemoryViewer _rom_viewer;
   widget::MemoryViewer _ram_viewer;

   Hack _hack { };
   GLuint _hack_screen_tex = -1;
   std::atomic<State> _hack_state = State::Off;
   // how fast the processor runs
   float _hack_speed = 1.0f;
   std::jthread _hack_worker, _dialog_worker;

   void show_top_bar();
   void show_hack_screen();
   void show_memory_view(MemoryViewType type, int default_height);
   void show_hack_registers();
   void clear_hack_memory(MemoryViewType type);

   public:
   ViewCtx(gui::Context *ctx);
   ~ViewCtx();

   void show() override;
   std::string_view view_name() const override;
};

};

#endif

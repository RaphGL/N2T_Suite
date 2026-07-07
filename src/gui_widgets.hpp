#ifndef GUI_WIDGETS_CPP
#define GUI_WIDGETS_CPP

namespace gui {

enum class MemoryViewType : int {
   ROM,
   RAM,
};

void show_memory_view(MemoryViewType type);

}

#endif

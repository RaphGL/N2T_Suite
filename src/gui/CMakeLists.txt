add_library(n2t_gui
  gui.cpp  
  cpu.cpp
  bitmap.cpp
  widget/log.cpp
)

target_link_libraries(n2t_gui PUBLIC ImGUI)

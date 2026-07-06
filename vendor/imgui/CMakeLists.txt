cmake_minimum_required(VERSION 3.15)

project(ImGUI)

add_library(ImGUI STATIC
    imgui.cpp
    imgui_demo.cpp
    imgui_draw.cpp
    imgui_tables.cpp
    imgui_widgets.cpp
    backends/imgui_impl_sdl3.cpp
    backends/imgui_impl_opengl3.cpp
)

target_include_directories(ImGUI PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.15)

project(n2t_suite
  VERSION 0.1
  LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  set(SANITIZERS -fsanitize=address,undefined)
  add_link_options(${SANITIZERS})
  add_compile_options(${SANITIZERS})
  message(STATUS "Enabled sanitizers with: ${SANITIZERS}")
  add_compile_options()
endif()

add_subdirectory(vendor/imgui)
add_subdirectory(src/hdl)
add_subdirectory(src/report)
add_subdirectory(src/asm)
add_subdirectory(src/hack)
add_subdirectory(src/gui)

add_executable(n2t
  src/main.cpp
)

find_package(SDL3 REQUIRED CONFIG REQUIRED COMPONENTS SDL3)
find_package(OpenGL REQUIRED)

target_link_libraries(n2t PRIVATE SDL3::SDL3 OpenGL::GL)
target_link_libraries(n2t PUBLIC
  ImGUI
  n2t_hdl
  n2t_report
  n2t_asm
  n2t_hack
  n2t_gui
)


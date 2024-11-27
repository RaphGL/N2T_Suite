cmake_minimum_required(VERSION 3.15)

project(n2t_suite
  VERSION 0.1
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

if (CMAKE_BUILD_TYPE STREQUAL "")
  set(CMAKE_BUILD_TYPE Debug)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  set(SANITIZERS -fsanitize=address,undefined)
  add_link_options(${SANITIZERS})
  add_compile_options(${SANITIZERS})
  message(STATUS "Enabled sanitizers with: ${SANITIZERS}")
  add_compile_options()
endif()

add_subdirectory(src/hdl)
add_subdirectory(src/report)

add_executable(n2t
  src/main.cpp
)

target_link_libraries(n2t PUBLIC n2t_hdl n2t_report)

cmake_minimum_required(VERSION 3.15)

project(n2t_suite)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  set(SANITIZERS -fsanitize=address,undefined)
  add_link_options(${SANITIZERS})
  add_compile_options(${SANITIZERS})
  message(STATUS "Enabled sanitizers with: ${SANITIZERS}")
endif()

add_subdirectory(src/hdl)

add_executable(n2t
  src/main.cpp
)

target_compile_features(n2t PUBLIC cxx_std_20)

target_link_libraries(n2t PUBLIC n2t_hdl)

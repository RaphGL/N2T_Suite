add_library(n2t_hdl
  parser.cpp
  lexer.cpp
)

target_compile_features(n2t_hdl PUBLIC cxx_std_20)

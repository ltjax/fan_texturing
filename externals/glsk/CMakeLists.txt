add_library(glsk
  source/config.cpp
  source/win32_core.cpp
  source/win32_input.cpp
  source/win32_window.cpp
  source/win32_keycodes.cpp
  source/pbuffer.cpp
  source/win32_main.cpp)
  
target_include_directories(glsk
  PUBLIC include/)

target_link_libraries(glsk
  PUBLIC CONAN_PKG::boost_utility
  PUBLIC CONAN_PKG::boost_tuple
  PUBLIC CONAN_PKG::boost_signals
  PUBLIC CONAN_PKG::boost_range
  PUBLIC CONAN_PKG::boost_assign)

target_compile_definitions(glsk
  PUBLIC -DBOOST_SIGNALS_NO_DEPRECATION_WARNING)

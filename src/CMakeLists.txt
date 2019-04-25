add_executable(fan_texturing_demo
  ApplicationKernel.cpp
  ApplicationKernel.hpp
  Caching.cpp
  Caching.hpp
  Common.hpp
  Cursor.cpp
  Cursor.hpp
  FanDrawer.cpp
  FanDrawer.hpp
  FantexMesh.cpp
  FantexMesh.hpp
  FantexRenderer.cpp
  FantexRenderer.hpp
  FileMonitor.cpp
  FileMonitor.hpp
  GLSLUtils.cpp
  GLSLUtils.hpp
  ImportObj.cpp
  ImportObj.hpp
  Index.hpp
  main.cpp
  Misc.cpp
  Misc.hpp
  PageAtlas.cpp
  PageAtlas.hpp
  PageScheduler.cpp
  PageScheduler.hpp
  RenderWindow.hpp
  ShaderTestKernel.cpp
  ShaderTestKernel.hpp
#  ShaderTestMain.cpp
  ShadowmapRenderer.cpp
  ShadowmapRenderer.hpp
  TextureGenerator.cpp
  TextureGenerator.hpp
  TextureWriter.cpp
  TextureWriter.hpp
  TSM.cpp
  TSM.hpp
  UserCamera.cpp
  UserCamera.hpp
  WireframeRenderer.cpp
  WireframeRenderer.hpp)


target_link_libraries(fan_texturing_demo
  PUBLIC glsk
  PUBLIC replay
  PUBLIC threadpool
  PUBLIC squish
  PUBLIC CONAN_PKG::glew
  PUBLIC CONAN_PKG::boost_filesystem
  PUBLIC CONAN_PKG::boost_assign
  PUBLIC CONAN_PKG::boost_bind
  PUBLIC CONAN_PKG::boost_date_time
  PUBLIC CONAN_PKG::boost_array
  PUBLIC CONAN_PKG::boost_program_options
  PUBLIC CONAN_PKG::boost_asio
  PUBLIC CONAN_PKG::boost_graph)

target_compile_options(fan_texturing_demo
  PUBLIC /wd4267 /wd4996 /wd4101)

target_compile_definitions(fan_texturing_demo
  PUBLIC -D_WIN32_WINDOWS=0x0601 -D_WIN32_WINNT=0x0601)
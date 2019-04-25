add_library(replay
  source/aabb.cpp
  source/box_packer.cpp
  source/byte_color.cpp
  source/math.cpp
  source/matrix2.cpp
  source/matrix3.cpp
  source/matrix4.cpp
  source/pixbuf.cpp
  source/pixbuf_io.cpp
  source/plane3.cpp
  source/quaternion.cpp
#  source/scalar.cpp
  source/vector_math.cpp)

target_include_directories(replay
  PUBLIC include/)

target_link_libraries(replay
  PUBLIC CONAN_PKG::boost_math
  PUBLIC CONAN_PKG::boost_utility
  PUBLIC CONAN_PKG::boost_filesystem
  PUBLIC CONAN_PKG::libpng)

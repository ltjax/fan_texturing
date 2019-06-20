add_subdirectory(replay)
add_subdirectory(squish)

add_library(threadpool INTERFACE)
target_include_directories(threadpool
  INTERFACE threadpool/)

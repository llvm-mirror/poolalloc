include_directories(include)
configure_file(include/poolalloc/Config/config.h.cmake
               include/poolalloc/Config/config.h)
add_subdirectory(lib)
add_subdirectory(runtime)
add_subdirectory(tools)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include
                    ${CMAKE_CURRENT_BINARY_DIR}/include)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/include/poolalloc/Config/config.h.cmake
               ${CMAKE_CURRENT_BINARY_DIR}/include/poolalloc/Config/config.h)
add_subdirectory(lib)
add_subdirectory(runtime)
add_subdirectory(tools)
add_subdirectory(test)

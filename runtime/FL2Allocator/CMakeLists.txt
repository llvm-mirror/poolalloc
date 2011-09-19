include_directories(/localhome/simmon12/progs/dyncall-0.5/dyncall)
link_directories(/localhome/simmon12/progs/dyncall-0.5/dyncall/build_out/linux_x86_gcc_release)
add_llvm_library( poolalloc_rt PoolAllocator.cpp )
set_property(
   TARGET poolalloc_rt
   PROPERTY COMPILE_DEFINITIONS
   )
target_link_libraries( poolalloc_rt dyncall_s )

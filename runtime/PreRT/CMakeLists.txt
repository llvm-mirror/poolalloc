file(GLOB sources *.cpp *.c)
add_llvm_library( pa_pre_rt ${sources} )

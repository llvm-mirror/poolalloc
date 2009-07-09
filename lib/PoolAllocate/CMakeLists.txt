file(GLOB sources *.cpp)
add_llvm_library(poolalloc ${sources})
target_link_libraries(poolalloc LLVMDataStructure)
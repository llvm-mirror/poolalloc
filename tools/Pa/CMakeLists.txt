set(LLVM_LINK_COMPONENTS bitreader bitwriter instrumentation scalaropts ipo nativecodegen)
file(GLOB sources *.cpp)
add_llvm_tool( pa ${sources} )
target_link_libraries(pa LLVMDataStructure poolalloc)

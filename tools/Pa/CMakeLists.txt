set(LLVM_LINK_COMPONENTS bitreader bitwriter instrumentation scalaropts ipo nativecodegen)
add_definitions(-fno-exceptions)
add_llvm_tool( pa pa.cpp )
target_link_libraries(pa LLVMDataStructure poolalloc)

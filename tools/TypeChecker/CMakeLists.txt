set(LLVM_LINK_COMPONENTS bitreader bitwriter instrumentation scalaropts ipo nativecodegen)
add_definitions(-fno-exceptions)
add_llvm_tool( tc tc.cpp )
target_link_libraries(tc LLVMDataStructure AssistDS )

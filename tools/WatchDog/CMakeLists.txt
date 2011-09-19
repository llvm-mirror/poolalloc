set(LLVM_LINK_COMPONENTS bitreader bitwriter instrumentation scalaropts ipo nativecodegen)
add_definitions(-fno-exceptions)
add_llvm_tool( watchdog WatchDog.cpp )

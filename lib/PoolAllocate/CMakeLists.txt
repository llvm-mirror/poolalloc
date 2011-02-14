add_llvm_library(poolalloc
  AccessTrace.cpp
  AllNodesHeuristic.cpp
  Heuristic.cpp
  PAMultipleGlobalPool.cpp
  PASimple.cpp
  PointerCompress.cpp
  PoolAllocate.cpp
  PoolOptimize.cpp
  RunTimeAssociate.cpp
  TransformFunctionBody.cpp
  )
target_link_libraries(poolalloc LLVMDataStructure)

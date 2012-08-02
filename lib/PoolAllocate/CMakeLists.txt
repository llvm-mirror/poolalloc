set(SOURCES
  AccessTrace.cpp
  AllHeapNodesHeuristic.cpp
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

# Build both shared and static libs
# Also, drop the 'lib' suffix to match how
# the Makefile-driven version functions.

if( NOT WIN32 AND LLVM_ENABLE_PIC )
  set(bsl ${BUILD_SHARED_LIBS})
  set(BUILD_SHARED_LIBS ON)
  add_llvm_library(poolalloc ${SOURCES})
  set(BUILD_SHARED_LIBS ${bsl})
  set_property(TARGET poolalloc PROPERTY OUTPUT_NAME "poolalloc")
  set_property(TARGET poolalloc PROPERTY PREFIX "")
  set(PA_STATIC_TARGET poolalloc_static)
  add_dependencies(poolalloc intrinsics_gen)
  target_link_libraries(poolalloc LLVMDataStructure)
else()
  set(PA_STATIC_TARGET poolalloc)
endif()

if( NOT BUILD_SHARED_LIBS )
  add_llvm_library(${PA_STATIC_TARGET} ${SOURCES})
  set_property(TARGET ${PA_STATIC_TARGET} PROPERTY OUTPUT_NAME "poolalloc")
  set_property(TARGET ${PA_STATIC_TARGET} PROPERTY PREFIX "")
  add_dependencies(${PA_STATIC_TARGET} intrinsics_gen)
  target_link_libraries(${PA_STATIC_TARGET} LLVMDataStructure)
endif()

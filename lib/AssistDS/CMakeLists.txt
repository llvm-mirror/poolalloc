set(SOURCES
  ArgCast.cpp
  ArgSimplify.cpp
  DSNodeEquivs.cpp
  DataStructureCallGraph.cpp
  Devirt.cpp
  DynCount.cpp
  FuncSimplify.cpp
  FuncSpec.cpp
  GEPExprArgs.cpp
  IndCloner.cpp
  Int2PtrCmp.cpp
  LoadArgs.cpp
  MergeGEP.cpp
  SVADevirt.cpp
  SimplifyExtractValue.cpp
  SimplifyGEP.cpp
  SimplifyInsertValue.cpp
  SimplifyLoad.cpp
  StructReturnToPointer.cpp
  TypeChecks.cpp
  TypeChecksOpt.cpp
  )

# Build both shared and static libs
# Also, drop the 'lib' suffix to match how
# the Makefile-driven version functions.

if( NOT WIN32 AND LLVM_ENABLE_PIC )
  set(bsl ${BUILD_SHARED_LIBS})
  set(BUILD_SHARED_LIBS ON)
  add_llvm_library(AssistDS ${SOURCES})
  set(BUILD_SHARED_LIBS ${bsl})
  set_property(TARGET AssistDS PROPERTY OUTPUT_NAME "AssistDS")
  set_property(TARGET AssistDS PROPERTY PREFIX "")
  set(AssistDS_STATIC_TARGET AssistDS_static)
  add_dependencies(AssistDS intrinsics_gen)
else()
  set(AssistDS_STATIC_TARGET AssistDS)
endif()

if( NOT BUILD_SHARED_LIBS )
  add_llvm_library(${AssistDS_STATIC_TARGET} ${SOURCES})
  set_property(TARGET ${AssistDS_STATIC_TARGET} PROPERTY OUTPUT_NAME "AssistDS")
  set_property(TARGET ${AssistDS_STATIC_TARGET} PROPERTY PREFIX "")
  add_dependencies(${AssistDS_STATIC_TARGET} intrinsics_gen)
endif()

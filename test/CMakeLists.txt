# Test runner infrastructure for PoolAlloc/DSA. This configures the PA test trees
# for use by Lit, and delegates to LLVM's lit test handlers.

set(PROJ_SRC_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(PROJ_OBJ_ROOT "${CMAKE_CURRENT_BINARY_DIR}/..")

configure_lit_site_cfg(
  ${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.in
  ${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg
  )

if( PATH_TO_LLVM_BUILD )
  set(POOLALLOC_TEST_EXTRA_ARGS "--path=${CMAKE_CURRENT_BINARY_DIR}/tools/")
endif()

option(POOLALLOC_TEST_USE_VG "Run PoolAlloc/DSA tests under Valgrind" OFF)
if(POOLALLOC_TEST_USE_VG)
  set(POOLALLOC_TEST_EXTRA_ARGS ${POOLALLOC_TEST_EXTRA_ARGS} "--vg")
endif ()

set(POOLALLOC_TEST_DEPS
  clang opt FileCheck llc not
  LLVMDataStructure AssistDS
  poolalloc poolalloc_rt
  )

# TODO: Add LLVM_INCLUDE_TESTS support?
#if(LLVM_INCLUDE_TESTS)
#  list(APPEND CLANG_TEST_DEPS ClangUnitTests)
#  list(APPEND CLANG_TEST_PARAMS
#    clang_unit_site_config=${CMAKE_CURRENT_BINARY_DIR}/Unit/lit.site.cfg
#    )

add_lit_testsuite(check-poolalloc "Running the PoolAlloc/DSA regression tests"
  ${CMAKE_CURRENT_BINARY_DIR}
  DEPENDS ${POOLALLOC_TEST_DEPS}
  ARGS ${POOLALLOC_TEST_EXTRA_ARGS}
  )
set_target_properties(check-poolalloc PROPERTIES FOLDER "PoolAlloc/DSA tests")

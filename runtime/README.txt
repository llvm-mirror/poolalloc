//===---------------------------------------------------------------------===//
This directory contains the pool allocator runtime library implementations.

The only fully maintained runtime library implementation is in the FL2Allocator
directory.  This supports the pool allocator, the bump pointer optimization,
and the pointer compression runtime.

The implementation in the FreeListAllocator directory is much slower than
the FL2Allocator and has not been updated.

The implementation in the PoolAllocator directory is also probably out of date,
and is much slower than FL2, but is used by the SAFECode project, which cannot
allow pool metadata to be stored intermixed with program data.



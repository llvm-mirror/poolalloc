;RUN: paopt %s -paheur-AllButUnreachableFromMemory -poolalloc -disable-output 2>&1
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  unreachable
}

define void @attr(i16 zeroext %IntParam, i8* %NeedsPool) {
entry:
  unreachable
}

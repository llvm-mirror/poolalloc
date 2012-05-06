;RUN: dsaopt %s -dsa-local -disable-output
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define void @A() {
entry:
  %0 = alloca i32
  %1 = ptrtoint i32* %0 to i64
  br label %loop

loop:
  %2 = phi i64 [ %1, %entry ], [ %2, %loop ]
  br label %loop

ret:
  ret void
}

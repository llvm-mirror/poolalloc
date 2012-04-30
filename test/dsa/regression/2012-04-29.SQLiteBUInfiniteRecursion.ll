;RUN: dsaopt %s -dsa-bu -disable-output
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define internal i32* @func(i32* %arg) nounwind {
entry:
  %0 = call i32* @A() nounwind
  ret i32* %0
}

define internal i32* @C(i32* (i32*)* %f, i32* %arg) nounwind {
entry:
  %0 = call i32* @A() nounwind
  %1 = call i32* @B() nounwind
  %2 = call i32* @func(i32* %1) nounwind
  %3 = call i32* %f(i32* %arg) nounwind
  ret i32* %3
}

define internal i32* @B() nounwind {
entry:
  %0 = call i32* @A() nounwind
  %1 = call i32* @C(i32* (i32*)* @func, i32* %0) nounwind
  ret i32* %1
}

define internal i32* @A() nounwind {
entry:
  %0 = call i32* @B() nounwind
  %1 = call i32* @func(i32* %0) nounwind
  ret i32* %1
}

define internal i32* @D() nounwind {
entry:
  %0 = call i32* @A() nounwind
  ret i32* %0
}

;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=A,B
;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=B,C
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=A,B
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=B,C

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define void @C() {
entry:
  ret void
}

define void @B() {
entry:
  call void @C()
  ret void
}

define void @A() {
entry:
   call void @B()
   ret void
}

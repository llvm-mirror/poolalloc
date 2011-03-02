;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=C,A,B

; ModuleID = 'scc.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define internal void @A() nounwind {
entry:
  call void @B() nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @B() nounwind {
entry:
  call void @A() nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @C() nounwind {
entry:
  call void @A() nounwind
  call void @B() nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

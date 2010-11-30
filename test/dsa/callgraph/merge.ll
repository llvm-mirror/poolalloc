;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=E,B,D,A
; ModuleID = 'merge.o'
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
  call void @D() nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @D() nounwind {
entry:
  call void @C() nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @E() nounwind {
entry:
  %fp = alloca void ()*                           ; <void ()**> [#uses=3]
  %fp1 = alloca void ()*                          ; <void ()**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store void ()* @B, void ()** %fp, align 8
  store void ()* @A, void ()** %fp1, align 8
  store void ()* @D, void ()** %fp, align 8
  %0 = load void ()** %fp, align 8                ; <void ()*> [#uses=1]
  call void %0() nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=main,A
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=main,A

; ModuleID = 'fptr1.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define internal void @A(void (i8*)** %f) nounwind {
entry:
  %f_addr = alloca void (i8*)**                   ; <void (i8*)***> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store void (i8*)** %f, void (i8*)*** %f_addr
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @B(void (i8*)** %f) nounwind {
entry:
  %f_addr = alloca void (i8*)**                   ; <void (i8*)***> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store void (i8*)** %f, void (i8*)*** %f_addr
  br label %return

return:                                           ; preds = %entry
  ret void
}

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=1]
  %F = alloca void (i8*)*                         ; <void (i8*)**> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store void (i8*)* bitcast (void (void (i8*)**)* @A to void (i8*)*), void (i8*)** %F, align 8
  %0 = load void (i8*)** %F, align 8              ; <void (i8*)*> [#uses=1]
  %F1 = bitcast void (i8*)** %F to i8*            ; <i8*> [#uses=1]
  call void %0(i8* %F1) nounwind
  br label %return

return:                                           ; preds = %entry
  %retval2 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval2
}

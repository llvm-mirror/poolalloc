;RUN: dsaopt %s -dsa-td -analyze -check-callees=main,B
;RUN: dsaopt %s -dsa-td -analyze -check-not-callees=main,A
; ModuleID = 'calltargets.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

@FP = internal global i32* (i32)* null              ; <i32* (i32)**> [#uses=3]

define internal i32* @B(i32 %i) nounwind {
entry:
  %i_addr = alloca i32                            ; <i32*> [#uses=1]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %i, i32* %i_addr
  store i32* null, i32** %0, align 8
  %1 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %1, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define internal i32* @A(float %f) nounwind {
entry:
  %f_addr = alloca float                          ; <float*> [#uses=1]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store float %f, float* %f_addr
  store i32* null, i32** %0, align 8
  %1 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %1, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define void @main() nounwind {
entry:
  %i = alloca i32                                 ; <i32*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* (i32)* @B, i32* (i32)** @FP, align 8
  store i32* (i32)* bitcast (i32* (float)* @A to i32* (i32)*), i32* (i32)** @FP, align 8
  store i32 5, i32* %i, align 4
  %0 = load i32* (i32)** @FP, align 8             ; <i32* (i32)*> [#uses=1]
  %1 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %2 = call i32* %0(i32 %1) nounwind              ; <i32*> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  ret void
}

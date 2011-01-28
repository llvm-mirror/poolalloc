; Pass same pointer(from main) as 2 different args to func
; The formal args to func do not alias in func
; Verify that the node is merged in TD

;RUN: dsaopt %s -dsa-td -analyze -check-same-node=func:arg1:0,func:arg2
;RUN: dsaopt %s -dsa-bu -analyze -check-not-same-node=func:arg1:0,func:arg2

; ModuleID = 'mergeArgs.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define void @func(i32** %arg1, i32* %arg2) nounwind {
entry:
  %arg1_addr = alloca i32**                       ; <i32***> [#uses=2]
  %arg2_addr = alloca i32*                        ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32** %arg1, i32*** %arg1_addr
  store i32* %arg2, i32** %arg2_addr
  %0 = load i32*** %arg1_addr, align 8            ; <i32**> [#uses=1]
  %1 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32 1, i32* %1, align 4
  %2 = load i32** %arg2_addr, align 8             ; <i32*> [#uses=1]
  store i32 2, i32* %2, align 4
  br label %return

return:                                           ; preds = %entry
  ret void
}

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=1]
  %p = alloca i32*                                ; <i32**> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  store i32* %1, i32** %p, align 8
  %2 = load i32** %p, align 8                     ; <i32*> [#uses=1]
  call void @func(i32** %p, i32* %2) nounwind
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

declare noalias i8* @malloc(i64) nounwind

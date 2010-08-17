
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:arr:0,func:b:0
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:arr,func:d:0,func:c:0

; ModuleID = 'arrays.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define void @func() nounwind {
entry:
  %arr = alloca i32*                              ; <i32**> [#uses=5]
  %i = alloca i32                                 ; <i32*> [#uses=6]
  %b = alloca i32*                                ; <i32**> [#uses=1]
  %c = alloca i32**                               ; <i32***> [#uses=1]
  %d = alloca i32**                               ; <i32***> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call noalias i8* @malloc(i64 40) nounwind  ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  store i32* %1, i32** %arr, align 8
  store i32 0, i32* %i, align 4
  br label %bb1

bb:                                               ; preds = %bb1
  %2 = load i32** %arr, align 8                   ; <i32*> [#uses=1]
  %3 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %4 = sext i32 %3 to i64                         ; <i64> [#uses=1]
  %5 = getelementptr inbounds i32* %2, i64 %4     ; <i32*> [#uses=1]
  %6 = load i32* %i, align 4                      ; <i32> [#uses=1]
  store i32 %6, i32* %5, align 1
  %7 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %8 = add nsw i32 %7, 1                          ; <i32> [#uses=1]
  store i32 %8, i32* %i, align 4
  br label %bb1

bb1:                                              ; preds = %bb, %entry
  %9 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %10 = icmp sle i32 %9, 9                        ; <i1> [#uses=1]
  br i1 %10, label %bb, label %bb2

bb2:                                              ; preds = %bb1
  %11 = load i32** %arr, align 8                  ; <i32*> [#uses=1]
  %12 = getelementptr inbounds i32* %11, i64 5    ; <i32*> [#uses=1]
  store i32* %12, i32** %b, align 8
  store i32** %arr, i32*** %c, align 8
  %13 = getelementptr inbounds i32** %arr, i64 4  ; <i32**> [#uses=1]
  store i32** %13, i32*** %d, align 8
  br label %return

return:                                           ; preds = %bb2
  ret void
}

declare noalias i8* @malloc(i64) nounwind

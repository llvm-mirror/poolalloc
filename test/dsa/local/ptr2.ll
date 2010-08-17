
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:a,func:a1,func:d:0,func:f:0
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:a,func:b:0,func:b1:0
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:b,func:b1
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:c:0,func:b,func:b1,func:g:0
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:c:0,func:b,func:b1,func:g:0

; ModuleID = 'ptr2.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define void @func() nounwind {
entry:
  %a = alloca i32                                 ; <i32*> [#uses=3]
  %a1 = alloca i32                                ; <i32*> [#uses=3]
  %b = alloca i32*                                ; <i32**> [#uses=2]
  %b1 = alloca i32*                               ; <i32**> [#uses=2]
  %c = alloca i32**                               ; <i32***> [#uses=4]
  %d = alloca i32*                                ; <i32**> [#uses=3]
  %e = alloca i32                                 ; <i32*> [#uses=1]
  %f = alloca i32*                                ; <i32**> [#uses=1]
  %g = alloca i32**                               ; <i32***> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 10, i32* %a, align 4
  store i32 20, i32* %a1, align 4
  store i32* %a, i32** %b, align 8
  store i32* %a1, i32** %b1, align 8
  %0 = load i32* %a, align 4                      ; <i32> [#uses=1]
  %1 = load i32* %a1, align 4                     ; <i32> [#uses=1]
  %2 = icmp sgt i32 %0, %1                        ; <i1> [#uses=1]
  br i1 %2, label %bb, label %bb1

bb:                                               ; preds = %entry
  store i32** %b1, i32*** %c, align 8
  br label %bb2

bb1:                                              ; preds = %entry
  store i32** %b, i32*** %c, align 8
  br label %bb2

bb2:                                              ; preds = %bb1, %bb
  %3 = load i32*** %c, align 8                    ; <i32**> [#uses=1]
  %4 = load i32** %3, align 8                     ; <i32*> [#uses=1]
  store i32* %4, i32** %d, align 8
  %5 = load i32** %d, align 8                     ; <i32*> [#uses=1]
  %6 = load i32* %5, align 4                      ; <i32> [#uses=1]
  store i32 %6, i32* %e, align 4
  %7 = load i32** %d, align 8                     ; <i32*> [#uses=1]
  store i32* %7, i32** %f, align 8
  %8 = load i32*** %c, align 8                    ; <i32**> [#uses=1]
  store i32** %8, i32*** %g, align 8
  br label %return

return:                                           ; preds = %bb2
  ret void
}

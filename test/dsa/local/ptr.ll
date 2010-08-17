
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:b:0,func:a,func:d:0
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:c:0,func:b


; ModuleID = 'ptr.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define void @func() nounwind {
entry:
  %a = alloca i32                                 ; <i32*> [#uses=2]
  %b = alloca i32*                                ; <i32**> [#uses=2]
  %c = alloca i32**                               ; <i32***> [#uses=2]
  %d = alloca i32*                                ; <i32**> [#uses=2]
  %e = alloca i32                                 ; <i32*> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 10, i32* %a, align 4
  store i32* %a, i32** %b, align 8
  store i32** %b, i32*** %c, align 8
  %0 = load i32*** %c, align 8                    ; <i32**> [#uses=1]
  %1 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %1, i32** %d, align 8
  %2 = load i32** %d, align 8                     ; <i32*> [#uses=1]
  %3 = load i32* %2, align 4                      ; <i32> [#uses=1]
  store i32 %3, i32* %e, align 4
  br label %return

return:                                           ; preds = %entry
  ret void
}

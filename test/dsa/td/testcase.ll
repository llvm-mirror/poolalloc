;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:a1:0+I"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "func:a1:0+HM-I"

; ModuleID = 'testcase.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define internal i32* @test() nounwind {
entry:
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %a2 = alloca i32*                               ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %2 = bitcast i8* %1 to i32*                     ; <i32*> [#uses=1]
  store i32* %2, i32** %a2, align 8
  %3 = load i32** %a2, align 8                    ; <i32*> [#uses=1]
  store i32* %3, i32** %0, align 8
  %4 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %4, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

declare noalias i8* @malloc(i64) nounwind

define void @func() nounwind {
entry:
  %a1 = alloca i32*                               ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call i32* @test() nounwind                 ; <i32*> [#uses=1]
  store i32* %0, i32** %a1, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

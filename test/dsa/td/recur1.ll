;RUN: dsaopt %s -dsa-td -analyze -verify-flags "test:b2:0+HM"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "test:b2:0+HI"

; ModuleID = 'recur1.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32* @test1(i32* %b3) nounwind {
entry:
  %b3_addr = alloca i32*                          ; <i32**> [#uses=2]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* %b3, i32** %b3_addr
  %1 = load i32** %b3_addr, align 8               ; <i32*> [#uses=1]
  %2 = getelementptr inbounds i32* %1, i64 1      ; <i32*> [#uses=1]
  store i32* %2, i32** %0, align 8
  %3 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %3, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define i32* @test() nounwind {
entry:
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=3]
  %a2 = alloca i32*                               ; <i32**> [#uses=3]
  %b2 = alloca i32*                               ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %2 = bitcast i8* %1 to i32*                     ; <i32*> [#uses=1]
  store i32* %2, i32** %a2, align 8
  %3 = load i32** %a2, align 8                    ; <i32*> [#uses=1]
  store i32 10, i32* %3, align 4
  %4 = load i32** %a2, align 8                    ; <i32*> [#uses=1]
  %5 = load i32* %4, align 4                      ; <i32> [#uses=1]
  %6 = icmp sgt i32 %5, 5                         ; <i1> [#uses=1]
  br i1 %6, label %bb, label %bb1

bb:                                               ; preds = %entry
  %7 = call i32* @test() nounwind                 ; <i32*> [#uses=1]
  store i32* %7, i32** %0, align 8
  br label %bb2

bb1:                                              ; preds = %entry
  %8 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %9 = bitcast i8* %8 to i32*                     ; <i32*> [#uses=1]
  store i32* %9, i32** %b2, align 8
  %10 = load i32** %b2, align 8                   ; <i32*> [#uses=1]
  %11 = call i32* @test1(i32* %10) nounwind       ; <i32*> [#uses=1]
  store i32* %11, i32** %0, align 8
  br label %bb2

bb2:                                              ; preds = %bb1, %bb
  %12 = load i32** %0, align 8                    ; <i32*> [#uses=1]
  store i32* %12, i32** %retval, align 8
  br label %return

return:                                           ; preds = %bb2
  %retval3 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval3
}

declare noalias i8* @malloc(i64) nounwind

define void @func() nounwind {
entry:
  %a1 = alloca i32*                               ; <i32**> [#uses=1]
  %b1 = alloca i32*                               ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call i32* @test() nounwind                 ; <i32*> [#uses=1]
  store i32* %0, i32** %a1, align 8
  %1 = call i32* @test() nounwind                 ; <i32*> [#uses=1]
  store i32* %1, i32** %b1, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

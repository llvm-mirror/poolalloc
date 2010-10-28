;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "test:b2:0+I"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "test:b2:0+HM-I"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "test:a2:0+I"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "test:a2:0+HM-I"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "test:a2+IMR-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "test:a2+SMRE-I"


;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "func:a1+SM-IE"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "func:a1:0+I-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "func:a1+SM-I-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "func:a1:0+HM-IE"

;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "func:b1+SM-IE"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "func:b1:0+I-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "func:b1+SM-IE"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "func:b1:0+HM-IE"

;RUN: dsaopt %s -dsa-td -analyze -check-same-node=func:mem1,func:mem2
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "func:mem1:0+HI-E"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "func:mem2:0+HI-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "func:mem1:0+HM-IE"

; ModuleID = 'call2.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32* @test(i32** %a2, i32** %b2) nounwind {
entry:
  %a2_addr = alloca i32**                         ; <i32***> [#uses=4]
  %b2_addr = alloca i32**                         ; <i32***> [#uses=3]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %temp = alloca i32*                             ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32** %a2, i32*** %a2_addr
  store i32** %b2, i32*** %b2_addr
  %1 = load i32*** %a2_addr, align 8              ; <i32**> [#uses=1]
  %2 = load i32** %1, align 8                     ; <i32*> [#uses=1]
  store i32* %2, i32** %temp, align 8
  %3 = load i32*** %b2_addr, align 8              ; <i32**> [#uses=1]
  store i32** %3, i32*** %a2_addr, align 8
  %4 = load i32*** %b2_addr, align 8              ; <i32**> [#uses=1]
  %5 = load i32** %temp, align 8                  ; <i32*> [#uses=1]
  store i32* %5, i32** %4, align 8
  %6 = load i32*** %a2_addr, align 8              ; <i32**> [#uses=1]
  %7 = load i32** %6, align 8                     ; <i32*> [#uses=1]
  store i32* %7, i32** %0, align 8
  %8 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %8, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define void @func() nounwind {
entry:
  %mem1 = alloca i32*                             ; <i32**> [#uses=3]
  %mem2 = alloca i32*                             ; <i32**> [#uses=3]
  %a1 = alloca i32*                               ; <i32**> [#uses=1]
  %b1 = alloca i32*                               ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  store i32* %1, i32** %mem1, align 8
  %2 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %3 = bitcast i8* %2 to i32*                     ; <i32*> [#uses=1]
  store i32* %3, i32** %mem2, align 8
  %4 = call i32* @test(i32** %mem1, i32** %mem2) nounwind ; <i32*> [#uses=1]
  store i32* %4, i32** %a1, align 8
  %5 = call i32* @test(i32** %mem1, i32** %mem2) nounwind ; <i32*> [#uses=1]
  store i32* %5, i32** %b1, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

declare noalias i8* @malloc(i64) nounwind

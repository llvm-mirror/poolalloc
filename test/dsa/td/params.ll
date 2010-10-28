;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:mem1:0+SIM"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:mem2:0+SIM"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:a1+SM"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:b1+SM"

;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "func:mem1:0+SHMR"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "func:mem2:0+SHMR"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "func:a1+SM"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "func:b1+SM"

;RUN: dsaopt %s -dsa-bu -analyze -check-same-node=func:a1:0,func:b1:0,func:mem2:0,func:mem1:0,func:r1,func:r2

;RUN: dsaopt %s -dsa-td -analyze -verify-flags "func:mem1:0+SHMR"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "func:mem2:0+SHMR"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "func:a1+SM"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "func:b1+SM"

;RUN: dsaopt %s -dsa-td -analyze -check-same-node=func:a1:0,func:b1:0,func:mem2:0,func:mem1:0,func:r1,func:r2

;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "test:b2:0+HIR-E"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "test:a2:0+HIR-E"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "test:a2+SIMR-E"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "test:temp:0+HIR-E"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "test:temp1:0+HIR-E"
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=test:temp2:0,test:a2:0,test:temp1:0,test:temp:0,test:b2:0

;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "test:b2:0+HIMR-E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "test:a2:0+HIMR-E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "test:a2+SIMR-E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "test:temp+SMR-IE,test:temp:0+I-E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "test:temp1+SMR-IE,test:temp1:0+I-E"
;RUN: dsaopt %s -dsa-bu -analyze -check-same-node=test:temp2:0,test:a2:0,test:temp1:0,test:temp:0,test:b2:0
;RUN: dsaopt %s -dsa-bu -analyze -check-same-node=test:a2,test:temp2


;RUN: dsaopt %s -dsa-td -analyze -verify-flags "test:b2:0+SHEMR"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "test:a2:0+SHEMR"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "test:a2+SMRE"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "test:temp+SMR-IE,test:temp:0+E-I"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "test:temp1+SMR,test:temp:0+E-I"
;RUN: dsaopt %s -dsa-td -analyze -check-same-node=test:temp2:0,test:a2:0,test:temp1:0,test:temp:0,test:b2:0
;RUN: dsaopt %s -dsa-td -analyze -check-same-node=test:a2,test:b2,test:temp2

; ModuleID = 'params.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32* @test(i32** %a2, i32** %b2) nounwind {
entry:
  %a2_addr = alloca i32**                         ; <i32***> [#uses=4]
  %b2_addr = alloca i32**                         ; <i32***> [#uses=3]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=3]
  %temp = alloca i32*                             ; <i32**> [#uses=2]
  %temp1 = alloca i32*                            ; <i32**> [#uses=2]
  %temp2 = alloca i32*                            ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32** %a2, i32*** %a2_addr
  store i32** %b2, i32*** %b2_addr
  %1 = load i32*** %a2_addr, align 8              ; <i32**> [#uses=1]
  %2 = load i32** %1, align 8                     ; <i32*> [#uses=1]
  store i32* %2, i32** %temp, align 8
  %3 = load i32*** %b2_addr, align 8              ; <i32**> [#uses=1]
  %4 = load i32** %3, align 8                     ; <i32*> [#uses=1]
  store i32* %4, i32** %temp1, align 8
  %5 = load i32** %temp1, align 8                 ; <i32*> [#uses=1]
  %6 = load i32* %5, align 4                      ; <i32> [#uses=1]
  %7 = load i32** %temp, align 8                  ; <i32*> [#uses=1]
  %8 = load i32* %7, align 4                      ; <i32> [#uses=1]
  %9 = icmp slt i32 %6, %8                        ; <i1> [#uses=1]
  br i1 %9, label %bb, label %bb1

bb:                                               ; preds = %entry
  %10 = load i32*** %b2_addr, align 8             ; <i32**> [#uses=1]
  %11 = load i32** %10, align 8                   ; <i32*> [#uses=1]
  store i32* %11, i32** %0, align 8
  br label %bb2

bb1:                                              ; preds = %entry
  %12 = call noalias i8* @malloc(i64 4) nounwind  ; <i8*> [#uses=1]
  %13 = bitcast i8* %12 to i32*                   ; <i32*> [#uses=1]
  store i32* %13, i32** %temp2, align 8
  store i32** %temp2, i32*** %a2_addr, align 8
  %14 = load i32*** %a2_addr, align 8             ; <i32**> [#uses=1]
  %15 = load i32** %14, align 8                   ; <i32*> [#uses=1]
  store i32* %15, i32** %0, align 8
  br label %bb2

bb2:                                              ; preds = %bb1, %bb
  %16 = load i32** %0, align 8                    ; <i32*> [#uses=1]
  store i32* %16, i32** %retval, align 8
  br label %return

return:                                           ; preds = %bb2
  %retval3 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval3
}

declare noalias i8* @malloc(i64) nounwind

define void @func() nounwind {
entry:
  %mem1 = alloca i32*                             ; <i32**> [#uses=3]
  %mem2 = alloca i32*                             ; <i32**> [#uses=3]
  %r1 = alloca i32                                ; <i32*> [#uses=2]
  %r2 = alloca i32                                ; <i32*> [#uses=2]
  %a1 = alloca i32*                               ; <i32**> [#uses=1]
  %b1 = alloca i32*                               ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 5, i32* %r1, align 4
  store i32 6, i32* %r2, align 4
  store i32* %r1, i32** %mem1, align 8
  store i32* %r2, i32** %mem2, align 8
  %0 = call i32* @test(i32** %mem1, i32** %mem2) nounwind ; <i32*> [#uses=1]
  store i32* %0, i32** %a1, align 8
  %1 = call i32* @test(i32** %mem2, i32** %mem1) nounwind ; <i32*> [#uses=1]
  store i32* %1, i32** %b1, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

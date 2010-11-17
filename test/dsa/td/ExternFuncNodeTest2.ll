; Example in which we take address of an external function
; make sure that it is marked X(externFuncNode). 
; and that it is not inlined
; Checking that the X flag is propogated everywhere.

;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "@fp:0+X"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "@fp:0+X"
;RUN: dsaopt %s -dsa-cbu -analyze -verify-flags "@fp:0+X"
;RUN: dsaopt %s -dsa-eq -analyze -verify-flags "@fp:0+X"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "@fp:0+X"
;RUN: dsaopt %s -dsa-eqtd -analyze -verify-flags "@fp:0+X"

; ModuleID = 'ttt'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

@fp = internal global i8* (i32)* bitcast (i8* (i64)* @malloc to i8* (i32)*) ; <i8* (i32)**> [#uses=1]

declare noalias i8* @malloc(i64) nounwind

define internal void @init() nounwind {
entry:
  %0 = load i8* (i32)** @fp, align 8              ; <i8* (i32)*> [#uses=1]
  %1 = call i8* %0(i32 32) nounwind               ; <i8*> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  ret void
}

define void @main() nounwind {
entry:
  call void @init() nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

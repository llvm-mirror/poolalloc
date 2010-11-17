; Example in which we take address of an external function
; make sure that it is marked X(externFuncNode). 
; and that it is not inlined
; Checking that the X flag is propogated everywhere.
; Flag is preserved even after we create ECs

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

define internal i8* @internalFunc(i32 %a) nounwind {
entry:
  %a_addr = alloca i32                            ; <i32*> [#uses=1]
  %retval = alloca i8*                            ; <i8**> [#uses=2]
  %0 = alloca i8*                                 ; <i8**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %a, i32* %a_addr
  %1 = call noalias i8* @malloc(i64 32) nounwind  ; <i8*> [#uses=1]
  store i8* %1, i8** %0, align 8
  %2 = load i8** %0, align 8                      ; <i8*> [#uses=1]
  store i8* %2, i8** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i8** %retval                    ; <i8*> [#uses=1]
  ret i8* %retval1
}

define internal void @mergingFunc(i8* (i32)* %f) nounwind {
entry:
  %f_addr = alloca i8* (i32)*                     ; <i8* (i32)**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i8* (i32)* %f, i8* (i32)** %f_addr
  %0 = load i8* (i32)** %f_addr, align 8          ; <i8* (i32)*> [#uses=1]
  %1 = call i8* %0(i32 32) nounwind               ; <i8*> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal i32* @init() nounwind {
entry:
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = load i8* (i32)** @fp, align 8              ; <i8* (i32)*> [#uses=1]
  call void @mergingFunc(i8* (i32)* %1) nounwind
  %2 = call noalias i8* @malloc(i64 32) nounwind  ; <i8*> [#uses=1]
  %3 = bitcast i8* %2 to i32*                     ; <i32*> [#uses=1]
  store i32* %3, i32** %0, align 8
  %4 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %4, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define void @main() nounwind {
entry:
  %x = alloca i32*                                ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call i32* @init() nounwind                 ; <i32*> [#uses=1]
  store i32* %0, i32** %x, align 8
  call void @mergingFunc(i8* (i32)* @internalFunc) nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

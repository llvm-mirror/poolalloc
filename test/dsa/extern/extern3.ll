; ModuleID = 'extern.c'
;This tests the various cases that could involve lack of 'knowsAll' due
;to external code, and additionally verifies these flags are NOT set
;on internal functions.
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

declare i32* @getPointerExtern()
declare void @takesPointerExtern(i32*)
declare noalias i8* @malloc(i64) nounwind
declare void @free(i8*) nounwind

;This function is in theory externally callable (not internal linkage)
;so it's parameters/ret val should be marked external in TD.
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "getPointer:ptr+I-E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "getPointer:ptr+I-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "getPointer:ptr+E-I"
define i32* @getPointer() nounwind {
entry:
  %0 = tail call noalias i8* @malloc(i64 4) nounwind ; <i8*> [#uses=1]
  %ptr = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  store i32 5, i32* %ptr ; Throw off malloc-wrapper analysis
  ret i32* %ptr
}

;Since this isn't internal, should have external arguments in TD
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "takesPointer:ptr+I-E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "takesPointer:ptr+I-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "takesPointer:ptr+E-I"
define i32 @takesPointer(i32* %ptr) nounwind {
entry:
  %0 = load i32* %ptr, align 4                    ; <i32> [#uses=1]
  ret i32 %0
}

;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "getPointerInternal:ptr+I-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "getPointerInternal:ptr-I-E"
define internal i32* @getPointerInternal() nounwind {
entry:
  %0 = tail call noalias i8* @malloc(i64 4) nounwind ; <i8*> [#uses=1]
  %ptr = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  store i32 5, i32* %ptr ; Throw off malloc-wrapper analysis
  ret i32* %ptr
}

;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "takesPointerInternal:ptr+I-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "takesPointerInternal:ptr-IE"
define internal i32 @takesPointerInternal(i32* %ptr) nounwind {
entry:
  %0 = load i32* %ptr, align 4                    ; <i32> [#uses=1]
  ret i32 %0
}

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  call void @checkExterns() nounwind
  call void @checkExternals() nounwind
  call void @checkInternals() nounwind
  ret i32 0
}

;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "checkExterns:get+EI"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "checkExterns:take+EI"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "checkExterns:get+E-I"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "checkExterns:take+E-I"
define void @checkExterns() nounwind {
  %get = tail call i32* ()* @getPointerExtern() nounwind ; <i32*> [#uses=0]
  %1 = tail call noalias i8* @malloc(i64 4) nounwind ; <i8*> [#uses=2]
  %take = bitcast i8* %1 to i32*                     ; <i32*> [#uses=1]
  tail call void @takesPointerExtern(i32* %take) nounwind
  tail call void @free(i8* %1) nounwind
  ret void
}

;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "checkExternals:get+I-E"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "checkExternals:take+I-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "checkExternals:get-IE"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "checkExternals:take-IE"
define void @checkExternals() nounwind {
entry:
  %get = tail call i32* ()* @getPointer() nounwind ; <i32*> [#uses=0]
  %0 = tail call noalias i8* @malloc(i64 4) nounwind ; <i8*> [#uses=2]
  %take = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  tail call i32 @takesPointer(i32* %take) nounwind
  tail call void @free(i8* %0) nounwind
  ret void
}

;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "checkInternals:get+I-E"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "checkInternals:take+I-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "checkInternals:get-IE"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "checkInternals:take-IE"
define void @checkInternals() nounwind {
entry:
  %get = tail call i32* ()* @getPointerInternal() nounwind ; <i32*> [#uses=0]
  %0 = tail call noalias i8* @malloc(i64 4) nounwind ; <i8*> [#uses=2]
  %take = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  tail call i32 @takesPointerInternal(i32* %take) nounwind
  tail call void @free(i8* %0) nounwind
  ret void
}

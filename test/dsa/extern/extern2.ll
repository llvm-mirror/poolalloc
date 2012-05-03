;Here we get a pointer from an external function and pass it to a callee.
;We then test that the flags on that pointer are set appropriately.
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "main:ptr+IE"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "takesPointer:ptr+I-E"

;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "main:ptr-I+E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "takesPointer:ptr+I-E"

;RUN: dsaopt %s -dsa-td -analyze -verify-flags "main:ptr-I+E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "takesPointer:ptr-I+E"

;RUN: dsaopt %s -dsa-cbu -analyze -verify-flags "main:ptr-I+E"
;RUN: dsaopt %s -dsa-cbu -analyze -verify-flags "takesPointer:ptr+I-E"

;RUN: dsaopt %s -dsa-eq -analyze -verify-flags "main:ptr-I+E"
;RUN: dsaopt %s -dsa-eq -analyze -verify-flags "takesPointer:ptr+I-E"

;RUN: dsaopt %s -dsa-eqtd -analyze -verify-flags "main:ptr-I+E"
;RUN: dsaopt %s -dsa-eqtd -analyze -verify-flags "takesPointer:ptr-I+E"


target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define internal i32 @takesPointer(i32* nocapture %ptr) nounwind readonly {
entry:
  %0 = load i32* %ptr, align 4                    ; <i32> [#uses=1]
  ret i32 %0
}

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  %ptr = tail call i32* (...)* @getPointerExtern() nounwind
  %0 = tail call i32 @takesPointer(i32* %ptr) nounwind
  ret i32 %0
}

declare i32* @getPointerExtern(...)

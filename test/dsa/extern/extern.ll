;Test that pointers from external functions are marked incomplete properly
;Tests on all the DSA passes for three simple cases:
;--calling an allocator wrapper (shouldn't be incomplete in interprocedural)
;--calling a undefined (external) function.  Should be incomplete at least
;  once the interprocedural passes run.
;--calling a function that calls an undefined (external) function.
;  Should be incomplete, tests very basic tracking.
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "main:ptrExtern+IE"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "main:ptr+I-E"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "main:ptrViaExtern+I-E"

;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "main:ptrExtern-I+E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "main:ptr-IE"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "main:ptrViaExtern-I+E"

;RUN: dsaopt %s -dsa-td -analyze -verify-flags "main:ptrExtern-I+E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "main:ptr-IE"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "main:ptrViaExtern-I+E"

;RUN: dsaopt %s -dsa-cbu -analyze -verify-flags "main:ptrExtern-I+E"
;RUN: dsaopt %s -dsa-cbu -analyze -verify-flags "main:ptr-IE"
;RUN: dsaopt %s -dsa-cbu -analyze -verify-flags "main:ptrViaExtern-I+E"

;RUN: dsaopt %s -dsa-eq -analyze -verify-flags "main:ptrExtern-I+E"
;RUN: dsaopt %s -dsa-eq -analyze -verify-flags "main:ptr-IE"
;RUN: dsaopt %s -dsa-eq -analyze -verify-flags "main:ptrViaExtern-I+E"

;RUN: dsaopt %s -dsa-eqtd -analyze -verify-flags "main:ptrExtern-I+E"
;RUN: dsaopt %s -dsa-eqtd -analyze -verify-flags "main:ptr-IE"
;RUN: dsaopt %s -dsa-eqtd -analyze -verify-flags "main:ptrViaExtern-I+E"

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

;RUN: dsaopt %s -dsa-local -analyze -verify-flags "getPointerViaExtern:ptr+E"
define i32* @getPointerViaExtern() nounwind {
entry:
  %ptr = tail call i32* (...)* @getPointerExtern() nounwind ; <i32*> [#uses=1]
  ret i32* %ptr
}

declare i32* @getPointerExtern(...)

define internal noalias i32* @getPointer() nounwind {
entry:
  %0 = tail call noalias i8* @malloc(i64 4) nounwind ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  ret i32* %1
}

declare noalias i8* @malloc(i64) nounwind

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  %ptr = tail call i32* ()* @getPointer() nounwind
  %ptrExtern = tail call i32* (...)* @getPointerExtern() nounwind
  %ptrViaExtern = tail call i32* ()* @getPointerViaExtern() nounwind
  %val1 = load i32* %ptr, align 4
  %val2 = load i32* %ptrExtern, align 4
  %val3 = load i32* %ptrViaExtern, align 4
  %sum_partial = add i32 %val1, %val2
  %sum = add i32 %sum_partial, %val3
  ret i32 %sum
}

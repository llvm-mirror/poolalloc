;RUN: dsaopt %s -dsa-eq -disable-output
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define void @test() nounwind {
entry:
  %ptr = alloca i32
  %ptr2 = alloca i32
  ; Handle insertvalue on constantstruct
  %struct = insertvalue {i64, i32*} { i64 10, i32* null }, i32 *%ptr, 1
  %struct2 = insertvalue {i64, i32*} %struct, i32 *%ptr2, 1
  ; Load the pointer (unification says may be ptr or ptr2),
  ; and verify we tracked it properly:
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=test:ptr,test:ptr2,test:both
  %both = extractvalue {i64, i32*} %struct, 1
  ; Ensure handle extractvalue on constantstruct also
  %val = extractvalue {i64, i64} { i64 10, i64 5 }, 1

  ret void
}

; While we're at it, verify we don't die on similar constructs
; when accessing (constant) arrays/vectors.
define void @ArrayAndVector() nounwind {
entry:
  %0 = extractvalue [2 x i64] [i64 10, i64 20], 1
  %1 = insertvalue [2 x i64] [i64 10, i64 20], i64 %0, 1
  %2 = extractelement <2 x i64> <i64 30, i64 40>, i32 1
  %3 = insertelement <2 x i64> <i64 30, i64 40>, i64 %2, i32 1
  ret void
}

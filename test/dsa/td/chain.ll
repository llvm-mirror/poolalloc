; Verify callgraph is as expected for all DSA passes
;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=A,B
;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=B,C
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=A,B
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=B,C
;RUN: dsaopt %s -dsa-td -analyze -check-callees=A,B
;RUN: dsaopt %s -dsa-td -analyze -check-callees=B,C

; Verify the heap flag is also set appropriately where it should be
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "C:memC-H"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "B:memB-H"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "A:memA-H"

;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "C:memC+H"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "B:memB-H"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "A:memA-H"

;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "C:memC+H"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "B:memB+H"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "A:memA+H"

;RUN: dsaopt %s -dsa-td -analyze -verify-flags "C:memC+H"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "B:memB+H"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "A:memA+H"

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

declare noalias i8* @malloc(i64) nounwind

; Malloc wrapper
define i8* @C(i64 %size) {
entry:
  %memC = call noalias i8* @malloc(i64 %size) nounwind
  store i8 5, i8* %memC, align 8 ; Store so this isn't detected as an allocator
  ret i8* %memC
}

; Chain along to malloc wrapper
define i8* @B() {
entry:
  %memB = call i8* @C(i64 10)
  ret i8* %memB
}

define void @A() {
entry:
   %memA = call i8* @B()
   ;call void free(i8* %memA)
   ; (leak)
   ret void
}

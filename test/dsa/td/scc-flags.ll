; This tests some basic TD functionality--but "obscures" it a tad
; with a callgraph that has DSCallGraph finding SCC's that are
; not discovered during BU.
; When this happens, TD misses all kinds of call edges.
; Same underlying issue as PR12744.

; Clearly 'ptr' in main should have the 'stack allocated' flag :)
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "main:ptr+S"

; When TD runs, if we're inlining properly, we'll propagate the +S from
; main down to B.  Check this.
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "main:ptr+S"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "A:ptr+S"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "B:ptr+S"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main() {
  %ptr = alloca i32
  call void @A(i32* %ptr)
  ret i32 0
}

define internal void @A(i32* %ptr) {
entry:
  call void @B(void (void (i32*)*, i32*)* @C, i32* %ptr)
  ret void
}

define internal void @B(void (void (i32*)*, i32*)* %FP, i32* %ptr) {
entry:
  call void %FP(void (i32*)* @A, i32* null)
  ret void
}

define internal void @C(void (i32*)* %FP, i32* %ptr) {
  call void %FP(i32* %ptr)
  ret void
}

; See scc-flags.ll for the reasoning behind this CFG pattern.
; This builds on it, but goes a step further to demonstrate:
; a)we lose track of nodes (Globals, at that!) not just flags
; b)there's another bug lurking that prevents inlining through
;   the dummy callsites constructed in TD.  This is tested
;   by checking that information is indeed propagated from B
;   to C through the callsite within B.

; Clearly 'ptr' in main should have the 'stack allocated' flag :)
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "main:ptr+S"

; When TD runs, if we're doing things properly, we'll propagate @Global from
; main down to the load in C.
; Additionally, at each stage @Global should point to a +S DSNode
; Check that this is so:
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "main:ptr+S"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "A:ptr+G"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "A:ptr:0+S"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "B:ptr+G"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "B:ptr:0+S"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "C:ptr+G"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "C:ptr:0+S"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "C:addr+S"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

@Global = internal global i32* null

define i32 @main() {
  %ptr = alloca i32
  store i32* %ptr, i32** @Global
  call void @A(i32** @Global)
  ret i32 0
}

define internal void @A(i32** %ptr) {
entry:
  call void @B(void (void (i32**)*, i32**)* @C, i32** %ptr)
  ret void
}

define internal void @B(void (void (i32**)*, i32**)* %FP, i32** %ptr) {
entry:
  call void %FP(void (i32**)* @A, i32** %ptr)
  ret void
}

define internal void @C(void (i32**)* %FP, i32** %ptr) {
  %addr = load i32** %ptr
  call void %FP(i32** null)
  ret void
}

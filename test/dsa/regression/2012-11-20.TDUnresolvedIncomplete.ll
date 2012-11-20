; PR14190
; Verify that TD keeps unresolved callsites and properly
; marks nodes reachable from them as incomplete.
; This is important as otherwise a DSA client (like DSAA) will
; erroneously conclude that retptr/ptr in @indirect cannot alias.

; Verify we do eventually discover direct/indirect/indirect2 call foo
; RUN: dsaopt %s -dsa-td -analyze -check-callees=direct,foo
; RUN: dsaopt %s -dsa-td -analyze -check-callees=indirect,foo
; RUN: dsaopt %s -dsa-td -analyze -check-callees=indirect2,foo
; Within each, by the end of TD, we should either know the effects of calling @foo
; or mark the related nodes incomplete.
; Check this:
; RUN: dsaopt %s -dsa-td -analyze -check-same-node=@direct:ptr,@direct:retptr
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@direct:ptr-I,@direct:retptr-I
; RUN: dsaopt %s -dsa-td -analyze -check-not-same-node=@indirect:ptr,@indirect:retptr
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@indirect:ptr+I,@indirect:retptr+I
; RUN: dsaopt %s -dsa-td -analyze -check-not-same-node=@indirect2:ptr,@indirect2:retptr
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@indirect2:ptr+I,@indirect2:retptr+I
; Check that EQTD also has this behavior.
; RUN: dsaopt %s -dsa-eqtd -analyze -check-same-node=@direct:ptr,@direct:retptr
; RUN: dsaopt %s -dsa-eqtd -analyze -verify-flags=@direct:ptr-I,@direct:retptr-I
; RUN: dsaopt %s -dsa-eqtd -analyze -check-not-same-node=@indirect:ptr,@indirect:retptr
; RUN: dsaopt %s -dsa-eqtd -analyze -verify-flags=@indirect:ptr+I,@indirect:retptr+I
; RUN: dsaopt %s -dsa-eqtd -analyze -check-not-same-node=@indirect2:ptr,@indirect2:retptr
; RUN: dsaopt %s -dsa-eqtd -analyze -verify-flags=@indirect2:ptr+I,@indirect2:retptr+I
; ModuleID = 'test-fp.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

define i8* @foo(i8* %ptr) nounwind {
  ret i8* %ptr
}

; Call foo with ptr
define void @direct(i8* %ptr) {
  %retptr = call i8* @foo(i8* %ptr)
  %val = load i8* %retptr
  ret void
}

; Same, but using indirect fp not resolved until inlined into main
define void @indirect(i8* (i8*)* %fp, i8* %ptr) {
  %retptr = call i8* %fp(i8* %ptr)
  %val = load i8* %retptr
  ret void
}

; Same, but using indirect fp not resolved until inlined into main
; with an additional level of indirection
define void @indirect2(i8* (i8*)** %fpp, i8* %ptr) {
  %fp = load i8* (i8*)** %fpp
  %retptr = call i8* %fp(i8* %ptr)
  %val = load i8* %retptr
  ret void
}

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
  ; Conjure some i8*
  %ptr = load i8** %argv, align 8

  call void @direct(i8* %ptr)

  call void @indirect(i8* (i8*)* @foo, i8* %ptr)

  %fpp = alloca i8* (i8*)*, align 8
  store i8* (i8*)* @foo, i8* (i8*)** %fpp, align 8
  call void @indirect2(i8* (i8*)** %fpp, i8* %ptr)

  ret i32 0
}

; Here we have an externally visible function 'externallyVisible', such that it's possible
; for some outside code to use it to change the value of 'globalptr'.  As such, the pointer loaded
; from it in 'usesGlobalPtr' should be marked external.
; ModuleID = 'extern_global_escape.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
; RUN: dsaopt %s -dsa-local -analyze
; RUN: dsaopt %s -dsa-bu -analyze
; RUN: dsaopt %s -dsa-td -analyze

; This global itself isn't externally accessible, only via 'externallyVisible'
; RUN: dsaopt %s -dsa-local -analyze -verify-flags "globalptr+G-E"
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags "globalptr+G-IE"
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags "globalptr:0-E"
; RUN: dsaopt %s -dsa-td -analyze -verify-flags "globalptr+G-IE"
; RUN: dsaopt %s -dsa-td -analyze -verify-flags "globalptr:0+E"
@globalptr = internal global i32* null                     ; <i32**> [#uses=2]

; RUN: dsaopt %s -dsa-local -analyze -verify-flags "externallyVisible:ptr+I"
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags "externallyVisible:ptr+I-E"
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags "externallyVisible:ptr:0-E"
; RUN: dsaopt %s -dsa-td -analyze -verify-flags "externallyVisible:ptr+E-I"
; RUN: dsaopt %s -dsa-td -analyze -verify-flags "externallyVisible:ptr+E-I"
define void @externallyVisible(i32** %ptr) nounwind {
entry:
  %0 = load i32** %ptr, align 8
  store i32* %0, i32** @globalptr, align 8
  ret void
}

; RUN: dsaopt %s -dsa-bu -analyze -verify-flags "usesGlobalPtr:ptr-IE"
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags "usesGlobalPtr:ptr:0-EI"
; RUN: dsaopt %s -dsa-td -analyze -verify-flags "usesGlobalPtr:ptr-E"
; RUN: dsaopt %s -dsa-td -analyze -verify-flags "usesGlobalPtr:ptr:0+E"
define void @usesGlobalPtr() nounwind {
entry:
  %ptr = alloca i32*                              ; <i32**> [#uses=1]
  %0 = load i32** @globalptr, align 8             ; <i32*> [#uses=1]
  store i32* %0, i32** %ptr, align 8
  ret void
}

define i32 @main() nounwind {
entry:
  %stack = alloca i32, align 4                    ; <i32*> [#uses=2]
  %stackptr = alloca i32*, align 4                ; <i32*> [#uses=2]
  store i32 1, i32* %stack, align 4
  store i32 * %stack, i32** %stackptr, align 4
  call void @externallyVisible(i32** %stackptr) nounwind ; <i32> [#uses=0]
  call void @usesGlobalPtr() nounwind
  ret i32 0
}

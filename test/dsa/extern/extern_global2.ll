; ModuleID = 'extern_global2.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
;RUN: dsaopt %s -dsa-local -analyze
;RUN: dsaopt %s -dsa-bu -analyze
;RUN: dsaopt %s -dsa-td -analyze

; Global stays non-external until TD!
; RUN: dsaopt %s -dsa-local -analyze -verify-flags "G+G-E"
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags "G+G-E"
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags "G:0-E"
; RUN: dsaopt %s -dsa-td -analyze -verify-flags "G+G-E"
; RUN: dsaopt %s -dsa-td -analyze -verify-flags "G:0+E"
@G = internal global i32* null                    ; <i32**> [#uses=2]

; This function is externally callable so we mark it's arguments
; external when processed in TD.
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "C:a+I-E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "C:a+I-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "C:a-I+E"
define internal void @C(i32** %a) nounwind {
entry:
  %a_addr = alloca i32**                          ; <i32***> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32** %a, i32*** %a_addr
  %0 = load i32** @G, align 8                     ; <i32*> [#uses=1]
  %1 = load i32*** %a_addr, align 8               ; <i32**> [#uses=1]
  store i32* %0, i32** %1, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

; RUN: dsaopt %s -dsa-local -analyze -verify-flags "B:ptr:0-E"
; RUN: dsaopt %s -dsa-local -analyze -verify-flags "B:a-E"
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags "B:ptr:0-E"
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags "B:a-E"
; RUN: dsaopt %s -dsa-td -analyze -verify-flags "B:ptr:0+E"
; RUN: dsaopt %s -dsa-td -analyze -verify-flags "B:a+E"
define internal void @B() nounwind {
entry:
  %a = alloca i32                                 ; <i32*> [#uses=1]
  %ptr = alloca i32*                              ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* %a, i32** %ptr, align 8
  call void @C(i32** %ptr) nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @B2() nounwind {
entry:
  %a = alloca i32*                                ; <i32**> [#uses=2]
  %val = alloca i32                               ; <i32*> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = load i32** @G, align 8                     ; <i32*> [#uses=1]
  store i32* %0, i32** %a, align 8
  %1 = load i32** %a, align 8                     ; <i32*> [#uses=1]
  %2 = load i32* %1, align 4                      ; <i32> [#uses=1]
  store i32 %2, i32* %val, align 4
  br label %return

return:                                           ; preds = %entry
  ret void
}

; 'f' is passed to an external function, should always be external.
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "externalize:f+E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "externalize:f+E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "externalize:f+E"
define internal void @externalize(void (i32**)* %f) nounwind {
entry:
  %f_addr = alloca void (i32**)*                  ; <void (i32**)**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store void (i32**)* %f, void (i32**)** %f_addr
  %0 = load void (i32**)** %f_addr, align 8       ; <void (i32**)*> [#uses=1]
  call void @external(void (i32**)* %0) nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

declare void @external(void (i32**)*)

; 'f' shouldn't be external in local, nothing to cause that yet.
; However in BU and TD f should point to something external, but not be
; external itself.
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "A:f-E"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "A:f:0-E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "A:f-E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "A:f:0+E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "A:f-E"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "A:f:0+E"
define internal void @A() nounwind {
entry:
  %f = alloca void (i32**)*                       ; <void (i32**)**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store void (i32**)* @C, void (i32**)** %f, align 8
  %0 = load void (i32**)** %f, align 8            ; <void (i32**)*> [#uses=1]
  call void @externalize(void (i32**)* %0) nounwind
  call void @B() nounwind
  call void @B2() nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

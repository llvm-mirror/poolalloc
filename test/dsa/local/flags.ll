; ModuleID = 'flags.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

;Test flag assignment on globals/stack/heap, as well as mod/ref.
;a's are none, b's are mod, c'd as ref, d's are mod/ref.

;--Stack:
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:stack_a+S-MR"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:stack_b+SM-R"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:stack_c+S-M+R"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:stack_d+SMR"

;--Heap:
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:heap_a:0+IE"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:heap_b:0+IE"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:heap_c:0+IE"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "func:heap_d:0+IE"

;--Heap:
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "func:heap_a:0+HM-R"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "func:heap_b:0+HM-R"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "func:heap_c:0+HMR"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "func:heap_d:0+HMR"

;--Globals:
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "global_a+G-MR"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "global_b+GM-R"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "global_c+G-M+R"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "global_d+GMR"


@global_c = common global i32 0                   ; <i32*> [#uses=1]
@global_b = common global i32 0                   ; <i32*> [#uses=1]
@global_d = common global i32 0                   ; <i32*> [#uses=2]
@global_a = common global i32 0                   ; <i32*> [#uses=0]

define void @func() nounwind {
entry:
  %stack_a = alloca i32                           ; <i32*> [#uses=0]
  %heap_a = alloca i32*                           ; <i32**> [#uses=2]
  %stack_b = alloca i32                           ; <i32*> [#uses=1]
  %heap_b = alloca i32*                           ; <i32**> [#uses=3]
  %stack_c = alloca i32                           ; <i32*> [#uses=1]
  %heap_c = alloca i32*                           ; <i32**> [#uses=3]
  %stack_d = alloca i32                           ; <i32*> [#uses=2]
  %heap_d = alloca i32*                           ; <i32**> [#uses=4]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  store i32* %1, i32** %heap_a, align 8
  %2 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %3 = bitcast i8* %2 to i32*                     ; <i32*> [#uses=1]
  store i32* %3, i32** %heap_b, align 8
  %4 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %5 = bitcast i8* %4 to i32*                     ; <i32*> [#uses=1]
  store i32* %5, i32** %heap_c, align 8
  %6 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %7 = bitcast i8* %6 to i32*                     ; <i32*> [#uses=1]
  store i32* %7, i32** %heap_d, align 8
  %8 = load i32* %stack_c, align 4                ; <i32> [#uses=1]
  store i32 %8, i32* %stack_b, align 4
  %9 = load i32** %heap_c, align 8                ; <i32*> [#uses=1]
  %10 = load i32* %9, align 4                     ; <i32> [#uses=1]
  %11 = load i32** %heap_b, align 8               ; <i32*> [#uses=1]
  store i32 %10, i32* %11, align 4
  %12 = load i32* @global_c, align 4              ; <i32> [#uses=1]
  store i32 %12, i32* @global_b, align 4
  %13 = load i32* @global_d, align 4              ; <i32> [#uses=1]
  store i32 %13, i32* %stack_d, align 4
  %14 = load i32** %heap_d, align 8               ; <i32*> [#uses=1]
  %15 = load i32* %14, align 4                    ; <i32> [#uses=1]
  store i32 %15, i32* @global_d, align 4
  %16 = load i32** %heap_d, align 8               ; <i32*> [#uses=1]
  %17 = load i32* %stack_d, align 4               ; <i32> [#uses=1]
  store i32 %17, i32* %16, align 4
  %18 = load i32** %heap_a, align 8               ; <i32*> [#uses=1]
  %19 = bitcast i32* %18 to i8*                   ; <i8*> [#uses=1]
  call void @free(i8* %19) nounwind
  %20 = load i32** %heap_b, align 8               ; <i32*> [#uses=1]
  %21 = bitcast i32* %20 to i8*                   ; <i8*> [#uses=1]
  call void @free(i8* %21) nounwind
  %22 = load i32** %heap_c, align 8               ; <i32*> [#uses=1]
  %23 = bitcast i32* %22 to i8*                   ; <i8*> [#uses=1]
  call void @free(i8* %23) nounwind
  %24 = load i32** %heap_d, align 8               ; <i32*> [#uses=1]
  %25 = bitcast i32* %24 to i8*                   ; <i8*> [#uses=1]
  call void @free(i8* %25) nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

declare noalias i8* @malloc(i64) nounwind

declare void @free(i8*) nounwind

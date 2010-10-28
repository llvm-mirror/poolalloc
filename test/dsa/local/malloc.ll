;--check that local detects call to malloc properly (marks them heap)
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "main:b:0+H"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "main:b:0+IE"
;--check that local has b pointing to node containing c and d
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=main:b:0,main:c,main:d
;--check that td/bu don't mark such nodes as incomplete
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "main:c-I"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "main:c-I"

; ModuleID = 'malloc_free.ll'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %a = alloca i32                                 ; <i32*> [#uses=2]
  %b = alloca i8*                                 ; <i8**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %c = call noalias i8* @malloc(i64 100) nounwind ; <i8*> [#uses=1]
  store i8* %c, i8** %b, align 8
  %d = load i8** %b, align 8                      ; <i8*> [#uses=1]
  call void @free(i8* %d) nounwind
  store i32 0, i32* %a, align 4
  %e = load i32* %a, align 4                      ; <i32> [#uses=1]
  store i32 %e, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

declare noalias i8* @malloc(i64) nounwind

declare void @free(i8*) nounwind

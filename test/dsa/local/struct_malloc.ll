;RUN: dsaopt %s -dsa-local -analyze -check-type=func:struct,0:i32Array
;XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

declare noalias i8* @malloc(i64) nounwind

define void @func() nounwind {
entry:
  ; Allocate 2 x i32 = 8 bytes
  %struct = call noalias i8* @malloc(i64 8) nounwind

  ; Index into first, store 0 there
  %ptr = getelementptr inbounds i8* %struct, i64 0
  %conv = bitcast i8* %ptr to i32*
  store i32 0, i32* %conv, align 4

  ; Index into second, store 0 there also
  %ptr2 = getelementptr inbounds i8* %struct, i64 4
  %conv2 = bitcast i8* %ptr2 to i32*
  store i32 0, i32* %conv2, align 4
  ret void
}

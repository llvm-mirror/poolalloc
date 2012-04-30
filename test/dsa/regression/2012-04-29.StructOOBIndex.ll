; OOB indexing example reduced from 483.xalancbmk
;RUN: dsaopt %s -dsa-local -disable-output
;RUN: dsaopt %s -dsa-bu -disable-output
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%structType = type { i32, i8*, i32 }

; Index OOB in a single function
define i32 @foo(%structType* %t) {
  ; Treat 't' as an array of structs, and index to the 'i8*' in the second one
  %ptr = getelementptr inbounds %structType* %t, i64 1, i32 1
  ; Cast so indexing past end of struct is 'allowed'
  %cast = bitcast i8** %ptr to %structType*
  ; Get pointer to second 'i32' that's now OOB of the original struct type
  %ptr2 = getelementptr inbounds %structType* %cast, i32 0, i32 2
  ret i32 0
}

; Same thing, only split across two functions
define i32 @fooStart(%structType* %t) {
  ; Treat 't' as an array of structs, and index to the 'i8*' in the second one
  %ptr = getelementptr inbounds %structType* %t, i64 1, i32 1
  ; Cast so indexing past end of struct is 'allowed'
  %cast = bitcast i8** %ptr to %structType*
  ; Call other function to finish
  %val = tail call i32 @fooGEP(%structType* %cast)
  ret i32 %val
}

declare void @fooEmpty(i32* %val)

define i32 @fooGEP(%structType* %t) {
  ; Get pointer to second 'i32' that's now OOB of the original struct type
  %ptr = getelementptr inbounds %structType* %t, i32 0, i32 2
  ; Use in call, triggers similar bug in inlining calls
  call void @fooEmpty(i32* %ptr)
  ret i32 0
}


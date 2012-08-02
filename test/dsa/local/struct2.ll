; casting a struct to int

;RUN: dsaopt %s -dsa-local -analyze -check-type=main:r,0:i64\|i32*::8:i32*
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=main:r:0,main:x
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "main:r+SUP2"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "main:r1+SUP2"

; ModuleID = 'struct2.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.R = type { i32*, i32* }

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %d = alloca i64                                 ; <i64*> [#uses=1]
  %r = alloca %struct.R                           ; <%struct.R*> [#uses=3]
  %x = alloca i32                                 ; <i32*> [#uses=1]
  %y = alloca i32                                 ; <i32*> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = getelementptr inbounds %struct.R* %r, i32 0, i32 0 ; <i32**> [#uses=1]
  store i32* %x, i32** %1, align 8
  %2 = getelementptr inbounds %struct.R* %r, i32 0, i32 1 ; <i32**> [#uses=1]
  store i32* %y, i32** %2, align 8
  %r1 = bitcast %struct.R* %r to i64*             ; <i64*> [#uses=1]
  %3 = load i64* %r1, align 8                     ; <i64> [#uses=1]
  store i64 %3, i64* %d, align 8
  store i32 0, i32* %0, align 4
  %4 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %4, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval3 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval3
}

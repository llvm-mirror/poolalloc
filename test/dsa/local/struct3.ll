; example with casts between structs

;RUN: dsaopt %s -dsa-local -analyze -check-type=main:r,0:i32*::8:i32*::16:i8*
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:s,0:i32*::8:i32*
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:p,0:%\struct.T*
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=main:r,main:p:0
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=main:r:8,main:y,main:s:8
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=main:r:0,main:x,main:s:0
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=main:r:16,main:c
; ModuleID = 'struct3.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.R = type { i32*, i32*, i8* }
%struct.S = type { i32*, i32*, i32* }
%struct.T = type { i32*, i32* }

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %r = alloca %struct.R                           ; <%struct.R*> [#uses=4]
  %s = alloca %struct.S                           ; <%struct.S*> [#uses=1]
  %p = alloca %struct.T*                          ; <%struct.T**> [#uses=2]
  %x = alloca i32                                 ; <i32*> [#uses=1]
  %y = alloca i32                                 ; <i32*> [#uses=1]
  %c = alloca i8                                  ; <i8*> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = getelementptr inbounds %struct.R* %r, i32 0, i32 0 ; <i32**> [#uses=1]
  store i32* %x, i32** %1, align 8
  %2 = getelementptr inbounds %struct.R* %r, i32 0, i32 1 ; <i32**> [#uses=1]
  store i32* %y, i32** %2, align 8
  %3 = getelementptr inbounds %struct.R* %r, i32 0, i32 2 ; <i8**> [#uses=1]
  store i8* %c, i8** %3, align 8
  %r1 = bitcast %struct.R* %r to %struct.T*       ; <%struct.T*> [#uses=1]
  store %struct.T* %r1, %struct.T** %p, align 8
  %s2 = bitcast %struct.S* %s to %struct.T*       ; <%struct.T*> [#uses=2]
  %4 = load %struct.T** %p, align 8               ; <%struct.T*> [#uses=2]
  %5 = getelementptr inbounds %struct.T* %4, i32 0, i32 0 ; <i32**> [#uses=1]
  %6 = getelementptr inbounds %struct.T* %s2, i32 0, i32 0 ; <i32**> [#uses=1]
  %7 = load i32** %6, align 8                     ; <i32*> [#uses=1]
  store i32* %7, i32** %5, align 8
  %8 = getelementptr inbounds %struct.T* %4, i32 0, i32 1 ; <i32**> [#uses=1]
  %9 = getelementptr inbounds %struct.T* %s2, i32 0, i32 1 ; <i32**> [#uses=1]
  %10 = load i32** %9, align 8                    ; <i32*> [#uses=1]
  store i32* %10, i32** %8, align 8
  store i32 0, i32* %0, align 4
  %11 = load i32* %0, align 4                     ; <i32> [#uses=1]
  store i32 %11, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval4 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval4
}

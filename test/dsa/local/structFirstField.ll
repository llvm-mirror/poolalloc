; can use a struct pointer to access the first field of the pointer


;RUN: dsaopt %s -dsa-local -analyze -check-type=main:p,0:%\struct.S*
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:s,0:i32*
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:r,0:i32*
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:q,0:i32*
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=main:p:0,main:s
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=main:s:0,main:x,main:r:0,main:q:0

; ModuleID = 'structFirstField.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.S = type { i32* }

define void @main() nounwind {
entry:
  %0 = alloca %struct.S                           ; <%struct.S*> [#uses=2]
  %x = alloca i32                                 ; <i32*> [#uses=1]
  %q = alloca i32*                                ; <i32**> [#uses=2]
  %r = alloca i32*                                ; <i32**> [#uses=1]
  %s = alloca %struct.S                           ; <%struct.S*> [#uses=2]
  %p = alloca %struct.S*                          ; <%struct.S**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.S* %s, %struct.S** %p, align 8
  store i32* %x, i32** %q, align 8
  %q1 = bitcast i32** %q to %struct.S*            ; <%struct.S*> [#uses=1]
  %1 = getelementptr inbounds %struct.S* %0, i32 0, i32 0 ; <i32**> [#uses=1]
  %2 = getelementptr inbounds %struct.S* %q1, i32 0, i32 0 ; <i32**> [#uses=1]
  %3 = load i32** %2, align 8                     ; <i32*> [#uses=1]
  store i32* %3, i32** %1, align 8
  %4 = load %struct.S** %p, align 8               ; <%struct.S*> [#uses=1]
  %5 = getelementptr inbounds %struct.S* %4, i32 0, i32 0 ; <i32**> [#uses=1]
  %6 = getelementptr inbounds %struct.S* %0, i32 0, i32 0 ; <i32**> [#uses=1]
  %7 = load i32** %6, align 8                     ; <i32*> [#uses=1]
  store i32* %7, i32** %5, align 8
  %8 = getelementptr inbounds %struct.S* %s, i32 0, i32 0 ; <i32**> [#uses=1]
  %9 = load i32** %8, align 8                     ; <i32*> [#uses=1]
  store i32* %9, i32** %r, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

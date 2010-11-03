;accessing 1st field using struct pointer

;RUN: dsaopt %s -dsa-local -analyze -check-type=main:r,0:i32*::8:i32*::16:i8*

; ModuleID = 'struct4.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.R = type { i32*, i32*, i8* }

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %r = alloca %struct.R                           ; <%struct.R*> [#uses=4]
  %x = alloca i32                                 ; <i32*> [#uses=2]
  %y = alloca i32                                 ; <i32*> [#uses=1]
  %c = alloca i8                                  ; <i8*> [#uses=1]
  %p = alloca i32**                               ; <i32***> [#uses=2]
  %d = alloca i32                                 ; <i32*> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 5, i32* %x, align 4
  %1 = getelementptr inbounds %struct.R* %r, i32 0, i32 0 ; <i32**> [#uses=1]
  store i32* %x, i32** %1, align 8
  %2 = getelementptr inbounds %struct.R* %r, i32 0, i32 1 ; <i32**> [#uses=1]
  store i32* %y, i32** %2, align 8
  %3 = getelementptr inbounds %struct.R* %r, i32 0, i32 2 ; <i8**> [#uses=1]
  store i8* %c, i8** %3, align 8
  %r1 = bitcast %struct.R* %r to i32**            ; <i32**> [#uses=1]
  store i32** %r1, i32*** %p, align 8
  %4 = load i32*** %p, align 8                    ; <i32**> [#uses=1]
  %5 = load i32** %4, align 8                     ; <i32*> [#uses=1]
  %6 = load i32* %5, align 4                      ; <i32> [#uses=1]
  store i32 %6, i32* %d, align 4
  store i32 0, i32* %0, align 4
  %7 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %7, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval2 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval2
}

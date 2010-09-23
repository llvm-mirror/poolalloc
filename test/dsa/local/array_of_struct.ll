; An example with an array of structs. The node for the array must 
; be folded modulo the size of the struct element

;RUN: dsaopt %s -dsa-local -analyze -check-type=test2:arr,0:i32::4:i32Array

; ModuleID = 'test2.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.sType = type { i32, i32 }

define void @test2() nounwind {
entry:
  %arr = alloca [10 x %struct.sType]              ; <[10 x %struct.sType]*> [#uses=2]
  %z = alloca i32                                 ; <i32*> [#uses=2]
  %t = alloca i32                                 ; <i32*> [#uses=1]
  %t1 = alloca i32                                ; <i32*> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = load i32* %z, align 4                      ; <i32> [#uses=1]
  %1 = sext i32 %0 to i64                         ; <i64> [#uses=1]
  %2 = getelementptr inbounds [10 x %struct.sType]* %arr, i64 0, i64 %1 ; <%struct.sType*> [#uses=1]
  %3 = getelementptr inbounds %struct.sType* %2, i32 0, i32 1 ; <i32*> [#uses=1]
  %4 = load i32* %3, align 4                      ; <i32> [#uses=1]
  store i32 %4, i32* %t, align 4
  %5 = load i32* %z, align 4                      ; <i32> [#uses=1]
  %6 = sext i32 %5 to i64                         ; <i64> [#uses=1]
  %7 = getelementptr inbounds [10 x %struct.sType]* %arr, i64 0, i64 %6 ; <%struct.sType*> [#uses=1]
  %8 = getelementptr inbounds %struct.sType* %7, i32 0, i32 0 ; <i32*> [#uses=1]
  %9 = load i32* %8, align 4                      ; <i32> [#uses=1]
  store i32 %9, i32* %t1, align 4
  br label %return

return:                                           ; preds = %entry
  ret void
}

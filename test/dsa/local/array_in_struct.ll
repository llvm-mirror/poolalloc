; An example with a struct with an array. The node for the struct must be folded
; if we access the array. 

;RUN: dsaopt %s -dsa-local -analyze -check-type=test1:arr,FoldedVOID

; ModuleID = 'test1.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.sType = type { i32, [10 x i32], i32 }

@.str = private constant [4 x i8] c"%d\0A\00", align 1 ; <[4 x i8]*> [#uses=1]

define void @test1() nounwind {
entry:
  %arr = alloca %struct.sType                     ; <%struct.sType*> [#uses=4]
  %z = alloca i32                                 ; <i32*> [#uses=2]
  %t = alloca i32                                 ; <i32*> [#uses=1]
  %r = alloca i32                                 ; <i32*> [#uses=1]
  %r1 = alloca i32                                ; <i32*> [#uses=1]
  %r2 = alloca float                              ; <float*> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call i32 (i8*, ...)* @scanf(i8* noalias getelementptr inbounds ([4 x i8]* @.str, i64 0, i64 0), i32* %z) nounwind ; <i32> [#uses=0]
  %1 = load i32* %z, align 4                      ; <i32> [#uses=1]
  %2 = getelementptr inbounds %struct.sType* %arr, i32 0, i32 1 ; <[10 x i32]*> [#uses=1]
  %3 = sext i32 %1 to i64                         ; <i64> [#uses=1]
  %4 = getelementptr inbounds [10 x i32]* %2, i64 0, i64 %3 ; <i32*> [#uses=1]
  %5 = load i32* %4, align 4                      ; <i32> [#uses=1]
  store i32 %5, i32* %t, align 4
  %6 = getelementptr inbounds %struct.sType* %arr, i32 0, i32 2 ; <i32*> [#uses=1]
  %7 = load i32* %6, align 4                      ; <i32> [#uses=1]
  store i32 %7, i32* %r, align 4
  %8 = getelementptr inbounds %struct.sType* %arr, i32 0, i32 1 ; <[10 x i32]*> [#uses=1]
  %9 = getelementptr inbounds [10 x i32]* %8, i64 0, i64 5 ; <i32*> [#uses=1]
  %10 = load i32* %9, align 4                     ; <i32> [#uses=1]
  store i32 %10, i32* %r1, align 4
  %11 = getelementptr inbounds %struct.sType* %arr, i32 0, i32 0 ; <i32*> [#uses=1]
  %12 = load i32* %11, align 4                    ; <i32> [#uses=1]
  %13 = sitofp i32 %12 to float                   ; <float> [#uses=1]
  store float %13, float* %r2, align 4
  br label %return

return:                                           ; preds = %entry
  ret void
}

declare i32 @scanf(i8* noalias, ...) nounwind

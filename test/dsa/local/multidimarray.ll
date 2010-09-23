; An example with multidimensional array. It is a flattened array
; Gets the same array tag.

;RUN: dsaopt %s -dsa-local -analyze -check-type=func:arr,0:i32::8:%\struct.StructType*Array

; ModuleID = 'multidimarray.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.StructType = type { i32, %struct.StructType* }

define void @func() nounwind {
entry:
  %arr = alloca [10 x [20 x %struct.StructType]]  ; <[10 x [20 x %struct.StructType]]*> [#uses=4]
  %i = alloca i32                                 ; <i32*> [#uses=11]
  %s2 = alloca %struct.StructType                 ; <%struct.StructType*> [#uses=3]
  %c = alloca %struct.StructType*                 ; <%struct.StructType**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 0, i32* %i, align 4
  br label %bb1

bb:                                               ; preds = %bb1
  %0 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %1 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %2 = sext i32 %0 to i64                         ; <i64> [#uses=1]
  %3 = getelementptr inbounds [10 x [20 x %struct.StructType]]* %arr, i64 0, i64 %2 ; <[20 x %struct.StructType]*> [#uses=1]
  %4 = sext i32 %1 to i64                         ; <i64> [#uses=1]
  %5 = getelementptr inbounds [20 x %struct.StructType]* %3, i64 0, i64 %4 ; <%struct.StructType*> [#uses=1]
  %6 = getelementptr inbounds %struct.StructType* %5, i32 0, i32 0 ; <i32*> [#uses=1]
  %7 = load i32* %i, align 4                      ; <i32> [#uses=1]
  store i32 %7, i32* %6, align 8
  %8 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %9 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %10 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %11 = sub nsw i32 %10, 1                        ; <i32> [#uses=1]
  %12 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %13 = sub nsw i32 %12, 1                        ; <i32> [#uses=1]
  %14 = sext i32 %11 to i64                       ; <i64> [#uses=1]
  %15 = getelementptr inbounds [10 x [20 x %struct.StructType]]* %arr, i64 0, i64 %14 ; <[20 x %struct.StructType]*> [#uses=1]
  %16 = sext i32 %13 to i64                       ; <i64> [#uses=1]
  %17 = getelementptr inbounds [20 x %struct.StructType]* %15, i64 0, i64 %16 ; <%struct.StructType*> [#uses=1]
  %18 = sext i32 %8 to i64                        ; <i64> [#uses=1]
  %19 = getelementptr inbounds [10 x [20 x %struct.StructType]]* %arr, i64 0, i64 %18 ; <[20 x %struct.StructType]*> [#uses=1]
  %20 = sext i32 %9 to i64                        ; <i64> [#uses=1]
  %21 = getelementptr inbounds [20 x %struct.StructType]* %19, i64 0, i64 %20 ; <%struct.StructType*> [#uses=1]
  %22 = getelementptr inbounds %struct.StructType* %21, i32 0, i32 1 ; <%struct.StructType**> [#uses=1]
  store %struct.StructType* %17, %struct.StructType** %22, align 8
  %23 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %24 = add nsw i32 %23, 1                        ; <i32> [#uses=1]
  store i32 %24, i32* %i, align 4
  br label %bb1

bb1:                                              ; preds = %bb, %entry
  %25 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %26 = icmp sle i32 %25, 9                       ; <i1> [#uses=1]
  br i1 %26, label %bb, label %bb2

bb2:                                              ; preds = %bb1
  %27 = getelementptr inbounds [10 x [20 x %struct.StructType]]* %arr, i64 0, i64 1 ; <[20 x %struct.StructType]*> [#uses=1]
  %28 = getelementptr inbounds [20 x %struct.StructType]* %27, i64 0, i64 1 ; <%struct.StructType*> [#uses=2]
  %29 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 0 ; <i32*> [#uses=1]
  %30 = getelementptr inbounds %struct.StructType* %28, i32 0, i32 0 ; <i32*> [#uses=1]
  %31 = load i32* %30, align 8                    ; <i32> [#uses=1]
  store i32 %31, i32* %29, align 8
  %32 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 1 ; <%struct.StructType**> [#uses=1]
  %33 = getelementptr inbounds %struct.StructType* %28, i32 0, i32 1 ; <%struct.StructType**> [#uses=1]
  %34 = load %struct.StructType** %33, align 8    ; <%struct.StructType*> [#uses=1]
  store %struct.StructType* %34, %struct.StructType** %32, align 8
  %35 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 1 ; <%struct.StructType**> [#uses=1]
  %36 = load %struct.StructType** %35, align 8    ; <%struct.StructType*> [#uses=1]
  store %struct.StructType* %36, %struct.StructType** %c, align 8
  br label %return

return:                                           ; preds = %bb2
  ret void
}

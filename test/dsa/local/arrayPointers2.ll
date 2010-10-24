; 2D array as array of pointers. Array should not be folded.

;RUN: dsaopt %s -dsa-local -analyze -check-type=main:array:0,0:i32::4:i32::8:floatArray
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:array,0:%\struct.structType*Array


; ModuleID = 'arrayPointers2.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.structType = type { i32, i32, float }

define void @main() nounwind {
entry:
  %nrows = alloca i32                             ; <i32*> [#uses=3]
  %ncolumns = alloca i32                          ; <i32*> [#uses=3]
  %i = alloca i32                                 ; <i32*> [#uses=14]
  %array = alloca [10 x %struct.structType*]      ; <[10 x %struct.structType*]*> [#uses=4]
  %j = alloca i32                                 ; <i32*> [#uses=9]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 10, i32* %nrows, align 4
  store i32 10, i32* %ncolumns, align 4
  store i32 0, i32* %i, align 4
  br label %bb1

bb:                                               ; preds = %bb1
  %0 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %1 = load i32* %ncolumns, align 4               ; <i32> [#uses=1]
  %2 = sext i32 %1 to i64                         ; <i64> [#uses=1]
  %3 = mul i64 %2, 12                             ; <i64> [#uses=1]
  %4 = call noalias i8* @malloc(i64 %3) nounwind  ; <i8*> [#uses=1]
  %5 = bitcast i8* %4 to %struct.structType*      ; <%struct.structType*> [#uses=1]
  %6 = sext i32 %0 to i64                         ; <i64> [#uses=1]
  %7 = getelementptr inbounds [10 x %struct.structType*]* %array, i64 0, i64 %6 ; <%struct.structType**> [#uses=1]
  store %struct.structType* %5, %struct.structType** %7, align 8
  %8 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %9 = add nsw i32 %8, 1                          ; <i32> [#uses=1]
  store i32 %9, i32* %i, align 4
  br label %bb1

bb1:                                              ; preds = %bb, %entry
  %10 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %11 = load i32* %nrows, align 4                 ; <i32> [#uses=1]
  %12 = icmp slt i32 %10, %11                     ; <i1> [#uses=1]
  br i1 %12, label %bb, label %bb2

bb2:                                              ; preds = %bb1
  store i32 0, i32* %i, align 4
  br label %bb7

bb3:                                              ; preds = %bb7
  store i32 0, i32* %j, align 4
  br label %bb5

bb4:                                              ; preds = %bb5
  %13 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %14 = sext i32 %13 to i64                       ; <i64> [#uses=1]
  %15 = getelementptr inbounds [10 x %struct.structType*]* %array, i64 0, i64 %14 ; <%struct.structType**> [#uses=1]
  %16 = load %struct.structType** %15, align 8    ; <%struct.structType*> [#uses=1]
  %17 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %18 = sext i32 %17 to i64                       ; <i64> [#uses=1]
  %19 = getelementptr inbounds %struct.structType* %16, i64 %18 ; <%struct.structType*> [#uses=1]
  %20 = getelementptr inbounds %struct.structType* %19, i32 0, i32 0 ; <i32*> [#uses=1]
  %21 = load i32* %i, align 4                     ; <i32> [#uses=1]
  store i32 %21, i32* %20, align 4
  %22 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %23 = add nsw i32 %22, 1                        ; <i32> [#uses=1]
  store i32 %23, i32* %j, align 4
  br label %bb5

bb5:                                              ; preds = %bb4, %bb3
  %24 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %25 = load i32* %ncolumns, align 4              ; <i32> [#uses=1]
  %26 = icmp slt i32 %24, %25                     ; <i1> [#uses=1]
  br i1 %26, label %bb4, label %bb6

bb6:                                              ; preds = %bb5
  %27 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %28 = sext i32 %27 to i64                       ; <i64> [#uses=1]
  %29 = getelementptr inbounds [10 x %struct.structType*]* %array, i64 0, i64 %28 ; <%struct.structType**> [#uses=1]
  %30 = load %struct.structType** %29, align 8    ; <%struct.structType*> [#uses=1]
  %31 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %32 = sext i32 %31 to i64                       ; <i64> [#uses=1]
  %33 = getelementptr inbounds %struct.structType* %30, i64 %32 ; <%struct.structType*> [#uses=1]
  %34 = getelementptr inbounds %struct.structType* %33, i32 0, i32 1 ; <i32*> [#uses=1]
  %35 = load i32* %j, align 4                     ; <i32> [#uses=1]
  store i32 %35, i32* %34, align 4
  %36 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %37 = sext i32 %36 to i64                       ; <i64> [#uses=1]
  %38 = getelementptr inbounds [10 x %struct.structType*]* %array, i64 0, i64 %37 ; <%struct.structType**> [#uses=1]
  %39 = load %struct.structType** %38, align 8    ; <%struct.structType*> [#uses=1]
  %40 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %41 = sext i32 %40 to i64                       ; <i64> [#uses=1]
  %42 = getelementptr inbounds %struct.structType* %39, i64 %41 ; <%struct.structType*> [#uses=1]
  %43 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %44 = sitofp i32 %43 to double                  ; <double> [#uses=1]
  %45 = fdiv double %44, 2.000000e+00             ; <double> [#uses=1]
  %46 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %47 = sitofp i32 %46 to double                  ; <double> [#uses=1]
  %48 = fdiv double %47, 2.000000e+00             ; <double> [#uses=1]
  %49 = fadd double %45, %48                      ; <double> [#uses=1]
  %50 = fptrunc double %49 to float               ; <float> [#uses=1]
  %51 = getelementptr inbounds %struct.structType* %42, i32 0, i32 2 ; <float*> [#uses=1]
  store float %50, float* %51, align 4
  %52 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %53 = add nsw i32 %52, 1                        ; <i32> [#uses=1]
  store i32 %53, i32* %i, align 4
  br label %bb7

bb7:                                              ; preds = %bb6, %bb2
  %54 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %55 = load i32* %nrows, align 4                 ; <i32> [#uses=1]
  %56 = icmp slt i32 %54, %55                     ; <i1> [#uses=1]
  br i1 %56, label %bb3, label %bb8

bb8:                                              ; preds = %bb7
  br label %return

return:                                           ; preds = %bb8
  ret void
}

declare noalias i8* @malloc(i64) nounwind

; array of pointers to arrays of structs. The inner array gets folded.

;RUN: dsaopt %s -dsa-local -analyze -check-type=main:array,0:%\struct.structType*Array
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:array:0,main:row1,main:row2,0:i32::4:i32::8:floatArray
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=main:row1,main:row2,main:array:0

; ModuleID = 'arrayPointers3.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.structType = type { i32, i32, float }

define void @main() nounwind {
entry:
  %nrows = alloca i32                             ; <i32*> [#uses=2]
  %ncolumns = alloca i32                          ; <i32*> [#uses=2]
  %i = alloca i32                                 ; <i32*> [#uses=9]
  %array = alloca [10 x %struct.structType*]      ; <[10 x %struct.structType*]*> [#uses=5]
  %row1 = alloca [10 x %struct.structType]        ; <[10 x %struct.structType]*> [#uses=1]
  %row2 = alloca [10 x %struct.structType]        ; <[10 x %struct.structType]*> [#uses=1]
  %j = alloca i32                                 ; <i32*> [#uses=9]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 2, i32* %nrows, align 4
  store i32 10, i32* %ncolumns, align 4
  %0 = getelementptr inbounds [10 x %struct.structType*]* %array, i64 0, i64 0 ; <%struct.structType**> [#uses=1]
  %row11 = bitcast [10 x %struct.structType]* %row1 to %struct.structType* ; <%struct.structType*> [#uses=1]
  store %struct.structType* %row11, %struct.structType** %0, align 8
  %1 = getelementptr inbounds [10 x %struct.structType*]* %array, i64 0, i64 1 ; <%struct.structType**> [#uses=1]
  %row22 = bitcast [10 x %struct.structType]* %row2 to %struct.structType* ; <%struct.structType*> [#uses=1]
  store %struct.structType* %row22, %struct.structType** %1, align 8
  store i32 0, i32* %i, align 4
  br label %bb6

bb:                                               ; preds = %bb6
  store i32 0, i32* %j, align 4
  br label %bb4

bb3:                                              ; preds = %bb4
  %2 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %3 = sext i32 %2 to i64                         ; <i64> [#uses=1]
  %4 = getelementptr inbounds [10 x %struct.structType*]* %array, i64 0, i64 %3 ; <%struct.structType**> [#uses=1]
  %5 = load %struct.structType** %4, align 8      ; <%struct.structType*> [#uses=1]
  %6 = load i32* %j, align 4                      ; <i32> [#uses=1]
  %7 = sext i32 %6 to i64                         ; <i64> [#uses=1]
  %8 = getelementptr inbounds %struct.structType* %5, i64 %7 ; <%struct.structType*> [#uses=1]
  %9 = getelementptr inbounds %struct.structType* %8, i32 0, i32 0 ; <i32*> [#uses=1]
  %10 = load i32* %i, align 4                     ; <i32> [#uses=1]
  store i32 %10, i32* %9, align 4
  %11 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %12 = add nsw i32 %11, 1                        ; <i32> [#uses=1]
  store i32 %12, i32* %j, align 4
  br label %bb4

bb4:                                              ; preds = %bb3, %bb
  %13 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %14 = load i32* %ncolumns, align 4              ; <i32> [#uses=1]
  %15 = icmp slt i32 %13, %14                     ; <i1> [#uses=1]
  br i1 %15, label %bb3, label %bb5

bb5:                                              ; preds = %bb4
  %16 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %17 = sext i32 %16 to i64                       ; <i64> [#uses=1]
  %18 = getelementptr inbounds [10 x %struct.structType*]* %array, i64 0, i64 %17 ; <%struct.structType**> [#uses=1]
  %19 = load %struct.structType** %18, align 8    ; <%struct.structType*> [#uses=1]
  %20 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %21 = sext i32 %20 to i64                       ; <i64> [#uses=1]
  %22 = getelementptr inbounds %struct.structType* %19, i64 %21 ; <%struct.structType*> [#uses=1]
  %23 = getelementptr inbounds %struct.structType* %22, i32 0, i32 1 ; <i32*> [#uses=1]
  %24 = load i32* %j, align 4                     ; <i32> [#uses=1]
  store i32 %24, i32* %23, align 4
  %25 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %26 = sext i32 %25 to i64                       ; <i64> [#uses=1]
  %27 = getelementptr inbounds [10 x %struct.structType*]* %array, i64 0, i64 %26 ; <%struct.structType**> [#uses=1]
  %28 = load %struct.structType** %27, align 8    ; <%struct.structType*> [#uses=1]
  %29 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %30 = sext i32 %29 to i64                       ; <i64> [#uses=1]
  %31 = getelementptr inbounds %struct.structType* %28, i64 %30 ; <%struct.structType*> [#uses=1]
  %32 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %33 = sitofp i32 %32 to double                  ; <double> [#uses=1]
  %34 = fdiv double %33, 2.000000e+00             ; <double> [#uses=1]
  %35 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %36 = sitofp i32 %35 to double                  ; <double> [#uses=1]
  %37 = fdiv double %36, 2.000000e+00             ; <double> [#uses=1]
  %38 = fadd double %34, %37                      ; <double> [#uses=1]
  %39 = fptrunc double %38 to float               ; <float> [#uses=1]
  %40 = getelementptr inbounds %struct.structType* %31, i32 0, i32 2 ; <float*> [#uses=1]
  store float %39, float* %40, align 4
  %41 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %42 = add nsw i32 %41, 1                        ; <i32> [#uses=1]
  store i32 %42, i32* %i, align 4
  br label %bb6

bb6:                                              ; preds = %bb5, %entry
  %43 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %44 = load i32* %nrows, align 4                 ; <i32> [#uses=1]
  %45 = icmp slt i32 %43, %44                     ; <i1> [#uses=1]
  br i1 %45, label %bb, label %bb7

bb7:                                              ; preds = %bb6
  br label %return

return:                                           ; preds = %bb7
  ret void
}

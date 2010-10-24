; 2D array as array of pointers. Array should not be folded.

;RUN: dsaopt %s -dsa-local -analyze -check-type=main:array,0:i32*Array
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:array:0,0:i32Array

; ModuleID = 'arrayPointers1.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define void @main() nounwind {
entry:
  %nrows = alloca i32                             ; <i32*> [#uses=3]
  %ncolumns = alloca i32                          ; <i32*> [#uses=3]
  %i = alloca i32                                 ; <i32*> [#uses=11]
  %array = alloca [10 x i32*]                     ; <[10 x i32*]*> [#uses=2]
  %j = alloca i32                                 ; <i32*> [#uses=6]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 10, i32* %nrows, align 4
  store i32 10, i32* %ncolumns, align 4
  store i32 0, i32* %i, align 4
  br label %bb1

bb:                                               ; preds = %bb1
  %0 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %1 = load i32* %ncolumns, align 4               ; <i32> [#uses=1]
  %2 = sext i32 %1 to i64                         ; <i64> [#uses=1]
  %3 = mul i64 %2, 4                              ; <i64> [#uses=1]
  %4 = call noalias i8* @malloc(i64 %3) nounwind  ; <i8*> [#uses=1]
  %5 = bitcast i8* %4 to i32*                     ; <i32*> [#uses=1]
  %6 = sext i32 %0 to i64                         ; <i64> [#uses=1]
  %7 = getelementptr inbounds [10 x i32*]* %array, i64 0, i64 %6 ; <i32**> [#uses=1]
  store i32* %5, i32** %7, align 8
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
  %15 = getelementptr inbounds [10 x i32*]* %array, i64 0, i64 %14 ; <i32**> [#uses=1]
  %16 = load i32** %15, align 8                   ; <i32*> [#uses=1]
  %17 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %18 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %19 = add nsw i32 %17, %18                      ; <i32> [#uses=1]
  %20 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %21 = sext i32 %20 to i64                       ; <i64> [#uses=1]
  %22 = getelementptr inbounds i32* %16, i64 %21  ; <i32*> [#uses=1]
  store i32 %19, i32* %22, align 1
  %23 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %24 = add nsw i32 %23, 1                        ; <i32> [#uses=1]
  store i32 %24, i32* %j, align 4
  br label %bb5

bb5:                                              ; preds = %bb4, %bb3
  %25 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %26 = load i32* %ncolumns, align 4              ; <i32> [#uses=1]
  %27 = icmp slt i32 %25, %26                     ; <i1> [#uses=1]
  br i1 %27, label %bb4, label %bb6

bb6:                                              ; preds = %bb5
  %28 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %29 = add nsw i32 %28, 1                        ; <i32> [#uses=1]
  store i32 %29, i32* %i, align 4
  br label %bb7

bb7:                                              ; preds = %bb6, %bb2
  %30 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %31 = load i32* %nrows, align 4                 ; <i32> [#uses=1]
  %32 = icmp slt i32 %30, %31                     ; <i1> [#uses=1]
  br i1 %32, label %bb3, label %bb8

bb8:                                              ; preds = %bb7
  br label %return

return:                                           ; preds = %bb8
  ret void
}

declare noalias i8* @malloc(i64) nounwind

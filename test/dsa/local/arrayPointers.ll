;2D array using pointers. Should be folded

;RUN: dsaopt %s -dsa-local -analyze -check-type=main:array:0,0:i32*Array
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:array:0:0,0:i32Array
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:array,0:i32**

; ModuleID = 'arrayPointers.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define void @main() nounwind {
entry:
  %nrows = alloca i32                             ; <i32*> [#uses=4]
  %ncolumns = alloca i32                          ; <i32*> [#uses=3]
  %i = alloca i32                                 ; <i32*> [#uses=11]
  %array = alloca i32**                           ; <i32***> [#uses=3]
  %j = alloca i32                                 ; <i32*> [#uses=6]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 10, i32* %nrows, align 4
  store i32 10, i32* %ncolumns, align 4
  %0 = load i32* %nrows, align 4                  ; <i32> [#uses=1]
  %1 = sext i32 %0 to i64                         ; <i64> [#uses=1]
  %2 = mul i64 %1, 8                              ; <i64> [#uses=1]
  %3 = call noalias i8* @malloc(i64 %2) nounwind  ; <i8*> [#uses=1]
  %4 = bitcast i8* %3 to i32**                    ; <i32**> [#uses=1]
  store i32** %4, i32*** %array, align 8
  store i32 0, i32* %i, align 4
  br label %bb1

bb:                                               ; preds = %bb1
  %5 = load i32* %ncolumns, align 4               ; <i32> [#uses=1]
  %6 = sext i32 %5 to i64                         ; <i64> [#uses=1]
  %7 = mul i64 %6, 4                              ; <i64> [#uses=1]
  %8 = call noalias i8* @malloc(i64 %7) nounwind  ; <i8*> [#uses=1]
  %9 = bitcast i8* %8 to i32*                     ; <i32*> [#uses=1]
  %10 = load i32*** %array, align 8               ; <i32**> [#uses=1]
  %11 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %12 = sext i32 %11 to i64                       ; <i64> [#uses=1]
  %13 = getelementptr inbounds i32** %10, i64 %12 ; <i32**> [#uses=1]
  store i32* %9, i32** %13, align 1
  %14 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %15 = add nsw i32 %14, 1                        ; <i32> [#uses=1]
  store i32 %15, i32* %i, align 4
  br label %bb1

bb1:                                              ; preds = %bb, %entry
  %16 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %17 = load i32* %nrows, align 4                 ; <i32> [#uses=1]
  %18 = icmp slt i32 %16, %17                     ; <i1> [#uses=1]
  br i1 %18, label %bb, label %bb2

bb2:                                              ; preds = %bb1
  store i32 0, i32* %i, align 4
  br label %bb7

bb3:                                              ; preds = %bb7
  store i32 0, i32* %j, align 4
  br label %bb5

bb4:                                              ; preds = %bb5
  %19 = load i32*** %array, align 8               ; <i32**> [#uses=1]
  %20 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %21 = sext i32 %20 to i64                       ; <i64> [#uses=1]
  %22 = getelementptr inbounds i32** %19, i64 %21 ; <i32**> [#uses=1]
  %23 = load i32** %22, align 1                   ; <i32*> [#uses=1]
  %24 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %25 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %26 = add nsw i32 %24, %25                      ; <i32> [#uses=1]
  %27 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %28 = sext i32 %27 to i64                       ; <i64> [#uses=1]
  %29 = getelementptr inbounds i32* %23, i64 %28  ; <i32*> [#uses=1]
  store i32 %26, i32* %29, align 1
  %30 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %31 = add nsw i32 %30, 1                        ; <i32> [#uses=1]
  store i32 %31, i32* %j, align 4
  br label %bb5

bb5:                                              ; preds = %bb4, %bb3
  %32 = load i32* %j, align 4                     ; <i32> [#uses=1]
  %33 = load i32* %ncolumns, align 4              ; <i32> [#uses=1]
  %34 = icmp slt i32 %32, %33                     ; <i1> [#uses=1]
  br i1 %34, label %bb4, label %bb6

bb6:                                              ; preds = %bb5
  %35 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %36 = add nsw i32 %35, 1                        ; <i32> [#uses=1]
  store i32 %36, i32* %i, align 4
  br label %bb7

bb7:                                              ; preds = %bb6, %bb2
  %37 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %38 = load i32* %nrows, align 4                 ; <i32> [#uses=1]
  %39 = icmp slt i32 %37, %38                     ; <i1> [#uses=1]
  br i1 %39, label %bb3, label %bb8

bb8:                                              ; preds = %bb7
  br label %return

return:                                           ; preds = %bb8
  ret void
}

declare noalias i8* @malloc(i64) nounwind

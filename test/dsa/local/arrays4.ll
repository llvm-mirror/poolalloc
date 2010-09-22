
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:s2:8,func:c:0,func:tmp:8


; ModuleID = 'arrays4.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.StructType = type { i32, i32* }

define void @func() nounwind {
entry:
  %tmp = alloca [10 x %struct.StructType]         ; <[10 x %struct.StructType]*> [#uses=2]
  %i = alloca i32                                 ; <i32*> [#uses=6]
  %s2 = alloca %struct.StructType                 ; <%struct.StructType*> [#uses=3]
  %c = alloca i32*                                ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 0, i32* %i, align 4
  br label %bb1

bb:                                               ; preds = %bb1
  %0 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %1 = sext i32 %0 to i64                         ; <i64> [#uses=1]
  %2 = getelementptr inbounds [10 x %struct.StructType]* %tmp, i64 0, i64 %1 ; <%struct.StructType*> [#uses=1]
  %3 = getelementptr inbounds %struct.StructType* %2, i32 0, i32 0 ; <i32*> [#uses=1]
  %4 = load i32* %i, align 4                      ; <i32> [#uses=1]
  store i32 %4, i32* %3, align 8
  %5 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %6 = add nsw i32 %5, 1                          ; <i32> [#uses=1]
  store i32 %6, i32* %i, align 4
  br label %bb1

bb1:                                              ; preds = %bb, %entry
  %7 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %8 = icmp sle i32 %7, 9                         ; <i1> [#uses=1]
  br i1 %8, label %bb, label %bb2

bb2:                                              ; preds = %bb1
  %9 = getelementptr inbounds [10 x %struct.StructType]* %tmp, i64 0, i64 0 ; <%struct.StructType*> [#uses=2]
  %10 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 0 ; <i32*> [#uses=1]
  %11 = getelementptr inbounds %struct.StructType* %9, i32 0, i32 0 ; <i32*> [#uses=1]
  %12 = load i32* %11, align 8                    ; <i32> [#uses=1]
  store i32 %12, i32* %10, align 8
  %13 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 1 ; <i32**> [#uses=1]
  %14 = getelementptr inbounds %struct.StructType* %9, i32 0, i32 1 ; <i32**> [#uses=1]
  %15 = load i32** %14, align 8                   ; <i32*> [#uses=1]
  store i32* %15, i32** %13, align 8
  %16 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 1 ; <i32**> [#uses=1]
  %17 = load i32** %16, align 8                   ; <i32*> [#uses=1]
  store i32* %17, i32** %c, align 8
  br label %return

return:                                           ; preds = %bb2
  ret void
}

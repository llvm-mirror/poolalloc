;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:s2:8,func:c:0,func:tmp:0:8
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:ptr:0,func:ptr1:0,func:s2

; ModuleID = 'arrays3.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.StructType = type { i32, i32* }

define void @func() nounwind {
entry:
  %tmp = alloca %struct.StructType*               ; <%struct.StructType**> [#uses=4]
  %i = alloca i32                                 ; <i32*> [#uses=7]
  %s2 = alloca %struct.StructType                 ; <%struct.StructType*> [#uses=4]
  %c = alloca i32*                                ; <i32**> [#uses=1]
  %ptr = alloca %struct.StructType*               ; <%struct.StructType**> [#uses=2]
  %ptr1 = alloca %struct.StructType*              ; <%struct.StructType**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call noalias i8* @malloc(i64 160) nounwind ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to %struct.StructType*      ; <%struct.StructType*> [#uses=1]
  store %struct.StructType* %1, %struct.StructType** %tmp, align 8
  store i32 0, i32* %i, align 4
  br label %bb1

bb:                                               ; preds = %bb1
  %2 = load %struct.StructType** %tmp, align 8    ; <%struct.StructType*> [#uses=1]
  %3 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %4 = sext i32 %3 to i64                         ; <i64> [#uses=1]
  %5 = getelementptr inbounds %struct.StructType* %2, i64 %4 ; <%struct.StructType*> [#uses=1]
  %6 = getelementptr inbounds %struct.StructType* %5, i32 0, i32 0 ; <i32*> [#uses=1]
  %7 = load i32* %i, align 4                      ; <i32> [#uses=1]
  store i32 %7, i32* %6, align 8
  %8 = load %struct.StructType** %tmp, align 8    ; <%struct.StructType*> [#uses=1]
  %9 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %10 = sext i32 %9 to i64                        ; <i64> [#uses=1]
  %11 = getelementptr inbounds %struct.StructType* %8, i64 %10 ; <%struct.StructType*> [#uses=1]
  %12 = call noalias i8* @malloc(i64 4) nounwind  ; <i8*> [#uses=1]
  %13 = bitcast i8* %12 to i32*                   ; <i32*> [#uses=1]
  %14 = getelementptr inbounds %struct.StructType* %11, i32 0, i32 1 ; <i32**> [#uses=1]
  store i32* %13, i32** %14, align 8
  %15 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %16 = add nsw i32 %15, 1                        ; <i32> [#uses=1]
  store i32 %16, i32* %i, align 4
  br label %bb1

bb1:                                              ; preds = %bb, %entry
  %17 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %18 = icmp sle i32 %17, 9                       ; <i1> [#uses=1]
  br i1 %18, label %bb, label %bb2

bb2:                                              ; preds = %bb1
  %19 = load %struct.StructType** %tmp, align 8   ; <%struct.StructType*> [#uses=1]
  %20 = getelementptr inbounds %struct.StructType* %19, i64 1 ; <%struct.StructType*> [#uses=2]
  %21 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 0 ; <i32*> [#uses=1]
  %22 = getelementptr inbounds %struct.StructType* %20, i32 0, i32 0 ; <i32*> [#uses=1]
  %23 = load i32* %22, align 1                    ; <i32> [#uses=1]
  store i32 %23, i32* %21, align 1
  %24 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 1 ; <i32**> [#uses=1]
  %25 = getelementptr inbounds %struct.StructType* %20, i32 0, i32 1 ; <i32**> [#uses=1]
  %26 = load i32** %25, align 1                   ; <i32*> [#uses=1]
  store i32* %26, i32** %24, align 1
  %27 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 1 ; <i32**> [#uses=1]
  %28 = load i32** %27, align 8                   ; <i32*> [#uses=1]
  store i32* %28, i32** %c, align 8
  store %struct.StructType* %s2, %struct.StructType** %ptr, align 8
  %29 = load %struct.StructType** %ptr, align 8   ; <%struct.StructType*> [#uses=1]
  %30 = getelementptr inbounds %struct.StructType* %29, i64 1 ; <%struct.StructType*> [#uses=1]
  store %struct.StructType* %30, %struct.StructType** %ptr1, align 8
  br label %return

return:                                           ; preds = %bb2
  ret void
}

declare noalias i8* @malloc(i64) nounwind

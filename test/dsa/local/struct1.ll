;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:tmp:0,func:s2:8,func:s1:0:8,func:c:0:8
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:c:0,func:s1:0

; ModuleID = 'struct1.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.StructType = type { i32, i32* }

define void @func() nounwind {
entry:
  %tmp = alloca i32*                              ; <i32**> [#uses=2]
  %s1 = alloca %struct.StructType*                ; <%struct.StructType**> [#uses=5]
  %c = alloca i32*                                ; <i32**> [#uses=1]
  %s2 = alloca %struct.StructType                 ; <%struct.StructType*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  store i32* %1, i32** %tmp, align 8
  %2 = call noalias i8* @malloc(i64 16) nounwind  ; <i8*> [#uses=1]
  %3 = bitcast i8* %2 to %struct.StructType*      ; <%struct.StructType*> [#uses=1]
  store %struct.StructType* %3, %struct.StructType** %s1, align 8
  %4 = load %struct.StructType** %s1, align 8     ; <%struct.StructType*> [#uses=1]
  %5 = getelementptr inbounds %struct.StructType* %4, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 10, i32* %5, align 8
  %6 = load %struct.StructType** %s1, align 8     ; <%struct.StructType*> [#uses=1]
  %7 = getelementptr inbounds %struct.StructType* %6, i32 0, i32 1 ; <i32**> [#uses=1]
  %8 = load i32** %tmp, align 8                   ; <i32*> [#uses=1]
  store i32* %8, i32** %7, align 8
  %9 = load %struct.StructType** %s1, align 8     ; <%struct.StructType*> [#uses=1]
  %10 = getelementptr inbounds %struct.StructType* %9, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32* %10, i32** %c, align 8
  %11 = load %struct.StructType** %s1, align 8    ; <%struct.StructType*> [#uses=2]
  %12 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 0 ; <i32*> [#uses=1]
  %13 = getelementptr inbounds %struct.StructType* %11, i32 0, i32 0 ; <i32*> [#uses=1]
  %14 = load i32* %13, align 8                    ; <i32> [#uses=1]
  store i32 %14, i32* %12, align 8
  %15 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 1 ; <i32**> [#uses=1]
  %16 = getelementptr inbounds %struct.StructType* %11, i32 0, i32 1 ; <i32**> [#uses=1]
  %17 = load i32** %16, align 8                   ; <i32*> [#uses=1]
  store i32* %17, i32** %15, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

declare noalias i8* @malloc(i64) nounwind

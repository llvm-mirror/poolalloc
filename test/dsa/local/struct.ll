
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:tmp:0,func:s2:8,func:s1:8
;RUN: dsaopt %s -dsa-local -analyze -check-same-node=func:c:0,func:s1


; ModuleID = 'struct.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.StructType = type { i32, i32* }

define void @func() nounwind {
entry:
  %tmp = alloca i32*                              ; <i32**> [#uses=2]
  %s1 = alloca %struct.StructType                 ; <%struct.StructType*> [#uses=5]
  %c = alloca i32*                                ; <i32**> [#uses=1]
  %s2 = alloca %struct.StructType                 ; <%struct.StructType*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  store i32* %1, i32** %tmp, align 8
  %2 = getelementptr inbounds %struct.StructType* %s1, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 10, i32* %2, align 8
  %3 = getelementptr inbounds %struct.StructType* %s1, i32 0, i32 1 ; <i32**> [#uses=1]
  %4 = load i32** %tmp, align 8                   ; <i32*> [#uses=1]
  store i32* %4, i32** %3, align 8
  %5 = getelementptr inbounds %struct.StructType* %s1, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32* %5, i32** %c, align 8
  %6 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 0 ; <i32*> [#uses=1]
  %7 = getelementptr inbounds %struct.StructType* %s1, i32 0, i32 0 ; <i32*> [#uses=1]
  %8 = load i32* %7, align 8                      ; <i32> [#uses=1]
  store i32 %8, i32* %6, align 8
  %9 = getelementptr inbounds %struct.StructType* %s2, i32 0, i32 1 ; <i32**> [#uses=1]
  %10 = getelementptr inbounds %struct.StructType* %s1, i32 0, i32 1 ; <i32**> [#uses=1]
  %11 = load i32** %10, align 8                   ; <i32*> [#uses=1]
  store i32* %11, i32** %9, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

declare noalias i8* @malloc(i64) nounwind

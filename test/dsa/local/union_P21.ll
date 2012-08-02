;RUN: dsaopt %s -dsa-local -analyze -verify-flags "main:obj1+UP2"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "main:obj+UP2"
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:obj,0:i32*::4:i32
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:obj1,0:i32\|i32*

; if any part of pointer overlaps with an interger, we must mark the into to ptr flag.
 

; ModuleID = 'union_P21.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.StructType = type { i32, i32 }
%union.UnionType = type { i32* }

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=1]
  %obj = alloca %union.UnionType                  ; <%union.UnionType*> [#uses=2]
  %d = alloca i32                                 ; <i32*> [#uses=1]
  %obj1 = alloca %union.UnionType                 ; <%union.UnionType*> [#uses=2]
  %e = alloca i32                                 ; <i32*> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  %2 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <i32**> [#uses=1]
  store i32* %1, i32** %2, align 8
  %3 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <i32**> [#uses=1]
  %4 = bitcast i32** %3 to %struct.StructType*    ; <%struct.StructType*> [#uses=1]
  %5 = getelementptr inbounds %struct.StructType* %4, i32 0, i32 1 ; <i32*> [#uses=1]
  %6 = load i32* %5, align 4                      ; <i32> [#uses=1]
  store i32 %6, i32* %d, align 4
  %7 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %8 = bitcast i8* %7 to i32*                     ; <i32*> [#uses=1]
  %9 = getelementptr inbounds %union.UnionType* %obj1, i32 0, i32 0 ; <i32**> [#uses=1]
  store i32* %8, i32** %9, align 8
  %10 = getelementptr inbounds %union.UnionType* %obj1, i32 0, i32 0 ; <i32**> [#uses=1]
  %11 = bitcast i32** %10 to %struct.StructType*  ; <%struct.StructType*> [#uses=1]
  %12 = getelementptr inbounds %struct.StructType* %11, i32 0, i32 0 ; <i32*> [#uses=1]
  %13 = load i32* %12, align 8                    ; <i32> [#uses=1]
  store i32 %13, i32* %e, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

declare noalias i8* @malloc(i64) nounwind

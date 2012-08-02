; ModuleID = 'union3.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
;RUN: dsaopt %s -dsa-local -analyze -check-type=func:obj,0:i32::4:i16\|i32::6:i16::8:i32
;RUN: dsaopt %s -dsa-local -analyze -enable-type-inference-opts -check-type=func:obj,0:i32::4:i32::8:i32

%struct.StructType1 = type { i32, i32, i32 }
%struct.StructType2 = type { i32, i16, i16, i32 }
%union.UnionType = type { %struct.StructType1 }

define void @func() nounwind {
entry:
  %obj = alloca %union.UnionType                  ; <%union.UnionType*> [#uses=4]
  %s = alloca %struct.StructType2                 ; <%struct.StructType2*> [#uses=4]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <%struct.StructType1*> [#uses=1]
  %1 = getelementptr inbounds %struct.StructType1* %0, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 2, i32* %1, align 4
  %2 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <%struct.StructType1*> [#uses=1]
  %3 = getelementptr inbounds %struct.StructType1* %2, i32 0, i32 1 ; <i32*> [#uses=1]
  store i32 33, i32* %3, align 4
  %4 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <%struct.StructType1*> [#uses=1]
  %5 = getelementptr inbounds %struct.StructType1* %4, i32 0, i32 2 ; <i32*> [#uses=1]
  store i32 22, i32* %5, align 4
  %6 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <%struct.StructType1*> [#uses=1]
  %7 = bitcast %struct.StructType1* %6 to %struct.StructType2* ; <%struct.StructType2*> [#uses=4]
  %8 = getelementptr inbounds %struct.StructType2* %s, i32 0, i32 0 ; <i32*> [#uses=1]
  %9 = getelementptr inbounds %struct.StructType2* %7, i32 0, i32 0 ; <i32*> [#uses=1]
  %10 = load i32* %9, align 4                     ; <i32> [#uses=1]
  store i32 %10, i32* %8, align 4
  %11 = getelementptr inbounds %struct.StructType2* %s, i32 0, i32 1 ; <i16*> [#uses=1]
  %12 = getelementptr inbounds %struct.StructType2* %7, i32 0, i32 1 ; <i16*> [#uses=1]
  %13 = load i16* %12, align 4                    ; <i16> [#uses=1]
  store i16 %13, i16* %11, align 4
  %14 = getelementptr inbounds %struct.StructType2* %s, i32 0, i32 2 ; <i16*> [#uses=1]
  %15 = getelementptr inbounds %struct.StructType2* %7, i32 0, i32 2 ; <i16*> [#uses=1]
  %16 = load i16* %15, align 2                    ; <i16> [#uses=1]
  store i16 %16, i16* %14, align 2
  %17 = getelementptr inbounds %struct.StructType2* %s, i32 0, i32 3 ; <i32*> [#uses=1]
  %18 = getelementptr inbounds %struct.StructType2* %7, i32 0, i32 3 ; <i32*> [#uses=1]
  %19 = load i32* %18, align 4                    ; <i32> [#uses=1]
  store i32 %19, i32* %17, align 4
  br label %return

return:                                           ; preds = %entry
  ret void
}

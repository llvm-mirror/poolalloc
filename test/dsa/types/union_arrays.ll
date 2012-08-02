; ModuleID = 'union_arrays.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
;RUN: dsaopt %s -dsa-local -analyze -check-type=func:obj,FoldedVOID
;RUN: dsaopt %s -dsa-local -analyze -enable-type-inference-opts -check-type=func:obj,FoldedVOID
;RUN: adsaopt %s -mem2reg -simplify-gep -mergearrgep -dce -o %t.bc
;RUN: dsaopt %t.bc -dsa-local -analyze -enable-type-inference-opts -check-type=func:obj,0:i32\|\[10xi32\]::40:i16\|\[10xi16\]::60:i32\|\[10xi32\]

%struct.StructType1 = type { [10 x i32], [10 x i16], [10 x i32] }
%struct.StructType2 = type { [10 x i32], [10 x i32], [10 x i32] }
%union.UnionType = type { %struct.StructType2 }

define void @func() nounwind {
entry:
  %obj = alloca %union.UnionType                  ; <%union.UnionType*> [#uses=6]
  %obj_copy = alloca %union.UnionType             ; <%union.UnionType*> [#uses=3]
  %i = alloca i32                                 ; <i32*> [#uses=20]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 0, i32* %i, align 4
  br label %bb1

bb:                                               ; preds = %bb1
  %0 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %1 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %2 = add nsw i32 %1, 10                         ; <i32> [#uses=1]
  %3 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <%struct.StructType2*> [#uses=1]
  %4 = bitcast %struct.StructType2* %3 to %struct.StructType1* ; <%struct.StructType1*> [#uses=1]
  %5 = getelementptr inbounds %struct.StructType1* %4, i32 0, i32 0 ; <[10 x i32]*> [#uses=1]
  %6 = sext i32 %0 to i64                         ; <i64> [#uses=1]
  %7 = getelementptr inbounds [10 x i32]* %5, i64 0, i64 %6 ; <i32*> [#uses=1]
  store i32 %2, i32* %7, align 4
  %8 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %9 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %10 = trunc i32 %9 to i16                       ; <i16> [#uses=1]
  %11 = add i16 %10, 32                           ; <i16> [#uses=1]
  %12 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <%struct.StructType2*> [#uses=1]
  %13 = bitcast %struct.StructType2* %12 to %struct.StructType1* ; <%struct.StructType1*> [#uses=1]
  %14 = getelementptr inbounds %struct.StructType1* %13, i32 0, i32 1 ; <[10 x i16]*> [#uses=1]
  %15 = sext i32 %8 to i64                        ; <i64> [#uses=1]
  %16 = getelementptr inbounds [10 x i16]* %14, i64 0, i64 %15 ; <i16*> [#uses=1]
  store i16 %11, i16* %16, align 2
  %17 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %18 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %19 = add nsw i32 %18, 64                       ; <i32> [#uses=1]
  %20 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <%struct.StructType2*> [#uses=1]
  %21 = bitcast %struct.StructType2* %20 to %struct.StructType1* ; <%struct.StructType1*> [#uses=1]
  %22 = getelementptr inbounds %struct.StructType1* %21, i32 0, i32 2 ; <[10 x i32]*> [#uses=1]
  %23 = sext i32 %17 to i64                       ; <i64> [#uses=1]
  %24 = getelementptr inbounds [10 x i32]* %22, i64 0, i64 %23 ; <i32*> [#uses=1]
  store i32 %19, i32* %24, align 4
  %25 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %26 = add nsw i32 %25, 1                        ; <i32> [#uses=1]
  store i32 %26, i32* %i, align 4
  br label %bb1

bb1:                                              ; preds = %bb, %entry
  %27 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %28 = icmp sle i32 %27, 9                       ; <i1> [#uses=1]
  br i1 %28, label %bb, label %bb2

bb2:                                              ; preds = %bb1
  store i32 0, i32* %i, align 4
  br label %bb4

bb3:                                              ; preds = %bb4
  %29 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %30 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %31 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <%struct.StructType2*> [#uses=1]
  %32 = bitcast %struct.StructType2* %31 to %struct.StructType1* ; <%struct.StructType1*> [#uses=1]
  %33 = getelementptr inbounds %struct.StructType1* %32, i32 0, i32 0 ; <[10 x i32]*> [#uses=1]
  %34 = sext i32 %30 to i64                       ; <i64> [#uses=1]
  %35 = getelementptr inbounds [10 x i32]* %33, i64 0, i64 %34 ; <i32*> [#uses=1]
  %36 = load i32* %35, align 4                    ; <i32> [#uses=1]
  %37 = getelementptr inbounds %union.UnionType* %obj_copy, i32 0, i32 0 ; <%struct.StructType2*> [#uses=1]
  %38 = getelementptr inbounds %struct.StructType2* %37, i32 0, i32 0 ; <[10 x i32]*> [#uses=1]
  %39 = sext i32 %29 to i64                       ; <i64> [#uses=1]
  %40 = getelementptr inbounds [10 x i32]* %38, i64 0, i64 %39 ; <i32*> [#uses=1]
  store i32 %36, i32* %40, align 4
  %41 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %42 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %43 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <%struct.StructType2*> [#uses=1]
  %44 = bitcast %struct.StructType2* %43 to %struct.StructType1* ; <%struct.StructType1*> [#uses=1]
  %45 = getelementptr inbounds %struct.StructType1* %44, i32 0, i32 1 ; <[10 x i16]*> [#uses=1]
  %46 = sext i32 %42 to i64                       ; <i64> [#uses=1]
  %47 = getelementptr inbounds [10 x i16]* %45, i64 0, i64 %46 ; <i16*> [#uses=1]
  %48 = load i16* %47, align 2                    ; <i16> [#uses=1]
  %49 = sext i16 %48 to i32                       ; <i32> [#uses=1]
  %50 = getelementptr inbounds %union.UnionType* %obj_copy, i32 0, i32 0 ; <%struct.StructType2*> [#uses=1]
  %51 = getelementptr inbounds %struct.StructType2* %50, i32 0, i32 1 ; <[10 x i32]*> [#uses=1]
  %52 = sext i32 %41 to i64                       ; <i64> [#uses=1]
  %53 = getelementptr inbounds [10 x i32]* %51, i64 0, i64 %52 ; <i32*> [#uses=1]
  store i32 %49, i32* %53, align 4
  %54 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %55 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %56 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <%struct.StructType2*> [#uses=1]
  %57 = bitcast %struct.StructType2* %56 to %struct.StructType1* ; <%struct.StructType1*> [#uses=1]
  %58 = getelementptr inbounds %struct.StructType1* %57, i32 0, i32 2 ; <[10 x i32]*> [#uses=1]
  %59 = sext i32 %55 to i64                       ; <i64> [#uses=1]
  %60 = getelementptr inbounds [10 x i32]* %58, i64 0, i64 %59 ; <i32*> [#uses=1]
  %61 = load i32* %60, align 4                    ; <i32> [#uses=1]
  %62 = getelementptr inbounds %union.UnionType* %obj_copy, i32 0, i32 0 ; <%struct.StructType2*> [#uses=1]
  %63 = getelementptr inbounds %struct.StructType2* %62, i32 0, i32 2 ; <[10 x i32]*> [#uses=1]
  %64 = sext i32 %54 to i64                       ; <i64> [#uses=1]
  %65 = getelementptr inbounds [10 x i32]* %63, i64 0, i64 %64 ; <i32*> [#uses=1]
  store i32 %61, i32* %65, align 4
  %66 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %67 = add nsw i32 %66, 1                        ; <i32> [#uses=1]
  store i32 %67, i32* %i, align 4
  br label %bb4

bb4:                                              ; preds = %bb3, %bb2
  %68 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %69 = icmp sle i32 %68, 9                       ; <i1> [#uses=1]
  br i1 %69, label %bb3, label %bb5

bb5:                                              ; preds = %bb4
  br label %return

return:                                           ; preds = %bb5
  ret void
}

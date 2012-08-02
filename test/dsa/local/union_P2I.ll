;checks that the PtrToInt and IntToPtr flag is set on unions that contain integer and pointer types

;RUN: dsaopt %s -dsa-local -analyze -check-same-node=main:ptr:0,main:obj:0
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:obj,FoldedVOID
;RUN: adsaopt %s -simplify-gep -mergearrgep -o t.bc
;RUN: dsaopt t.bc -dsa-local -enable-type-inference-opts -analyze -check-type=main:obj,0:i32\|\[100x%\struct.StructType\]


; ModuleID = 'union_P2.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.StructType = type { %struct.StructType*, double }
%union.UnionType = type { [100 x %struct.StructType] }

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=1]
  %obj = alloca %union.UnionType                  ; <%union.UnionType*> [#uses=2]
  %ptr = alloca %struct.StructType*               ; <%struct.StructType**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <[100 x %struct.StructType]*> [#uses=1]
  %1 = bitcast [100 x %struct.StructType]* %0 to i32* ; <i32*> [#uses=1]
  store i32 123456, i32* %1, align 8
  %2 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <[100 x %struct.StructType]*> [#uses=1]
  %3 = getelementptr inbounds [100 x %struct.StructType]* %2, i64 0, i64 0 ; <%struct.StructType*> [#uses=1]
  %4 = getelementptr inbounds %struct.StructType* %3, i32 0, i32 0 ; <%struct.StructType**> [#uses=1]
  %5 = load %struct.StructType** %4, align 8      ; <%struct.StructType*> [#uses=1]
  store %struct.StructType* %5, %struct.StructType** %ptr, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

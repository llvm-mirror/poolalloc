;RUN: dsaopt %s -dsa-local -analyze -check-same-node=main:c:0,main:obj:0
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:obj,FoldedVOID

;union of array of int/int*. Must get collapsed, as the element type is not of consistent size

; ModuleID = 'union_P2I_1.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%union.UnionType = type { [100 x i32*] }

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=1]
  %obj = alloca %union.UnionType                  ; <%union.UnionType*> [#uses=2]
  %c = alloca i32*                                ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <[100 x i32*]*> [#uses=1]
  %1 = bitcast [100 x i32*]* %0 to [100 x i32]*   ; <[100 x i32]*> [#uses=1]
  %2 = getelementptr inbounds [100 x i32]* %1, i64 0, i64 3 ; <i32*> [#uses=1]
  store i32 123456, i32* %2, align 4
  %3 = getelementptr inbounds %union.UnionType* %obj, i32 0, i32 0 ; <[100 x i32*]*> [#uses=1]
  %4 = getelementptr inbounds [100 x i32*]* %3, i64 0, i64 3 ; <i32**> [#uses=1]
  %5 = load i32** %4, align 8                     ; <i32*> [#uses=1]
  store i32* %5, i32** %c, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

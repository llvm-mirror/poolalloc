; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
;RUN: dsaopt %s -dsa-local -analyze -check-type=tree,FoldedVOID
;RUN: dsaopt %s -dsa-local -enable-type-inference-opts -analyze -check-type=tree,12:\[8xi32\]Array
; LLVM front end converts the type of tree, to a struct type instead of an array of the right type.
; even though structurally equivalent, DSA cant infer this yet.


%0 = type { %struct..0tnode, %struct..0tnode, %struct..0tnode, %struct..0tnode, %struct..0tnode, %struct..0tnode, %struct..0tnode, %struct..0tnode, %1, %1, %1, %1, %1, %1, %struct..0tnode, %struct..0tnode, %struct..0tnode, %struct..0tnode, %struct..0tnode, %struct..0tnode, %1 }
%1 = type { i32, i32, i32, [32 x i8] }
%struct..0tnode = type { i32, i32, i32, [8 x i32] }

@tree = external global %0, align 32         ; <%0*> [#uses=1]

define i32 @opening(i32* %i, i32* %j, i32 %type) nounwind {
entry:
  br i1 undef, label %bb8, label %bb10

bb8:                                              ; preds = %entry
  br i1 undef, label %bb2.i, label %random_nasko.exit

bb2.i:                                            ; preds = %bb8
  br label %random_nasko.exit

random_nasko.exit:                                ; preds = %bb2.i, %bb8
  %tmp81moda7 = getelementptr [21 x %struct..0tnode]* bitcast (%0* @tree to [21 x %struct..0tnode]*), i64 0, i64 undef, i32 3, i64 undef ; <i32*> [#uses=0]
  ret i32 1

bb10:                                             ; preds = %entry
  ret i32 0
}

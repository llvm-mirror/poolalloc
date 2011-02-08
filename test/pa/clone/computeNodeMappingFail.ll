;This hits an assert in computeNodeMapping, when offset of merging node is greater than 1st node. 
;RUN: paopt %s -paheur-AllButUnreachableFromMemory -poolalloc -o %t.bc |& grep "Pool allocating.*nodes!"
;RUN: llvm-dis %t.bc -o %t.ll

; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.RefObj = type { i8*, %struct.TypeToken }
%struct.TypeToken = type { i32, i16, i16 }

define fastcc void @ImageTokenToRef(%struct.TypeToken* %Token, i32 %AttrNum, i32* %Status, i8** nocapture %RefObject) nounwind {
entry:
  %RefTkn = alloca %struct.TypeToken, align 8     ; <%struct.TypeToken*> [#uses=1]
  br i1 undef, label %bb13, label %bb

bb:                                               ; preds = %entry
  unreachable

bb13:                                             ; preds = %entry
  br i1 undef, label %bb14, label %bb33

bb14:                                             ; preds = %bb13
  br i1 undef, label %bb15, label %bb21

bb15:                                             ; preds = %bb14
  br i1 undef, label %bb17, label %KernelGetAttr.exit

KernelGetAttr.exit:                               ; preds = %bb15
  unreachable

bb17:                                             ; preds = %bb15
  %tmp62 = call fastcc i32 @ImageGetObject(%struct.TypeToken* %RefTkn, i32* %Status, i8** %RefObject) nounwind ; <i32> [#uses=0]
  unreachable

bb21:                                             ; preds = %bb14
  br i1 undef, label %bb23, label %KernelGetAttr.exit42

KernelGetAttr.exit42:                             ; preds = %bb21
  unreachable

bb23:                                             ; preds = %bb21
  %tmp72 = getelementptr inbounds %struct.RefObj* undef, i64 0, i32 1 ; <%struct.TypeToken*> [#uses=1]
  %tmp85 = call fastcc i32 @ImageGetObject(%struct.TypeToken* %tmp72, i32* %Status, i8** %RefObject) nounwind ; <i32> [#uses=0]
  unreachable

bb33:                                             ; preds = %bb13
  ret void
}

define fastcc i32 @ImageGetObject(%struct.TypeToken* %Token, i32* %Status, i8** nocapture %This) nounwind {
entry:
  unreachable
}

;RUN: dsaopt %s -ds-aa -gvn
; FIXME: Make this test more directly test the underlying issue.
; (the -ds-aa and -gvn are just ways of forcing the right query to be made)

;(Reduced from Stanford/Treesort)
; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.node = type { %struct.node*, %struct.node*, i32 }

@sortlist = external global [5001 x i32], align 32 ; <[5001 x i32]*> [#uses=1]

define fastcc i32 @Checktree(%struct.node* nocapture %p) nounwind readonly {
entry:
  ret i32 1
}

define i32 @main() nounwind {
bb.nph:
  br label %bb

bb:                                               ; preds = %Trees.exit, %bb.nph
  br label %bb.i.i

bb.i.i:                                           ; preds = %bb.i.i, %bb
  br i1 undef, label %bb.nph.i, label %bb.i.i

bb.nph.i:                                         ; preds = %bb.i.i
  br label %bb.i

bb.i:                                             ; preds = %bb1.i.i, %bb.nph.i
  %scevgep.i = getelementptr [5001 x i32]* @sortlist, i64 0, i64 0 ; <i32*> [#uses=1]
  %0 = load i32* %scevgep.i, align 4              ; <i32> [#uses=0]
  br label %bb.i1.i

bb.i1.i:                                          ; preds = %bb.i1.i, %bb.i
  br i1 undef, label %bb1.i.i, label %bb.i1.i

bb1.i.i:                                          ; preds = %bb.i1.i
  br i1 undef, label %bb2.i, label %bb.i

bb2.i:                                            ; preds = %bb1.i.i
  br i1 undef, label %bb7.i.i, label %bb.i4.i

bb.i4.i:                                          ; preds = %bb2.i
  br i1 undef, label %bb2.i5.i, label %bb7.i.i

bb2.i5.i:                                         ; preds = %bb.i4.i
  br i1 undef, label %Checktree.exit, label %bb8.i

bb8.i:                                            ; preds = %bb2.i5.i
  br i1 undef, label %bb10.i, label %bb5.i6.i

bb10.i:                                           ; preds = %bb8.i
  %1 = tail call fastcc i32 @Checktree(%struct.node* undef) nounwind ; <i32> [#uses=0]
  br label %Checktree.exit

Checktree.exit:                                   ; preds = %bb10.i, %bb2.i5.i
  br i1 undef, label %bb5.i6.i, label %bb7.i.i

bb5.i6.i:                                         ; preds = %Checktree.exit, %bb8.i
  br label %bb7.i.i

bb7.i.i:                                          ; preds = %bb5.i6.i, %Checktree.exit, %bb.i4.i, %bb2.i
  br i1 undef, label %bb3.i, label %Trees.exit

bb3.i:                                            ; preds = %bb7.i.i
  unreachable

Trees.exit:                                       ; preds = %bb7.i.i
  br i1 undef, label %bb2, label %bb

bb2:                                              ; preds = %Trees.exit
  ret i32 0
}

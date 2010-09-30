; ModuleID = 'bugpoint-reduced-simplified.bc'
;RUN: paopt %s -paheur-AllButUnreachableFromMemory -poolalloc -disable-output 2>&1
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.TypHeader = type { i64, %struct.TypHeader**, [3 x i8], i8 }

define fastcc void @InitEval() nounwind {
bb.nph49:
  br label %bb

bb:                                               ; preds = %bb, %bb.nph49
  br i1 undef, label %bb5.preheader, label %bb

bb4:                                              ; preds = %bb5.preheader, %bb4
  br i1 undef, label %bb6, label %bb4

bb6:                                              ; preds = %bb4
  br i1 undef, label %bb11.preheader, label %bb5.preheader

bb5.preheader:                                    ; preds = %bb6, %bb
  br label %bb4

bb10:                                             ; preds = %bb11.preheader, %bb10
  br i1 undef, label %bb14.loopexit, label %bb10

bb13:                                             ; preds = %bb14.loopexit, %bb13
  br i1 undef, label %bb15, label %bb13

bb14.loopexit:                                    ; preds = %bb10
  br i1 undef, label %bb13, label %bb15

bb15:                                             ; preds = %bb14.loopexit, %bb13
  br i1 undef, label %bb17, label %bb11.preheader

bb11.preheader:                                   ; preds = %bb15, %bb6
  br label %bb10

bb17:                                             ; preds = %bb15
  store %struct.TypHeader* bitcast (%struct.TypHeader* (%struct.TypHeader*)* @IntComm to %struct.TypHeader*), %struct.TypHeader** undef
  unreachable
}

define %struct.TypHeader* @IntComm(%struct.TypHeader* nocapture %hdCall) nounwind {
entry:
  unreachable
}

define i32 @main(i32 %argc, i8** nocapture %argv) noreturn nounwind {
entry:
  unreachable
}

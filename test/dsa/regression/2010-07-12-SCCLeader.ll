;RUN: dsaopt %s -dsa-eq -disable-output
; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.TypHeader = type { i64, %struct.TypHeader**, [3 x i8], i8 }

@.str4251 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=1]
@.str8302 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=1]
@HdIdenttab = external global %struct.TypHeader*  ; <%struct.TypHeader**> [#uses=1]

define %struct.TypHeader* @FunCoeffsCyc(%struct.TypHeader* nocapture %hdCall) nounwind {
entry:
  ret %struct.TypHeader* null
}

define fastcc void @InitEval() nounwind {

InstIntFunc.exit17:                               ; preds = %bb.i16, %InstIntFunc.exit20
  %tmp79 = tail call fastcc %struct.TypHeader* @NewBag(i32 16, i64 8) nounwind ; <%struct.TypHeader*> [#uses=2]
  %tmp80 = getelementptr inbounds %struct.TypHeader* %tmp79, i64 0, i32 1 ; <%struct.TypHeader***> [#uses=1]
  %tmp81 = load %struct.TypHeader*** %tmp80, align 8 ; <%struct.TypHeader**> [#uses=1]
  store %struct.TypHeader* bitcast (%struct.TypHeader* (%struct.TypHeader*)* @FunIsBound to %struct.TypHeader*), %struct.TypHeader** %tmp81
  %tmp82 = tail call fastcc %struct.TypHeader* @FindIdent(i8* getelementptr inbounds ([8 x i8]* @.str8302, i64 0, i64 0)) nounwind ; <%struct.TypHeader*> [#uses=1]
  %tmp83 = getelementptr inbounds %struct.TypHeader* %tmp82, i64 0, i32 1 ; <%struct.TypHeader***> [#uses=1]
  %tmp84 = load %struct.TypHeader*** %tmp83, align 8 ; <%struct.TypHeader**> [#uses=1]
  br i1 undef, label %InstIntFunc.exit14, label %bb.i13

bb.i13:                                           ; preds = %InstIntFunc.exit17
  unreachable

InstIntFunc.exit14:                               ; preds = %InstIntFunc.exit17
  store %struct.TypHeader* %tmp79, %struct.TypHeader** %tmp84, align 8
  br i1 undef, label %InstIntFunc.exit11, label %bb.i10

bb.i10:                                           ; preds = %InstIntFunc.exit14
  unreachable

InstIntFunc.exit11:                               ; preds = %InstIntFunc.exit14
  br i1 undef, label %InstIntFunc.exit9.i, label %bb.i8.i

bb.i8.i:                                          ; preds = %InstIntFunc.exit11
  unreachable

InstIntFunc.exit9.i:                              ; preds = %InstIntFunc.exit11
  br i1 undef, label %InstIntFunc.exit6.i, label %bb.i5.i

bb.i5.i:                                          ; preds = %InstIntFunc.exit9.i
  unreachable

InstIntFunc.exit6.i:                              ; preds = %InstIntFunc.exit9.i
  br i1 undef, label %InstIntFunc.exit3.i, label %bb.i2.i

bb.i2.i:                                          ; preds = %InstIntFunc.exit6.i
  unreachable

InstIntFunc.exit3.i:                              ; preds = %InstIntFunc.exit6.i
  br i1 undef, label %InitInt.exit, label %bb.i.i

bb.i.i:                                           ; preds = %InstIntFunc.exit3.i
  br label %InitInt.exit

InitInt.exit:                                     ; preds = %bb.i.i, %InstIntFunc.exit3.i
  br i1 undef, label %InstIntFunc.exit6.i419, label %bb.i5.i418

bb.i5.i418:                                       ; preds = %InitInt.exit
  unreachable

InstIntFunc.exit6.i419:                           ; preds = %InitInt.exit
  br i1 undef, label %InstIntFunc.exit3.i422, label %bb.i2.i421

bb.i2.i421:                                       ; preds = %InstIntFunc.exit6.i419
  unreachable

InstIntFunc.exit3.i422:                           ; preds = %InstIntFunc.exit6.i419
  br i1 undef, label %InitRat.exit, label %bb.i.i424

bb.i.i424:                                        ; preds = %InstIntFunc.exit3.i422
  unreachable

InitRat.exit:                                     ; preds = %InstIntFunc.exit3.i422
  br label %bb.i396

bb.i396:                                          ; preds = %bb.i396, %InitRat.exit
  br i1 undef, label %bb2.i398, label %bb.i396

bb2.i398:                                         ; preds = %bb.i396
  br i1 undef, label %InstIntFunc.exit15.i401, label %bb.i14.i400

bb.i14.i400:                                      ; preds = %bb2.i398
  br label %InstIntFunc.exit15.i401

InstIntFunc.exit15.i401:                          ; preds = %bb.i14.i400, %bb2.i398
  br i1 undef, label %InstIntFunc.exit12.i404, label %bb.i11.i403

bb.i11.i403:                                      ; preds = %InstIntFunc.exit15.i401
  unreachable

InstIntFunc.exit12.i404:                          ; preds = %InstIntFunc.exit15.i401
  br i1 undef, label %InstIntFunc.exit9.i407, label %bb.i8.i406

bb.i8.i406:                                       ; preds = %InstIntFunc.exit12.i404
  unreachable

InstIntFunc.exit9.i407:                           ; preds = %InstIntFunc.exit12.i404
  br i1 undef, label %InstIntFunc.exit6.i410, label %bb.i5.i409

bb.i5.i409:                                       ; preds = %InstIntFunc.exit9.i407
  br label %InstIntFunc.exit6.i410

InstIntFunc.exit6.i410:                           ; preds = %bb.i5.i409, %InstIntFunc.exit9.i407
  %tmp215 = tail call fastcc %struct.TypHeader* @NewBag(i32 16, i64 8) nounwind ; <%struct.TypHeader*> [#uses=2]
  %tmp216 = getelementptr inbounds %struct.TypHeader* %tmp215, i64 0, i32 1 ; <%struct.TypHeader***> [#uses=1]
  %tmp217 = load %struct.TypHeader*** %tmp216, align 8 ; <%struct.TypHeader**> [#uses=1]
  store %struct.TypHeader* bitcast (%struct.TypHeader* (%struct.TypHeader*)* @FunCoeffsCyc to %struct.TypHeader*), %struct.TypHeader** %tmp217
  %tmp218 = tail call fastcc %struct.TypHeader* @FindIdent(i8* getelementptr inbounds ([10 x i8]* @.str4251, i64 0, i64 0)) nounwind ; <%struct.TypHeader*> [#uses=1]
  %tmp219 = getelementptr inbounds %struct.TypHeader* %tmp218, i64 0, i32 1 ; <%struct.TypHeader***> [#uses=1]
  %tmp220 = load %struct.TypHeader*** %tmp219, align 8 ; <%struct.TypHeader**> [#uses=1]
  br i1 undef, label %InstIntFunc.exit3.i413, label %bb.i2.i412

bb.i2.i412:                                       ; preds = %InstIntFunc.exit6.i410
  unreachable

InstIntFunc.exit3.i413:                           ; preds = %InstIntFunc.exit6.i410
  store %struct.TypHeader* %tmp215, %struct.TypHeader** %tmp220, align 8
  unreachable
}

define %struct.TypHeader* @FunIsBound(%struct.TypHeader* nocapture %hdCall) nounwind {
entry:
  br i1 undef, label %bb1, label %bb

bb:                                               ; preds = %entry
  ret %struct.TypHeader* undef

bb1:                                              ; preds = %entry
  br i1 undef, label %bb3, label %bb24

bb3:                                              ; preds = %bb1
  br i1 undef, label %bb25, label %bb24

bb24:                                             ; preds = %bb3, %bb1
  ret %struct.TypHeader* undef

bb25:                                             ; preds = %bb3
  br i1 undef, label %bb28, label %bb86

bb28:                                             ; preds = %bb25
  br i1 undef, label %bb133, label %bb86

bb86:                                             ; preds = %bb28, %bb25
  br i1 undef, label %bb93, label %bb94

bb93:                                             ; preds = %bb86
  unreachable

bb94:                                             ; preds = %bb86
  br i1 undef, label %bb97, label %bb108

bb97:                                             ; preds = %bb94
  ret %struct.TypHeader* undef

bb108:                                            ; preds = %bb94
  br i1 undef, label %bb110, label %bb109

bb109:                                            ; preds = %bb108
  %tmp91 = bitcast %struct.TypHeader** undef to i8* ; <i8*> [#uses=1]
  %tmp92 = call fastcc %struct.TypHeader* @FindRecname(i8* %tmp91) nounwind ; <%struct.TypHeader*> [#uses=0]
  unreachable

bb110:                                            ; preds = %bb108
  ret %struct.TypHeader* undef

bb133:                                            ; preds = %bb28
  ret %struct.TypHeader* undef
}

define fastcc %struct.TypHeader* @NewBag(i32 %type, i64 %size) nounwind {
entry:
  br i1 undef, label %bb3, label %bb2

bb2:                                              ; preds = %entry
  br i1 undef, label %bb22, label %bb28

bb3:                                              ; preds = %entry
  unreachable

bb22:                                             ; preds = %bb2
  unreachable

bb28:                                             ; preds = %bb2
  br i1 undef, label %bb30, label %bb29

bb29:                                             ; preds = %bb28
  unreachable

bb30:                                             ; preds = %bb28
  br i1 undef, label %bb33, label %bb31

bb31:                                             ; preds = %bb30
  br i1 undef, label %bb32, label %bb33

bb32:                                             ; preds = %bb31
  tail call fastcc void @Resize(%struct.TypHeader* undef, i64 undef) nounwind
  ret %struct.TypHeader* undef

bb33:                                             ; preds = %bb31, %bb30
  ret %struct.TypHeader* undef
}

define fastcc void @Resize(%struct.TypHeader* %hdBag, i64 %newSize) nounwind {
entry:
  br i1 undef, label %bb1, label %bb2

bb1:                                              ; preds = %entry
  br label %bb2

bb2:                                              ; preds = %bb1, %entry
  br i1 undef, label %bb3, label %bb4

bb3:                                              ; preds = %bb2
  ret void

bb4:                                              ; preds = %bb2
  %tmp53 = tail call fastcc %struct.TypHeader* @NewBag(i32 undef, i64 %newSize) nounwind ; <%struct.TypHeader*> [#uses=0]
  unreachable
}

define fastcc %struct.TypHeader* @FindRecname(i8* nocapture %name) nounwind {
entry:
  br i1 undef, label %bb8, label %bb5

bb5:                                              ; preds = %entry
  unreachable

bb8:                                              ; preds = %entry
  %tmp24 = tail call fastcc %struct.TypHeader* @NewBag(i32 78, i64 0) nounwind ; <%struct.TypHeader*> [#uses=1]
  %tmp26 = getelementptr inbounds %struct.TypHeader* %tmp24, i64 0, i32 1 ; <%struct.TypHeader***> [#uses=1]
  %tmp27 = load %struct.TypHeader*** %tmp26, align 8 ; <%struct.TypHeader**> [#uses=1]
  %tmp29 = bitcast %struct.TypHeader** %tmp27 to i8* ; <i8*> [#uses=1]
  %tmp30 = tail call i8* @strncat(i8* %tmp29, i8* %name, i64 undef) nounwind ; <i8*> [#uses=0]
  unreachable
}

define fastcc %struct.TypHeader* @FindIdent(i8* nocapture %name) nounwind {
entry:
  br i1 undef, label %bb10, label %bb8

bb8:                                              ; preds = %entry
  br label %bb10

bb10:                                             ; preds = %bb8, %entry
  %tmp28 = load %struct.TypHeader** @HdIdenttab, align 8 ; <%struct.TypHeader*> [#uses=1]
  %tmp32 = getelementptr inbounds %struct.TypHeader* %tmp28, i64 0, i32 1 ; <%struct.TypHeader***> [#uses=1]
  %tmp33 = load %struct.TypHeader*** %tmp32, align 8 ; <%struct.TypHeader**> [#uses=1]
  br label %bb12

bb11:                                             ; preds = %bb13
  br label %bb12

bb12:                                             ; preds = %bb11, %bb10
  %tmp36 = getelementptr inbounds %struct.TypHeader** %tmp33, i64 undef ; <%struct.TypHeader**> [#uses=1]
  %tmp37 = load %struct.TypHeader** %tmp36, align 8 ; <%struct.TypHeader*> [#uses=2]
  br i1 undef, label %bb19, label %bb13

bb13:                                             ; preds = %bb12
  br i1 undef, label %bb14, label %bb11

bb14:                                             ; preds = %bb13
  br i1 undef, label %bb19, label %bb15

bb15:                                             ; preds = %bb14
  br i1 undef, label %bb18, label %bb17

bb17:                                             ; preds = %bb15
  ret %struct.TypHeader* %tmp37

bb18:                                             ; preds = %bb15
  ret %struct.TypHeader* %tmp37

bb19:                                             ; preds = %bb14, %bb12
  unreachable
}

declare i8* @strncat(i8*, i8* nocapture, i64) nounwind

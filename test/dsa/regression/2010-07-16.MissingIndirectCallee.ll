;RUN: dsaopt %s -dsa-cbu -disable-output
; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-apple-darwin10.4"

%0 = type { %struct.block }
%struct.AV = type { %struct.XPVAV*, i32, i32 }
%struct.COP = type { %struct.OP*, %struct.OP*, %struct.OP* ()*, i32, i16, i16, i8, i8, i8*, %struct.HV*, %struct.GV*, i32, i32, i16 }
%struct.CV = type { %struct.XPVCV*, i32, i32 }
%struct.DIR = type { i32, i64, i64, i8*, i32, i64, i64, i32, %struct.__darwin_pthread_mutex_t, %struct._telldir* }
%struct.FILE = type { i8*, i32, i32, i16, i16, %struct.__sbuf, i32, i8*, i32 (i8*)*, i32 (i8*, i8*, i32)*, i64 (i8*, i64, i32)*, i32 (i8*, i8*, i32)*, %struct.__sbuf, %struct.__sFILEX*, i32, [3 x i8], [1 x i8], %struct.__sbuf, i32, i64 }
%struct.GP = type { %struct.SV*, i32, %struct.io*, %struct.CV*, %struct.AV*, %struct.HV*, %struct.GV*, %struct.CV*, i32, i32, i16, %struct.GV* }
%struct.GV = type { %struct.XPVGV*, i32, i32 }
%struct.HE = type { %struct.HE*, %struct.HEK*, %struct.SV* }
%struct.HEK = type { i32, i32, [1 x i8] }
%struct.HV = type { %struct.XPVHV*, i32, i32 }
%struct.MAGIC = type { %struct.MAGIC*, %struct.MGVTBL*, i16, i8, i8, %struct.SV*, i8*, i32 }
%struct.MGVTBL = type { i32 (%struct.SV*, %struct.MAGIC*)*, i32 (%struct.SV*, %struct.MAGIC*)*, i32 (%struct.SV*, %struct.MAGIC*)*, i32 (%struct.SV*, %struct.MAGIC*)*, i32 (%struct.SV*, %struct.MAGIC*)* }
%struct.OP = type { %struct.OP*, %struct.OP*, %struct.OP* ()*, i32, i16, i16, i8, i8 }
%struct.PERL_CONTEXT = type { i32, %0 }
%struct.PMOP = type { %struct.OP*, %struct.OP*, %struct.OP* ()*, i32, i16, i16, i8, i8, %struct.OP*, %struct.OP*, i32, %struct.OP*, %struct.OP*, %struct.PMOP*, %struct.REGEXP*, i16, i16, i8 }
%struct.REGEXP = type { i32, i8**, i8**, %struct.regnode*, i32, i32, i32, i32, i8*, i8*, i8*, i8*, i16, i16, %struct.reg_substr_data*, %struct.reg_data*, [1 x %struct.regnode] }
%struct.SV = type { i8*, i32, i32 }
%struct.XPVAV = type { i8*, i64, i64, i64, double, %struct.MAGIC*, %struct.HV*, %struct.SV**, %struct.SV*, i8 }
%struct.XPVCV = type { i8*, i64, i64, i64, double, %struct.MAGIC*, %struct.HV*, %struct.HV*, %struct.OP*, %struct.OP*, void (%struct.CV*)*, %union.ANY, %struct.GV*, %struct.GV*, i64, %struct.AV*, %struct.CV*, i16 }
%struct.XPVGV = type { i8*, i64, i64, i64, double, %struct.MAGIC*, %struct.HV*, %struct.GP*, i8*, i64, %struct.HV*, i8 }
%struct.XPVHV = type { i8*, i64, i64, i64, double, %struct.MAGIC*, %struct.HV*, i32, %struct.HE*, %struct.PMOP*, i8* }
%struct.XPVIO = type { i8*, i64, i64, i64, double, %struct.MAGIC*, %struct.HV*, %struct.FILE*, %struct.FILE*, %struct.DIR*, i64, i64, i64, i64, i8*, %struct.GV*, i8*, %struct.GV*, i8*, %struct.GV*, i16, i8, i8 }
%struct.__darwin_pthread_mutex_t = type { i64, [56 x i8] }
%struct.__sFILEX = type opaque
%struct.__sbuf = type { i8*, i32 }
%struct._telldir = type opaque
%struct.block = type { i32, %struct.COP*, i32, i32, i32, %struct.PMOP*, i8, %union.anon }
%struct.block_loop = type { i8*, i32, %struct.OP*, %struct.OP*, %struct.OP*, %struct.SV**, %struct.SV*, %struct.SV*, %struct.AV*, i64, i64 }
%struct.io = type { %struct.XPVIO*, i32, i32 }
%struct.reg_data = type { i32, i8*, [1 x i8*] }
%struct.reg_substr_data = type { [3 x %struct.reg_substr_datum] }
%struct.reg_substr_datum = type { i32, i32, %struct.SV* }
%struct.regnode = type { i8, i8, i16 }
%union.ANY = type { i8* }
%union.anon = type { %struct.block_loop }

@PL_sv_root = external global %struct.SV*         ; <%struct.SV**> [#uses=3]
@PL_stack_sp = external global %struct.SV**       ; <%struct.SV***> [#uses=2]
@PL_curpad = external global %struct.SV**         ; <%struct.SV***> [#uses=3]
@PL_savestack = external global %union.ANY*       ; <%union.ANY**> [#uses=1]

declare void @XS_MD5_digest(%struct.CV* nocapture) nounwind ssp

define %struct.OP* @Perl_pp_require_DIRECT() nounwind ssp {
entry:
  %tmp = load %struct.SV*** @PL_stack_sp, align 8 ; <%struct.SV**> [#uses=1]
  %tmp1 = load %struct.SV** %tmp, align 8         ; <%struct.SV*> [#uses=1]
  br i1 undef, label %bb12, label %bb11

bb11:                                             ; preds = %entry
  %tmp38 = getelementptr inbounds %struct.SV* %tmp1, i64 0, i32 0 ; <i8**> [#uses=1]
  %tmp43 = load i8** %tmp38, align 8              ; <i8*> [#uses=1]
  %tmp44 = bitcast i8* %tmp43 to i8**             ; <i8**> [#uses=1]
  %tmp45 = load i8** %tmp44, align 8              ; <i8*> [#uses=1]
  br i1 undef, label %bb16, label %bb14

bb12:                                             ; preds = %entry
  unreachable

bb14:                                             ; preds = %bb11
  br i1 undef, label %bb16, label %bb15

bb15:                                             ; preds = %bb14
  br i1 undef, label %bb16, label %bb17

bb16:                                             ; preds = %bb15, %bb14, %bb11
  ret %struct.OP* undef

bb17:                                             ; preds = %bb15
  br i1 undef, label %bb31, label %bb32

bb31:                                             ; preds = %bb17
  unreachable

bb32:                                             ; preds = %bb17
  br i1 undef, label %bb34, label %bb35

bb34:                                             ; preds = %bb32
  unreachable

bb35:                                             ; preds = %bb32
  %tmp107 = load %struct.SV** @PL_sv_root, align 8 ; <%struct.SV*> [#uses=2]
  br i1 undef, label %bb1.i, label %bb.i4

bb.i4:                                            ; preds = %bb35
  %tmp109 = getelementptr inbounds %struct.SV* %tmp107, i64 0, i32 0 ; <i8**> [#uses=1]
  %tmp110 = load i8** %tmp109, align 8            ; <i8*> [#uses=1]
  %tmp111 = bitcast i8* %tmp110 to %struct.SV*    ; <%struct.SV*> [#uses=1]
  store %struct.SV* %tmp111, %struct.SV** @PL_sv_root, align 8
  %tmp115 = getelementptr inbounds %struct.SV* %tmp107, i64 0, i32 0 ; <i8**> [#uses=1]
  br label %bb46

bb1.i:                                            ; preds = %bb35
  unreachable

bb36:                                             ; preds = %bb48
  %tmp148 = load i8** %tmp115, align 8            ; <i8*> [#uses=1]
  %tmp149 = bitcast i8* %tmp148 to i8**           ; <i8**> [#uses=1]
  %tmp150 = load i8** %tmp149, align 8            ; <i8*> [#uses=1]
  br label %bb46

bb46:                                             ; preds = %bb36, %bb.i4
  %tryname.1 = phi i8* [ %tmp150, %bb36 ], [ null, %bb.i4 ] ; <i8*> [#uses=1]
  br i1 undef, label %bb48, label %bb47

bb47:                                             ; preds = %bb46
  unreachable

bb48:                                             ; preds = %bb46
  br i1 undef, label %bb50, label %bb36

bb50:                                             ; preds = %bb48
  %storemerge4 = select i1 undef, i8* %tmp45, i8* %tryname.1 ; <i8*> [#uses=0]
  unreachable
}

define %struct.OP* @Perl_pp_goto_DIRECT() nounwind ssp {
entry:
  %tmp1 = load %struct.SV*** @PL_stack_sp, align 8 ; <%struct.SV**> [#uses=1]
  %tmp11 = load %struct.SV** %tmp1, align 8       ; <%struct.SV*> [#uses=1]
  br i1 undef, label %bb117, label %bb1

bb1:                                              ; preds = %entry
  %tmp17 = getelementptr inbounds %struct.SV* %tmp11, i64 0, i32 0 ; <i8**> [#uses=1]
  %tmp18 = load i8** %tmp17, align 8              ; <i8*> [#uses=1]
  %tmp19 = bitcast i8* %tmp18 to %struct.SV**     ; <%struct.SV**> [#uses=1]
  %tmp20 = load %struct.SV** %tmp19, align 8      ; <%struct.SV*> [#uses=1]
  br i1 undef, label %bb2, label %bb117

bb2:                                              ; preds = %bb1
  %tmp25 = bitcast %struct.SV* %tmp20 to %struct.CV* ; <%struct.CV*> [#uses=3]
  %tmp26 = getelementptr inbounds %struct.CV* %tmp25, i64 0, i32 0 ; <%struct.XPVCV**> [#uses=1]
  %tmp90 = load %struct.PERL_CONTEXT** undef, align 8 ; <%struct.PERL_CONTEXT*> [#uses=2]
  br i1 undef, label %bb.i.i, label %dopoptosub.exit

bb.i.i:                                           ; preds = %bb2
  br label %dopoptosub.exit

dopoptosub.exit:                                  ; preds = %bb.i.i, %bb2
  br i1 undef, label %bb13, label %bb14

bb13:                                             ; preds = %dopoptosub.exit
  ret %struct.OP* undef

bb14:                                             ; preds = %dopoptosub.exit
  br i1 undef, label %bb15, label %bb16

bb15:                                             ; preds = %bb14
  unreachable

bb16:                                             ; preds = %bb14
  switch i32 undef, label %bb37 [
    i32 2, label %bb18
    i32 1, label %bb20
  ]

bb18:                                             ; preds = %bb16
  ret %struct.OP* undef

bb20:                                             ; preds = %bb16
  br i1 undef, label %bb37, label %bb21

bb21:                                             ; preds = %bb20
  br i1 undef, label %bb22, label %bb23

bb22:                                             ; preds = %bb21
  br label %bb23

bb23:                                             ; preds = %bb22, %bb21
  br i1 undef, label %bb28, label %bb27

bb27:                                             ; preds = %bb23
  br label %bb28

bb28:                                             ; preds = %bb27, %bb23
  unreachable

bb37:                                             ; preds = %bb20, %bb16
  br i1 undef, label %bb41, label %bb42

bb41:                                             ; preds = %bb37
  unreachable

bb42:                                             ; preds = %bb37
  br i1 undef, label %bb.i1, label %entry.bb1_crit_edge.i

entry.bb1_crit_edge.i:                            ; preds = %bb42
  br i1 undef, label %bb51, label %bb43

bb.i1:                                            ; preds = %bb42
  unreachable

bb43:                                             ; preds = %entry.bb1_crit_edge.i
  br i1 undef, label %bb47, label %bb45.preheader

bb45.preheader:                                   ; preds = %bb43
  unreachable

bb47:                                             ; preds = %bb43
  br i1 undef, label %bb48, label %bb49

bb48:                                             ; preds = %bb47
  unreachable

bb49:                                             ; preds = %bb47
  %tmp336 = load %struct.XPVCV** %tmp26, align 8  ; <%struct.XPVCV*> [#uses=1]
  %tmp337 = getelementptr inbounds %struct.XPVCV* %tmp336, i64 0, i32 10 ; <void (%struct.CV*)**> [#uses=1]
  %tmp338 = load void (%struct.CV*)** %tmp337, align 8 ; <void (%struct.CV*)*> [#uses=1]
  call void %tmp338(%struct.CV* %tmp25) nounwind
  unreachable

bb51:                                             ; preds = %entry.bb1_crit_edge.i
  %tmp380 = bitcast i8* undef to %struct.SV**     ; <%struct.SV**> [#uses=2]
  br i1 undef, label %bb52, label %bb53

bb52:                                             ; preds = %bb51
  br label %bb53

bb53:                                             ; preds = %bb52, %bb51
  %tmp391 = getelementptr inbounds %struct.PERL_CONTEXT* %tmp90, i64 undef, i32 1, i32 0, i32 7, i32 0, i32 0 ; <i8**> [#uses=1]
  %.c = bitcast %struct.CV* %tmp25 to i8*         ; <i8*> [#uses=1]
  store i8* %.c, i8** %tmp391
  br i1 undef, label %bb83, label %bb57

bb57:                                             ; preds = %bb53
  br i1 undef, label %bb61, label %bb83

bb61:                                             ; preds = %bb57
  %tmp446 = getelementptr inbounds %struct.SV** %tmp380, i64 undef ; <%struct.SV**> [#uses=1]
  %tmp447 = load %struct.SV** %tmp446, align 8    ; <%struct.SV*> [#uses=1]
  %tmp448 = getelementptr inbounds %struct.SV* %tmp447, i64 0, i32 0 ; <i8**> [#uses=1]
  %tmp449 = load i8** %tmp448, align 8            ; <i8*> [#uses=1]
  %tmp450 = bitcast i8* %tmp449 to i8**           ; <i8**> [#uses=1]
  %tmp451 = load i8** %tmp450, align 8            ; <i8*> [#uses=1]
  br i1 undef, label %bb.nph28, label %bb79

bb.nph28:                                         ; preds = %bb61
  %scevgep66 = getelementptr i8* %tmp451, i64 undef ; <i8*> [#uses=0]
  unreachable

bb79:                                             ; preds = %bb61
  unreachable

bb83:                                             ; preds = %bb57, %bb53
  br i1 undef, label %bb.i25, label %entry.bb1_crit_edge.i23

entry.bb1_crit_edge.i23:                          ; preds = %bb83
  %.pre1.i22 = load %union.ANY** @PL_savestack, align 8 ; <%union.ANY*> [#uses=2]
  %tmp620 = load %struct.SV*** @PL_curpad, align 8 ; <%struct.SV**> [#uses=1]
  %tmp621 = getelementptr inbounds %union.ANY* %.pre1.i22, i64 undef, i32 0 ; <i8**> [#uses=1]
  %tmp622 = bitcast %struct.SV** %tmp620 to i8*   ; <i8*> [#uses=1]
  store i8* %tmp622, i8** %tmp621, align 8
  %tmp625 = getelementptr inbounds %union.ANY* %.pre1.i22, i64 undef, i32 0 ; <i8**> [#uses=1]
  store i8* bitcast (%struct.SV*** @PL_curpad to i8*), i8** %tmp625, align 8
  %tmp634 = getelementptr inbounds %struct.SV** %tmp380, i64 undef ; <%struct.SV**> [#uses=1]
  %tmp635 = load %struct.SV** %tmp634, align 8    ; <%struct.SV*> [#uses=1]
  %tmp636 = bitcast %struct.SV* %tmp635 to %struct.XPVAV** ; <%struct.XPVAV**> [#uses=1]
  %tmp637 = load %struct.XPVAV** %tmp636, align 8 ; <%struct.XPVAV*> [#uses=1]
  %tmp638 = getelementptr inbounds %struct.XPVAV* %tmp637, i64 0, i32 0 ; <i8**> [#uses=1]
  %tmp639 = load i8** %tmp638, align 8            ; <i8*> [#uses=1]
  %tmp640 = bitcast i8* %tmp639 to %struct.SV**   ; <%struct.SV**> [#uses=2]
  store %struct.SV** %tmp640, %struct.SV*** @PL_curpad, align 8
  %tmp646 = load %struct.SV** %tmp640, align 8    ; <%struct.SV*> [#uses=1]
  %tmp664 = getelementptr inbounds %struct.PERL_CONTEXT* %tmp90, i64 undef, i32 1, i32 0, i32 7, i32 0, i32 4 ; <%struct.OP**> [#uses=1]
  %.c4 = bitcast %struct.SV* %tmp646 to %struct.OP* ; <%struct.OP*> [#uses=1]
  store %struct.OP* %.c4, %struct.OP** %tmp664
  unreachable

bb.i25:                                           ; preds = %bb83
  unreachable

bb117:                                            ; preds = %bb1, %entry
  br i1 undef, label %bb119, label %bb118

bb118:                                            ; preds = %bb117
  br i1 undef, label %bb163, label %bb164

bb119:                                            ; preds = %bb117
  unreachable

bb163:                                            ; preds = %bb118
  br label %bb164

bb164:                                            ; preds = %bb163, %bb118
  ret %struct.OP* null
}

define fastcc void @Perl_newXS_SPEC3() nounwind ssp {
entry:
  br i1 undef, label %bb22, label %bb7

bb7:                                              ; preds = %entry
  br label %bb22

bb22:                                             ; preds = %bb7, %entry
  br i1 undef, label %bb1.i, label %bb.i

bb.i:                                             ; preds = %bb22
  br label %Perl_newSV.exit

bb1.i:                                            ; preds = %bb22
  br label %Perl_newSV.exit

Perl_newSV.exit:                                  ; preds = %bb1.i, %bb.i
  br i1 undef, label %bb31, label %bb34

bb31:                                             ; preds = %Perl_newSV.exit
  ret void

bb34:                                             ; preds = %Perl_newSV.exit
  unreachable
}

define fastcc void @Perl_newXS_SPEC4() nounwind ssp {
entry:
  %tmp53 = load %struct.SV** @PL_sv_root, align 8 ; <%struct.SV*> [#uses=1]
  br i1 undef, label %bb1.i, label %bb.i

bb.i:                                             ; preds = %entry
  %tmp64 = bitcast %struct.SV* %tmp53 to %struct.CV* ; <%struct.CV*> [#uses=1]
  %tmp76 = getelementptr inbounds %struct.CV* %tmp64, i64 0, i32 0 ; <%struct.XPVCV**> [#uses=1]
  br i1 undef, label %bb26, label %bb25

bb1.i:                                            ; preds = %entry
  unreachable

bb25:                                             ; preds = %bb.i
  br label %bb26

bb26:                                             ; preds = %bb25, %bb.i
  %tmp86 = load %struct.XPVCV** %tmp76, align 8   ; <%struct.XPVCV*> [#uses=1]
  %tmp87 = getelementptr inbounds %struct.XPVCV* %tmp86, i64 0, i32 10 ; <void (%struct.CV*)**> [#uses=1]
  store void (%struct.CV*)* @XS_MD5_digest, void (%struct.CV*)** %tmp87, align 8
  br i1 undef, label %bb30, label %bb28

bb28:                                             ; preds = %bb26
  br label %bb30

bb30:                                             ; preds = %bb28, %bb26
  unreachable
}

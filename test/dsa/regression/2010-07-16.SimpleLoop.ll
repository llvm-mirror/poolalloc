;dsaopt %s -dsa-eq -disable-output
; I have no idea, but as formulated above dsa works.  But compile it into
; .bc and then things happen.  Not going to investigate now.
;RUN: llvm-as %s -o - | dsaopt -dsa-eq -disable-output
; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-apple-darwin10.4"

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
%struct.__va_list_tag = type { i32, i32, i8*, i8* }
%struct._telldir = type opaque
%struct.io = type { %struct.XPVIO*, i32, i32 }
%struct.reg_data = type { i32, i8*, [1 x i8*] }
%struct.reg_substr_data = type { [3 x %struct.reg_substr_datum] }
%struct.reg_substr_datum = type { i32, i32, %struct.SV* }
%struct.regnode = type { i8, i8, i16 }
%union.ANY = type { i8* }

@PL_sv_undef = external global %struct.SV, align 16 ; <%struct.SV*> [#uses=1]
@PL_sv_no = external global %struct.SV, align 16  ; <%struct.SV*> [#uses=1]
@PL_sv_yes = external global %struct.SV, align 16 ; <%struct.SV*> [#uses=2]
@PL_lex_casestack = external global i8*           ; <i8**> [#uses=2]
@PL_sv_root = external global %struct.SV*         ; <%struct.SV**> [#uses=3]
@PL_savestack = external global %union.ANY*       ; <%union.ANY**> [#uses=2]

define fastcc i8* @Perl_scan_word(i8* %s, i8* %dest, i64 %destlen, i32 %allow_package, i64* nocapture %slp) nounwind ssp {
entry:
  unreachable
}

define fastcc void @Perl_yyerror(i8* %s) nounwind ssp {
entry:
  unreachable
}

define fastcc i32 @Perl_start_subparse(i32 %is_format, i32 %flags) nounwind ssp {
entry:
  unreachable
}

define fastcc void @Perl_check_uni() nounwind ssp {
entry:
  ret void
}

define fastcc %struct.SV* @new_constant(i8* %s, i64 %len, i8* %key, %struct.SV* %sv, %struct.SV* %pv, i8* %type) nounwind ssp {
entry:
  unreachable
}

define fastcc i8* @Perl_scan_num(i8* %start) nounwind ssp {
entry:
  unreachable
}

define fastcc i32 @Perl_keyword(i8* %d, i32 %len) nounwind ssp {
entry:
  unreachable
}

define fastcc void @Perl_force_ident(i8* %s, i32 %kind) nounwind ssp {
entry:
  ret void
}

define fastcc void @Perl_no_op(i8* %what, i8* %s) nounwind ssp {
entry:
  unreachable
}

define void @restore_rsfp(i8* %f) nounwind ssp {
entry:
  ret void
}

define fastcc void @Perl_checkcomma(i8* %s, i8* %what) nounwind ssp {
entry:
  unreachable
}

define fastcc %struct.SV* @tokeq(%struct.SV* %sv) nounwind ssp {
entry:
  unreachable
}

define fastcc i32 @sublex_start() nounwind ssp {
entry:
  unreachable
}

define fastcc void @incline(i8* %s) nounwind ssp {
entry:
  unreachable
}

define fastcc void @missingterm(i8* %s) nounwind ssp {
entry:
  unreachable
}

define fastcc i32 @Perl_filter_read(i32 %idx, %struct.SV* %buf_sv) nounwind ssp {
entry:
  unreachable
}

define fastcc i8* @filter_gets(%struct.SV* %sv, %struct.FILE* nocapture %fp) nounwind ssp {
entry:
  unreachable
}

define fastcc i8* @Perl_skipspace(i8* %s) nounwind ssp {
entry:
  unreachable
}

define fastcc i8* @Perl_scan_ident(i8* %s, i8* %send, i8* %dest, i64 %destlen, i32 %ck_uni) nounwind ssp {
entry:
  unreachable
}

define fastcc i32 @Perl_intuit_more(i8* %s) nounwind ssp {
entry:
  unreachable
}

define fastcc i32 @intuit_method(i8* %start, %struct.GV* %gv) nounwind ssp {
entry:
  unreachable
}

define fastcc i8* @Perl_scan_str(i8* %start) nounwind ssp {
entry:
  unreachable
}

define fastcc i8* @Perl_scan_pat(i8* %start, i32 %type) nounwind ssp {
entry:
  unreachable
}

define fastcc i8* @Perl_scan_trans(i8* %start) nounwind ssp {
entry:
  unreachable
}

define fastcc i8* @Perl_force_word(i8* %start, i32 %token, i32 %check_keyword, i32 %allow_pack, i32 %allow_initial_tick) nounwind ssp {
entry:
  unreachable
}

define fastcc i8* @force_version(i8* %s) nounwind ssp {
entry:
  unreachable
}

define fastcc i32 @lop(i32 %f, i32 %x, i8* %s) nounwind ssp {
entry:
  br i1 undef, label %bb, label %bb2

bb:                                               ; preds = %entry
  unreachable

bb2:                                              ; preds = %entry
  ret i32 286
}

define fastcc i32 @Perl_yylex() nounwind ssp {
entry:
  unreachable
}

define fastcc i32 @sublex_done() nounwind ssp {
entry:
  unreachable
}

define fastcc void @Perl_lex_start(%struct.SV* %line) nounwind ssp {
entry:
  br i1 undef, label %bb.i, label %entry.Perl_save_I32.exit_crit_edge

entry.Perl_save_I32.exit_crit_edge:               ; preds = %entry
  br i1 undef, label %bb.i2, label %Perl_save_I32.exit3

bb.i:                                             ; preds = %entry
  unreachable

bb.i2:                                            ; preds = %entry.Perl_save_I32.exit_crit_edge
  unreachable

Perl_save_I32.exit3:                              ; preds = %entry.Perl_save_I32.exit_crit_edge
  br i1 undef, label %bb.i5, label %Perl_save_I32.exit6

bb.i5:                                            ; preds = %Perl_save_I32.exit3
  unreachable

Perl_save_I32.exit6:                              ; preds = %Perl_save_I32.exit3
  br i1 undef, label %bb.i8, label %Perl_save_I32.exit9

bb.i8:                                            ; preds = %Perl_save_I32.exit6
  unreachable

Perl_save_I32.exit9:                              ; preds = %Perl_save_I32.exit6
  br i1 undef, label %bb.i11, label %Perl_save_I32.exit12

bb.i11:                                           ; preds = %Perl_save_I32.exit9
  br label %Perl_save_I32.exit12

Perl_save_I32.exit12:                             ; preds = %bb.i11, %Perl_save_I32.exit9
  br i1 undef, label %bb.i14, label %Perl_save_I32.exit15

bb.i14:                                           ; preds = %Perl_save_I32.exit12
  %tmp137 = bitcast i8* undef to %union.ANY*      ; <%union.ANY*> [#uses=2]
  store %union.ANY* %tmp137, %union.ANY** @PL_savestack, align 8
  br label %Perl_save_I32.exit15

Perl_save_I32.exit15:                             ; preds = %bb.i14, %Perl_save_I32.exit12
  %tmp139 = phi %union.ANY* [ undef, %Perl_save_I32.exit12 ], [ %tmp137, %bb.i14 ] ; <%union.ANY*> [#uses=2]
  br i1 undef, label %bb.i16, label %Perl_save_sptr.exit

bb.i16:                                           ; preds = %Perl_save_I32.exit15
  unreachable

Perl_save_sptr.exit:                              ; preds = %Perl_save_I32.exit15
  br i1 undef, label %bb.i18, label %Perl_save_I32.exit19

bb.i18:                                           ; preds = %Perl_save_sptr.exit
  unreachable

Perl_save_I32.exit19:                             ; preds = %Perl_save_sptr.exit
  %tmp203 = getelementptr inbounds %struct.COP* undef, i64 0, i32 13 ; <i16*> [#uses=1]
  br i1 undef, label %bb.i1, label %Perl_save_I16.exit

bb.i1:                                            ; preds = %Perl_save_I32.exit19
  unreachable

Perl_save_I16.exit:                               ; preds = %Perl_save_I32.exit19
  %tmp223 = getelementptr inbounds %union.ANY* %tmp139, i64 undef, i32 0 ; <i8**> [#uses=1]
  %tmp224 = bitcast i16* %tmp203 to i8*           ; <i8*> [#uses=1]
  store i8* %tmp224, i8** %tmp223, align 8
  br i1 undef, label %bb.i6, label %Perl_save_pptr.exit

bb.i6:                                            ; preds = %Perl_save_I16.exit
  unreachable

Perl_save_pptr.exit:                              ; preds = %Perl_save_I16.exit
  br i1 undef, label %bb.i10, label %Perl_save_pptr.exit11

bb.i10:                                           ; preds = %Perl_save_pptr.exit
  unreachable

Perl_save_pptr.exit11:                            ; preds = %Perl_save_pptr.exit
  br i1 false, label %bb.i15, label %Perl_save_pptr.exit16

bb.i15:                                           ; preds = %Perl_save_pptr.exit11
  unreachable

Perl_save_pptr.exit16:                            ; preds = %Perl_save_pptr.exit11
  br i1 undef, label %bb.i20, label %Perl_save_pptr.exit21

bb.i20:                                           ; preds = %Perl_save_pptr.exit16
  unreachable

Perl_save_pptr.exit21:                            ; preds = %Perl_save_pptr.exit16
  br i1 undef, label %bb.i26, label %Perl_save_pptr.exit27

bb.i26:                                           ; preds = %Perl_save_pptr.exit21
  unreachable

Perl_save_pptr.exit27:                            ; preds = %Perl_save_pptr.exit21
  br i1 undef, label %bb.i21, label %Perl_save_sptr.exit22

bb.i21:                                           ; preds = %Perl_save_pptr.exit27
  unreachable

Perl_save_sptr.exit22:                            ; preds = %Perl_save_pptr.exit27
  br i1 undef, label %bb.i32, label %Perl_save_pptr.exit33

bb.i32:                                           ; preds = %Perl_save_sptr.exit22
  unreachable

Perl_save_pptr.exit33:                            ; preds = %Perl_save_sptr.exit22
  br i1 undef, label %bb.i37, label %Perl_save_pptr.exit38

bb.i37:                                           ; preds = %Perl_save_pptr.exit33
  br label %Perl_save_pptr.exit38

Perl_save_pptr.exit38:                            ; preds = %bb.i37, %Perl_save_pptr.exit33
  %tmp404 = phi %union.ANY* [ undef, %bb.i37 ], [ %tmp139, %Perl_save_pptr.exit33 ] ; <%union.ANY*> [#uses=3]
  %tmp407 = load i8** @PL_lex_casestack, align 8  ; <i8*> [#uses=1]
  %tmp408 = getelementptr inbounds %union.ANY* %tmp404, i64 undef, i32 0 ; <i8**> [#uses=1]
  store i8* %tmp407, i8** %tmp408, align 8
  %tmp411 = getelementptr inbounds %union.ANY* %tmp404, i64 undef, i32 0 ; <i8**> [#uses=1]
  store i8* bitcast (i8** @PL_lex_casestack to i8*), i8** %tmp411, align 8
  br i1 undef, label %bb.i23, label %Perl_save_destructor.exit

bb.i23:                                           ; preds = %Perl_save_pptr.exit38
  unreachable

Perl_save_destructor.exit:                        ; preds = %Perl_save_pptr.exit38
  %tmp433 = getelementptr inbounds %union.ANY* %tmp404, i64 undef, i32 0 ; <i8**> [#uses=1]
  store i8* bitcast (void (i8*)* @restore_rsfp to i8*), i8** %tmp433
  br i1 undef, label %bb.i25, label %Perl_save_sptr.exit26

bb.i25:                                           ; preds = %Perl_save_destructor.exit
  unreachable

Perl_save_sptr.exit26:                            ; preds = %Perl_save_destructor.exit
  unreachable
}

define fastcc signext i8 @Perl_sv_derived_from(%struct.SV* %sv, i8* %name) nounwind ssp {
entry:
  br i1 undef, label %bb1, label %bb

bb:                                               ; preds = %entry
  unreachable

bb1:                                              ; preds = %entry
  br i1 undef, label %bb10, label %bb8

bb8:                                              ; preds = %bb1
  %tmp41 = call fastcc %struct.SV* @isa_lookup(%struct.HV* undef, i8* %name, i32 undef, i32 0) nounwind ssp ; <%struct.SV*> [#uses=0]
  ret i8 undef

bb10:                                             ; preds = %bb1
  ret i8 0
}

define void @XS_UNIVERSAL_isa(%struct.CV* nocapture %cv) nounwind ssp {
entry:
  br i1 undef, label %bb1, label %bb

bb:                                               ; preds = %entry
  unreachable

bb1:                                              ; preds = %entry
  br i1 undef, label %bb3, label %bb2

bb2:                                              ; preds = %bb1
  %tmp38 = call fastcc signext i8 @Perl_sv_derived_from(%struct.SV* undef, i8* undef) nounwind ; <i8> [#uses=0]
  %storemerge1 = select i1 undef, %struct.SV* @PL_sv_no, %struct.SV* @PL_sv_yes ; <%struct.SV*> [#uses=0]
  ret void

bb3:                                              ; preds = %bb1
  unreachable
}

define void @XS_UNIVERSAL_can(%struct.CV* nocapture %cv) nounwind ssp {
entry:
  unreachable
}

define void @XS_UNIVERSAL_VERSION(%struct.CV* nocapture %cv) nounwind ssp {
entry:
  unreachable
}

define fastcc %struct.SV* @isa_lookup(%struct.HV* %stash, i8* %name, i32 %len, i32 %level) nounwind ssp {
entry:
  br i1 undef, label %bb36, label %bb1

bb1:                                              ; preds = %entry
  br i1 undef, label %bb36, label %bb3

bb3:                                              ; preds = %bb1
  br i1 undef, label %bb4, label %bb5

bb4:                                              ; preds = %bb3
  unreachable

bb5:                                              ; preds = %bb3
  br i1 undef, label %bb32, label %bb12

bb12:                                             ; preds = %bb5
  br i1 undef, label %bb32, label %bb13

bb13:                                             ; preds = %bb12
  br i1 undef, label %bb32, label %bb14

bb14:                                             ; preds = %bb13
  br i1 undef, label %bb15, label %bb21

bb15:                                             ; preds = %bb14
  unreachable

bb21:                                             ; preds = %bb14
  br i1 undef, label %bb32, label %bb22

bb22:                                             ; preds = %bb21
  br i1 undef, label %bb31, label %bb24

bb24:                                             ; preds = %bb22
  br i1 false, label %bb27, label %bb28

bb27:                                             ; preds = %bb24
  unreachable

bb28:                                             ; preds = %bb24
  %tmp87 = call fastcc %struct.SV* @isa_lookup(%struct.HV* undef, i8* %name, i32 %len, i32 undef) nounwind ssp ; <%struct.SV*> [#uses=0]
  unreachable

bb31:                                             ; preds = %bb22
  unreachable

bb32:                                             ; preds = %bb21, %bb13, %bb12, %bb5
  ret %struct.SV* undef

bb36:                                             ; preds = %bb1, %entry
  %.0 = phi %struct.SV* [ @PL_sv_undef, %entry ], [ @PL_sv_yes, %bb1 ] ; <%struct.SV*> [#uses=1]
  ret %struct.SV* %.0
}

define fastcc i8* @Perl_screaminstr(%struct.SV* nocapture %bigstr, %struct.SV* nocapture %littlestr, i32 %start_shift, i32 %end_shift, i32* nocapture %old_posp, i32 %last) nounwind ssp {
entry:
  unreachable
}

define void @sig_trap(i32 %signo) nounwind ssp {
entry:
  ret void
}

define fastcc i8* @Perl_fbm_instr(i8* %big, i8* %bigend, %struct.SV* %littlestr) nounwind ssp {
entry:
  unreachable
}

define fastcc void @Perl_fbm_compile(%struct.SV* %sv) nounwind ssp {
entry:
  unreachable
}

define fastcc i8* @Perl_mess(i8* %pat, [1 x %struct.__va_list_tag]* %args) nounwind ssp {
entry:
  unreachable
}

define void @Perl_warn(i8* %pat, ...) nounwind ssp {
entry:
  br i1 undef, label %bb17, label %bb

bb:                                               ; preds = %entry
  br i1 undef, label %bb.i1, label %Perl_pop_scope.exit

bb.i1:                                            ; preds = %bb
  unreachable

Perl_pop_scope.exit:                              ; preds = %bb
  br i1 undef, label %bb17, label %bb5

bb5:                                              ; preds = %Perl_pop_scope.exit
  br i1 undef, label %bb6, label %bb17

bb6:                                              ; preds = %bb5
  br i1 undef, label %bb.i2, label %bb8.Perl_push_scope.exit3_crit_edge

bb8.Perl_push_scope.exit3_crit_edge:              ; preds = %bb6
  %tmp86 = load %struct.SV** @PL_sv_root, align 8 ; <%struct.SV*> [#uses=1]
  br i1 undef, label %bb1.i, label %bb.i5

bb.i2:                                            ; preds = %bb6
  unreachable

bb.i5:                                            ; preds = %bb8.Perl_push_scope.exit3_crit_edge
  br label %Perl_newSVpv.exit

bb1.i:                                            ; preds = %bb8.Perl_push_scope.exit3_crit_edge
  br label %Perl_newSVpv.exit

Perl_newSVpv.exit:                                ; preds = %bb1.i, %bb.i5
  %sv.0.i = phi %struct.SV* [ undef, %bb1.i ], [ %tmp86, %bb.i5 ] ; <%struct.SV*> [#uses=1]
  br i1 undef, label %bb.i4, label %Perl_push_scope.exit3.Perl_save_freesv.exit_crit_edge

Perl_push_scope.exit3.Perl_save_freesv.exit_crit_edge: ; preds = %Perl_newSVpv.exit
  %.pre14 = load %union.ANY** @PL_savestack, align 8 ; <%union.ANY*> [#uses=1]
  %tmp116 = getelementptr inbounds %union.ANY* %.pre14, i64 undef, i32 0 ; <i8**> [#uses=1]
  %tmp117 = bitcast %struct.SV* %sv.0.i to i8*    ; <i8*> [#uses=1]
  store i8* %tmp117, i8** %tmp116, align 8
  br i1 undef, label %bb9, label %bb10

bb.i4:                                            ; preds = %Perl_newSVpv.exit
  unreachable

bb9:                                              ; preds = %Perl_push_scope.exit3.Perl_save_freesv.exit_crit_edge
  unreachable

bb10:                                             ; preds = %Perl_push_scope.exit3.Perl_save_freesv.exit_crit_edge
  unreachable

bb17:                                             ; preds = %bb5, %Perl_pop_scope.exit, %entry
  ret void
}

define fastcc i64 @Perl_scan_hex(i8* %start, i32 %len, i32* nocapture %retlen) nounwind ssp {
entry:
  unreachable
}

define fastcc i64 @Perl_scan_oct(i8* nocapture %start, i32 %len, i32* nocapture %retlen) nounwind ssp {
entry:
  unreachable
}

define void @Perl_croak(i8* %pat, ...) nounwind ssp {
entry:
  unreachable
}

define fastcc i32 @Perl_wait4pid(i32 %pid, i32* %statusp, i32 %flags) nounwind ssp {
entry:
  unreachable
}

define fastcc i32 @Perl_my_pclose(%struct.FILE* nocapture %ptr) nounwind ssp {
entry:
  unreachable
}

define fastcc %struct.FILE* @Perl_my_popen(i8* %cmd, i8* %mode) nounwind ssp {
entry:
  unreachable
}

define %struct.OP* @Perl_die(i8* %pat, ...) nounwind ssp {
entry:
  unreachable
}

define i8* @Perl_form(i8* %pat, ...) nounwind ssp {
entry:
  unreachable
}

define fastcc i8* @Perl_savepvn(i8* %sv, i32 %len) nounwind ssp {
entry:
  unreachable
}

define fastcc i8* @Perl_savepv(i8* %sv) nounwind ssp {
entry:
  ret i8* undef
}

define fastcc i8* @Perl_saferealloc(i8* %where, i64 %size) nounwind ssp {
entry:
  unreachable
}

define fastcc void @Perl_my_setenv(i8* %nam, i8* %val) nounwind ssp {
entry:
  br i1 undef, label %Perl_setenv_getix.exit, label %bb.i5

bb.i5:                                            ; preds = %entry
  unreachable

Perl_setenv_getix.exit:                           ; preds = %entry
  unreachable
}

define void @Perl_magic_get_DIRECT(%struct.SV* %sv, %struct.MAGIC* nocapture %mg) nounwind ssp {
entry:
  unreachable
}

define %struct.OP* @Perl_ck_sort_DIRECT(%struct.OP* %o) nounwind ssp {
entry:
  unreachable
}

define %struct.OP* @Perl_ck_subr_DIRECT(%struct.OP* %o) nounwind ssp {
entry:
  unreachable
}

define %struct.OP* @Perl_ck_fun_DIRECT(%struct.OP* %o) nounwind ssp {
entry:
  ret %struct.OP* %o
}

define void @Perl_pp_anonlist_DIRECT() nounwind ssp {
entry:
  unreachable
}

define %struct.OP* @Perl_pp_entereval_DIRECT() nounwind ssp {
entry:
  unreachable
}

define %struct.OP* @Perl_pp_require_DIRECT() nounwind ssp {
entry:
  unreachable
}

define %struct.OP* @Perl_pp_goto_DIRECT() nounwind ssp {
entry:
  unreachable
}

define fastcc void @Perl_newXS_SPEC1() nounwind ssp {
entry:
  unreachable
}

define fastcc void @Perl_newXS_SPEC2() nounwind ssp {
entry:
  unreachable
}

define fastcc void @Perl_newXS_SPEC3() nounwind ssp {
entry:
  unreachable
}

define fastcc void @Perl_newXS_SPEC4() nounwind ssp {
entry:
  unreachable
}

define fastcc void @Perl_newXS_SPEC5() nounwind ssp {
entry:
  %tmp53 = load %struct.SV** @PL_sv_root, align 8 ; <%struct.SV*> [#uses=1]
  br i1 undef, label %bb1.i, label %bb.i

bb.i:                                             ; preds = %entry
  %tmp64 = bitcast %struct.SV* %tmp53 to %struct.CV* ; <%struct.CV*> [#uses=1]
  %tmp76 = getelementptr inbounds %struct.CV* %tmp64, i64 0, i32 0 ; <%struct.XPVCV**> [#uses=1]
  %tmp86 = load %struct.XPVCV** %tmp76, align 8   ; <%struct.XPVCV*> [#uses=1]
  %tmp87 = getelementptr inbounds %struct.XPVCV* %tmp86, i64 0, i32 10 ; <void (%struct.CV*)**> [#uses=1]
  store void (%struct.CV*)* @XS_UNIVERSAL_isa, void (%struct.CV*)** %tmp87, align 8
  unreachable

bb1.i:                                            ; preds = %entry
  unreachable
}

define fastcc void @Perl_newXS_SPEC7() nounwind ssp {
entry:
  %tmp53 = load %struct.SV** @PL_sv_root, align 8 ; <%struct.SV*> [#uses=1]
  br i1 undef, label %bb1.i, label %bb.i

bb.i:                                             ; preds = %entry
  %tmp64 = bitcast %struct.SV* %tmp53 to %struct.CV* ; <%struct.CV*> [#uses=1]
  %tmp76 = getelementptr inbounds %struct.CV* %tmp64, i64 0, i32 0 ; <%struct.XPVCV**> [#uses=1]
  %tmp86 = load %struct.XPVCV** %tmp76, align 8   ; <%struct.XPVCV*> [#uses=1]
  %tmp87 = getelementptr inbounds %struct.XPVCV* %tmp86, i64 0, i32 10 ; <void (%struct.CV*)**> [#uses=1]
  store void (%struct.CV*)* @XS_UNIVERSAL_VERSION, void (%struct.CV*)** %tmp87, align 8
  unreachable

bb1.i:                                            ; preds = %entry
  unreachable
}

define fastcc void @Perl_newXS_SPEC9() nounwind ssp {
entry:
  unreachable
}

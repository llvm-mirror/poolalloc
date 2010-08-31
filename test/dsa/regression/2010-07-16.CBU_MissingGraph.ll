;This test presently fails because cbu isn't robust to callee's not being in
; the globals graph... which happens all the time--direct call's callees
; don't get entries, for example.
;--verify this works without filtering
;RUN: dsaopt %s -dsa-cbu -disable-output \
;RUN: -dsa-no-filter-callcc=true \
;RUN: -dsa-no-filter-vararg=true \
;RUN: -dsa-no-filter-intfp=true
;--verify this works with filtering
;RUN: dsaopt %s -dsa-cbu -disable-output
; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-apple-darwin10.4"

%struct.OP = type { %struct.OP*, %struct.OP*, %struct.OP* ()*, i32, i16, i16, i8, i8 }
%struct.REGEXP = type { i32, i8**, i8**, %struct.regnode*, i32, i32, i32, i32, i8*, i8*, i8*, i8*, i16, i16, %struct.reg_substr_data*, %struct.reg_data*, [1 x %struct.regnode] }
%struct.SV = type { i8*, i32, i32 }
%struct.__va_list_tag = type { i32, i32, i8*, i8* }
%struct.reg_data = type { i32, i8*, [1 x i8*] }
%struct.reg_substr_data = type { [3 x %struct.reg_substr_datum] }
%struct.reg_substr_datum = type { i32, i32, %struct.SV* }
%struct.regnode = type { i8, i8, i16 }

@.str1239 = external constant [1 x i8], align 1   ; <[1 x i8]*> [#uses=1]
@PL_sv_undef = external global %struct.SV, align 16 ; <%struct.SV*> [#uses=1]

define %struct.SV* @Perl_amagic_call(%struct.SV* %left, %struct.SV* %right, i32 %method, i32 %flags) nounwind ssp {
entry:
  call void @Perl_sv_setsv(%struct.SV* %left, %struct.SV* undef) nounwind
  ret %struct.SV* %left
}

define %struct.OP* @Perl_pp_sort() nounwind ssp {
entry:
  %storemerge1 = select i1 undef, i32 (%struct.SV*, %struct.SV*)* @Perl_sv_cmp, i32 (%struct.SV*, %struct.SV*)* @amagic_cmp ; <i32 (%struct.SV*, %struct.SV*)*> [#uses=1]
  %0 = call i32 %storemerge1(%struct.SV* undef, %struct.SV* undef) nounwind ; <i32> [#uses=0]
  unreachable
}

declare i32 @amagic_cmp(%struct.SV*, %struct.SV*) nounwind ssp

define i8* @Perl_sv_2pv(%struct.SV* %sv, i64* %lp) nounwind ssp {
entry:
  %0 = call %struct.SV* @Perl_amagic_call(%struct.SV* %sv, %struct.SV* @PL_sv_undef, i32 4, i32 9) nounwind ; <%struct.SV*> [#uses=0]
  unreachable
}

define i32 @Perl_sv_cmp(%struct.SV* %str1, %struct.SV* %str2) nounwind ssp {
entry:
  %0 = call i8* @Perl_sv_2pv(%struct.SV* %str1, i64* undef) nounwind ssp ; <i8*> [#uses=0]
  unreachable
}

define void @Perl_sv_setsv(%struct.SV* %dstr, %struct.SV* %sstr) nounwind ssp {
entry:
  %0 = tail call i32 @Perl_sv_cmp(%struct.SV* undef, %struct.SV* undef) nounwind ssp ; <i32> [#uses=0]
  unreachable
}


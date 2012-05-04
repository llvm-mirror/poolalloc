;RUN: dsaopt %s -dsa-td -analyze -check-callees=main,A
;RUN: dsaopt %s -dsa-td -analyze -check-not-callees=main,B
; We don't filter potential callees based on float/double type mismatches.
;XFAIL: *
; ModuleID = 'calltargets.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

@.str = private constant [4 x i8] c"%f\0A\00", align 1 ; <[4 x i8]*> [#uses=1]
@.str1 = private constant [4 x i8] c"%d\0A\00", align 1 ; <[4 x i8]*> [#uses=1]
@FP = common global i32* (double)* null           ; <i32* (double)**> [#uses=3]

define internal i32* @B(float %f) nounwind {
entry:
  %f_addr = alloca float                          ; <float*> [#uses=2]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store float %f, float* %f_addr
  %1 = load float* %f_addr, align 4               ; <float> [#uses=1]
  %2 = fpext float %1 to double                   ; <double> [#uses=1]
  %3 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([4 x i8]* @.str, i64 0, i64 0), double %2) nounwind ; <i32> [#uses=0]
  store i32* null, i32** %0, align 8
  %4 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %4, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

declare i32 @printf(i8* noalias, ...) nounwind

define internal i32* @A(double %d) nounwind {
entry:
  %d_addr = alloca double                         ; <double*> [#uses=2]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store double %d, double* %d_addr
  %1 = load double* %d_addr, align 8              ; <double> [#uses=1]
  %2 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([4 x i8]* @.str1, i64 0, i64 0), double %1) nounwind ; <i32> [#uses=0]
  store i32* null, i32** %0, align 8
  %3 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %3, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define void @main() nounwind {
entry:
  %d = alloca double                              ; <double*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* (double)* @A, i32* (double)** @FP, align 8
  store i32* (double)* bitcast (i32* (float)* @B to i32* (double)*), i32* (double)** @FP, align 8
  store double 5.000000e+00, double* %d, align 8
  %0 = load i32* (double)** @FP, align 8          ; <i32* (double)*> [#uses=1]
  %1 = load double* %d, align 8                   ; <double> [#uses=1]
  %2 = call i32* %0(double %1) nounwind           ; <i32*> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  ret void
}

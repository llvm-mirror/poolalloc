;RUN: dsaopt %s -dsa-bu -analyze -check-callees=main,init
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=init,A,B,SetFP
;XFAIL: *

; ModuleID = 'loop2.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

@FP = common global i32* (i32*)* null             ; <i32* (i32*)**> [#uses=4]

define i32* @B() nounwind {
entry:
  %retval = alloca i32*                           ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define i32* @A() nounwind {
entry:
  %retval = alloca i32*                           ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define void @D(i32* (i32*)* %f) nounwind {
entry:
  %f_addr = alloca i32* (i32*)*                   ; <i32* (i32*)**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* (i32*)* %f, i32* (i32*)** %f_addr
  store i32* (i32*)* bitcast (i32* ()* @B to i32* (i32*)*), i32* (i32*)** %f_addr, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

define i32* @SetFP(i8* %f) nounwind {
entry:
  %f_addr = alloca i8*                            ; <i8**> [#uses=1]
  %retval = alloca i32*                           ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i8* %f, i8** %f_addr
  %0 = load i32* (i32*)** @FP, align 8            ; <i32* (i32*)*> [#uses=1]
  call void @D(i32* (i32*)* %0) nounwind
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define i32* @init() nounwind {
entry:
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %setter = alloca i32* (i8*)*                    ; <i32* (i8*)**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* (i32*)* bitcast (i32* ()* @A to i32* (i32*)*), i32* (i32*)** @FP, align 8
  store i32* (i8*)* @SetFP, i32* (i8*)** %setter, align 8
  %1 = call noalias i8* @malloc(i64 3) nounwind   ; <i8*> [#uses=1]
  %2 = load i32* (i8*)** %setter, align 8         ; <i32* (i8*)*> [#uses=1]
  %3 = call i32* %2(i8* %1) nounwind              ; <i32*> [#uses=0]
  %4 = load i32* (i32*)** @FP, align 8            ; <i32* (i32*)*> [#uses=1]
  %5 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %6 = bitcast i8* %5 to i32*                     ; <i32*> [#uses=1]
  %7 = call i32* %4(i32* %6) nounwind             ; <i32*> [#uses=0]
  %8 = load i32* (i32*)** @FP, align 8            ; <i32* (i32*)*> [#uses=1]
  %9 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %10 = bitcast i8* %9 to i32*                    ; <i32*> [#uses=1]
  %11 = call i32* %8(i32* %10) nounwind           ; <i32*> [#uses=1]
  store i32* %11, i32** %0, align 8
  %12 = load i32** %0, align 8                    ; <i32*> [#uses=1]
  store i32* %12, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

declare noalias i8* @malloc(i64) nounwind

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call i32* @init() nounwind                 ; <i32*> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

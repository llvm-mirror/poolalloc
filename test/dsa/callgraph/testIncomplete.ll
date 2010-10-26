; another cause of incompleteness
; fptr, and arg both point to same node
; and hence incomplete

;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=A,B,C,D
;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=B,B,C,D

; ModuleID = 'testIncomplete.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32* @B(i8* %fp) nounwind {
entry:
  %fp_addr = alloca i8*                           ; <i8**> [#uses=2]
  %retval = alloca i32*                           ; <i32**> [#uses=1]
  %x = alloca i32*                                ; <i32**> [#uses=2]
  %fp1 = alloca i32* (i8*)**                      ; <i32* (i8*)***> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i8* %fp, i8** %fp_addr
  %0 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  store i32* %1, i32** %x, align 8
  %2 = load i8** %fp_addr, align 8                ; <i8*> [#uses=1]
  %3 = bitcast i8* %2 to i32* (i8*)**             ; <i32* (i8*)**> [#uses=1]
  store i32* (i8*)** %3, i32* (i8*)*** %fp1, align 8
  %4 = load i32* (i8*)*** %fp1, align 8           ; <i32* (i8*)**> [#uses=1]
  %5 = load i32* (i8*)** %4, align 8              ; <i32* (i8*)*> [#uses=1]
  %6 = load i32** %x, align 8                     ; <i32*> [#uses=1]
  %7 = bitcast i32* %6 to i8*                     ; <i8*> [#uses=1]
  %8 = call i32* %5(i8* %7) nounwind              ; <i32*> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

declare noalias i8* @malloc(i64) nounwind

define i32* @C(i8* %fp1) nounwind {
entry:
  %fp1_addr = alloca i8*                          ; <i8**> [#uses=1]
  %retval = alloca i32*                           ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i8* %fp1, i8** %fp1_addr
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define i32* @D(i8* %fp1) nounwind {
entry:
  %fp1_addr = alloca i8*                          ; <i8**> [#uses=1]
  %retval = alloca i32*                           ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i8* %fp1, i8** %fp1_addr
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define i32* @A() nounwind {
entry:
  %retval = alloca i32*                           ; <i32**> [#uses=1]
  %fp = alloca i32* (i8*)*                        ; <i32* (i8*)**> [#uses=4]
  %fp1 = alloca i32* (i8*)*                       ; <i32* (i8*)**> [#uses=4]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* (i8*)* @B, i32* (i8*)** %fp, align 8
  store i32* (i8*)* @C, i32* (i8*)** %fp, align 8
  store i32* (i8*)* @D, i32* (i8*)** %fp, align 8
  store i32* (i8*)* @B, i32* (i8*)** %fp1, align 8
  store i32* (i8*)* @C, i32* (i8*)** %fp1, align 8
  store i32* (i8*)* @D, i32* (i8*)** %fp1, align 8
  %0 = load i32* (i8*)** %fp, align 8             ; <i32* (i8*)*> [#uses=1]
  %1 = load i32* (i8*)** %fp1, align 8            ; <i32* (i8*)*> [#uses=1]
  %2 = bitcast i32* (i8*)* %1 to i8*              ; <i8*> [#uses=1]
  %3 = call i32* %0(i8* %2) nounwind              ; <i32*> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

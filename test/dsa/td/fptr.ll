
;RUN: dsaopt %s -dsa-eq -analyze 

; ModuleID = 'multi_level.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32* @foo1(i32* %a) nounwind {
entry:
  %a_addr = alloca i32*                           ; <i32**> [#uses=2]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* %a, i32** %a_addr
  %1 = load i32** %a_addr, align 8                ; <i32*> [#uses=1]
  store i32* %1, i32** %0, align 8
  %2 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %2, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define i32* @foo2(i32* %a) nounwind {
entry:
  %a_addr = alloca i32*                           ; <i32**> [#uses=2]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* %a, i32** %a_addr
  %1 = load i32** %a_addr, align 8                ; <i32*> [#uses=1]
  store i32* %1, i32** %0, align 8
  %2 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %2, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define i32* @foo3(i32* %a) nounwind {
entry:
  %a_addr = alloca i32*                           ; <i32**> [#uses=2]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* %a, i32** %a_addr
  %1 = load i32** %a_addr, align 8                ; <i32*> [#uses=1]
  store i32* %1, i32** %0, align 8
  %2 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %2, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define i32* @foo4(i32* %a) nounwind {
entry:
  %a_addr = alloca i32*                           ; <i32**> [#uses=2]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* %a, i32** %a_addr
  %1 = load i32** %a_addr, align 8                ; <i32*> [#uses=1]
  store i32* %1, i32** %0, align 8
  %2 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %2, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define i32* @bar1(i32* (i32*)* %fp, i32* %b) nounwind {
entry:
  %fp_addr = alloca i32* (i32*)*                  ; <i32* (i32*)**> [#uses=2]
  %b_addr = alloca i32*                           ; <i32**> [#uses=2]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* (i32*)* %fp, i32* (i32*)** %fp_addr
  store i32* %b, i32** %b_addr
  %1 = load i32* (i32*)** %fp_addr, align 8       ; <i32* (i32*)*> [#uses=1]
  %2 = load i32** %b_addr, align 8                ; <i32*> [#uses=1]
  %3 = call i32* %1(i32* %2) nounwind             ; <i32*> [#uses=1]
  store i32* %3, i32** %0, align 8
  %4 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %4, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define i32* @zar1(i32* (i32*)* %fp1, i32* (i32*)* %fp2, i32* %c) nounwind {
entry:
  %fp1_addr = alloca i32* (i32*)*                 ; <i32* (i32*)**> [#uses=2]
  %fp2_addr = alloca i32* (i32*)*                 ; <i32* (i32*)**> [#uses=2]
  %c_addr = alloca i32*                           ; <i32**> [#uses=4]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* (i32*)* %fp1, i32* (i32*)** %fp1_addr
  store i32* (i32*)* %fp2, i32* (i32*)** %fp2_addr
  store i32* %c, i32** %c_addr
  %1 = load i32** %c_addr, align 8                ; <i32*> [#uses=1]
  %2 = load i32* %1, align 4                      ; <i32> [#uses=1]
  %3 = icmp sgt i32 %2, 10                        ; <i1> [#uses=1]
  br i1 %3, label %bb, label %bb1

bb:                                               ; preds = %entry
  %4 = load i32* (i32*)** %fp1_addr, align 8      ; <i32* (i32*)*> [#uses=1]
  %5 = load i32** %c_addr, align 8                ; <i32*> [#uses=1]
  %6 = call i32* @bar1(i32* (i32*)* %4, i32* %5) nounwind ; <i32*> [#uses=1]
  store i32* %6, i32** %0, align 8
  br label %bb2

bb1:                                              ; preds = %entry
  %7 = load i32* (i32*)** %fp2_addr, align 8      ; <i32* (i32*)*> [#uses=1]
  %8 = load i32** %c_addr, align 8                ; <i32*> [#uses=1]
  %9 = call i32* @bar1(i32* (i32*)* %7, i32* %8) nounwind ; <i32*> [#uses=1]
  store i32* %9, i32** %0, align 8
  br label %bb2

bb2:                                              ; preds = %bb1, %bb
  %10 = load i32** %0, align 8                    ; <i32*> [#uses=1]
  store i32* %10, i32** %retval, align 8
  br label %return

return:                                           ; preds = %bb2
  %retval3 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval3
}

define void @main() nounwind {
entry:
  %0 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  %2 = call i32* @zar1(i32* (i32*)* @foo1, i32* (i32*)* @foo2, i32* %1) nounwind ; <i32*> [#uses=0]
  %3 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %4 = bitcast i8* %3 to i32*                     ; <i32*> [#uses=1]
  %5 = call i32* @zar1(i32* (i32*)* @foo3, i32* (i32*)* @foo4, i32* %4) nounwind ; <i32*> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  ret void
}

declare noalias i8* @malloc(i64) nounwind

;RUN: dsaopt %s -dsa-bu -analyze -check-callees=main,init,init1
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=init,A,B,malloc
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=init1,init2
;XFAIL: *

; ModuleID = 'loop1.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.S = type { i32* (i32*)* }

@FP = common global i32* (i32*)* null             ; <i32* (i32*)**> [#uses=5]

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

define internal i32* @init() nounwind {
entry:
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* (i32*)* bitcast (i32* ()* @A to i32* (i32*)*), i32* (i32*)** @FP, align 8
  %1 = load i32* (i32*)** @FP, align 8            ; <i32* (i32*)*> [#uses=1]
  %2 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %3 = bitcast i8* %2 to i32*                     ; <i32*> [#uses=1]
  %4 = call i32* %1(i32* %3) nounwind             ; <i32*> [#uses=0]
  %5 = load i32* (i32*)** @FP, align 8            ; <i32* (i32*)*> [#uses=1]
  %6 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %7 = bitcast i8* %6 to i32*                     ; <i32*> [#uses=1]
  %8 = call i32* %5(i32* %7) nounwind             ; <i32*> [#uses=1]
  store i32* %8, i32** %0, align 8
  %9 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %9, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

declare noalias i8* @malloc(i64) nounwind

define void @init2(%struct.S* %o) nounwind {
entry:
  %o_addr = alloca %struct.S*                     ; <%struct.S**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.S* %o, %struct.S** %o_addr
  %0 = load %struct.S** %o_addr, align 8          ; <%struct.S*> [#uses=1]
  %1 = getelementptr inbounds %struct.S* %0, i32 0, i32 0 ; <i32* (i32*)**> [#uses=1]
  store i32* (i32*)* bitcast (i32* ()* @B to i32* (i32*)*), i32* (i32*)** %1, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @init1() nounwind {
entry:
  %t = alloca %struct.S*                          ; <%struct.S**> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call noalias i8* @malloc(i64 8) nounwind   ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to %struct.S*               ; <%struct.S*> [#uses=1]
  store %struct.S* %1, %struct.S** %t, align 8
  %2 = load i32* (i32*)** @FP, align 8            ; <i32* (i32*)*> [#uses=1]
  %3 = load %struct.S** %t, align 8               ; <%struct.S*> [#uses=1]
  %4 = getelementptr inbounds %struct.S* %3, i32 0, i32 0 ; <i32* (i32*)**> [#uses=1]
  store i32* (i32*)* %2, i32* (i32*)** %4, align 8
  %5 = load %struct.S** %t, align 8               ; <%struct.S*> [#uses=1]
  call void @init2(%struct.S* %5) nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call i32* @init() nounwind                 ; <i32*> [#uses=0]
  call void @init1() nounwind
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

; ModuleID = 'scc3b.o'
;RUN: dsaopt %s -dsa-bu -disable-output
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define internal i32* @func(i32* %arg) nounwind {
entry:
  %arg_addr = alloca i32*                         ; <i32**> [#uses=2]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* %arg, i32** %arg_addr
  %1 = call i32* @A() nounwind                    ; <i32*> [#uses=0]
  %2 = load i32** %arg_addr, align 8              ; <i32*> [#uses=1]
  %3 = call i32* @C(i32* (i32*)* null, i32* %2) nounwind ; <i32*> [#uses=1]
  store i32* %3, i32** %0, align 8
  %4 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %4, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define internal i32* @C(i32* (i32*)* %f, i32* %arg) nounwind {
entry:
  %f_addr = alloca i32* (i32*)*                   ; <i32* (i32*)**> [#uses=2]
  %arg_addr = alloca i32*                         ; <i32**> [#uses=2]
  %retval = alloca i32*                           ; <i32**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32* (i32*)* %f, i32* (i32*)** %f_addr
  store i32* %arg, i32** %arg_addr
  %0 = call i32* @A() nounwind                    ; <i32*> [#uses=0]
  %1 = call i32* @B() nounwind                    ; <i32*> [#uses=1]
  %2 = call i32* @func(i32* %1) nounwind          ; <i32*> [#uses=0]
  %3 = load i32* (i32*)** %f_addr, align 8        ; <i32* (i32*)*> [#uses=1]
  %4 = load i32** %arg_addr, align 8              ; <i32*> [#uses=1]
  %5 = call i32* %3(i32* %4) nounwind             ; <i32*> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define internal i32* @B() nounwind {
entry:
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = call i32* @A() nounwind                    ; <i32*> [#uses=1]
  %2 = call i32* @func(i32* %1) nounwind          ; <i32*> [#uses=0]
  %3 = call i32* @A() nounwind                    ; <i32*> [#uses=1]
  %4 = call i32* @C(i32* (i32*)* @func, i32* %3) nounwind ; <i32*> [#uses=1]
  store i32* %4, i32** %0, align 8
  %5 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %5, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define internal i32* @A() nounwind {
entry:
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = call i32* @B() nounwind                    ; <i32*> [#uses=1]
  %2 = call i32* @func(i32* %1) nounwind          ; <i32*> [#uses=1]
  %3 = call i32* @C(i32* (i32*)* null, i32* %2) nounwind ; <i32*> [#uses=1]
  store i32* %3, i32** %0, align 8
  %4 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %4, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

define internal i32* @D() nounwind {
entry:
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = call i32* @A() nounwind                    ; <i32*> [#uses=1]
  store i32* %1, i32** %0, align 8
  %2 = load i32** %0, align 8                     ; <i32*> [#uses=1]
  store i32* %2, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

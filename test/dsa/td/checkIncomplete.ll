;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "print:buffer+I"
;RUN: dsaopt %s -dsa-stdlib -analyze -verify-flags "@buf-I"

;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "print:buffer+I"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "@buf-I"

;RUN: dsaopt %s -dsa-td -analyze -verify-flags "print:buffer-I"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "@buf-I"

; global buf, is passed as argument to print. It should not be marked incomplete after td

; ModuleID = 'checkIncomplete.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

@buf = internal global [30 x i8] zeroinitializer, align 16 ; <[30 x i8]*> [#uses=1]

define internal void @print(i8* %buffer) nounwind {
entry:
  %buffer_addr = alloca i8*                       ; <i8**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i8* %buffer, i8** %buffer_addr
  %0 = load i8** %buffer_addr, align 8            ; <i8*> [#uses=1]
  %1 = getelementptr inbounds i8* %0, i64 0       ; <i8*> [#uses=1]
  store i8 97, i8* %1, align 1
  br label %return

return:                                           ; preds = %entry
  ret void
}

define void @main() nounwind {
entry:
  call void @print(i8* getelementptr inbounds ([30 x i8]* @buf, i64 0, i64 0)) nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

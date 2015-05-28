;RUN: dsaopt %s -dsa-td -analyze -check-callees=run_func,f1
;RUN: dsaopt %s -dsa-td -analyze -check-not-callees=run_func,main

;XFAIL: *
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [10 x i8] c"Main: %p\0A\00", align 1
@.str1 = private unnamed_addr constant [10 x i8] c"Sum:  %d\0A\00", align 1

; Function Attrs: nounwind uwtable
define internal i32 @main(i32 %argc, i8** %argv) #0 {
entry:
  %call = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @.str, i32 0, i32 0), i32 (i32, i8**)* @main)
  %call1 = call i32 @foo()
  %call2 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([10 x i8], [10 x i8]* @.str1, i32 0, i32 0), i32 %call1)
  ret i32 0
}

declare i32 @printf(i8*, ...)

; Function Attrs: noinline nounwind uwtable
define internal i32 @foo() #1 {
entry:
  %call = call i32 @run_func(i32 (i32, i32)* @f1, i32 1, i32 2)
  ret i32 %call
}

; Function Attrs: noinline nounwind uwtable
define internal i32 @run_func(i32 (i32, i32)* %fptr, i32 %arg1, i32 %arg2) #1 {
entry:
  %call = call i32 %fptr(i32 %arg1, i32 %arg2)
  ret i32 %call
}

; Function Attrs: noinline nounwind uwtable
define internal i32 @f1(i32 %arg1, i32 %arg2) #1 {
entry:
  %add = add nsw i32 %arg1, %arg2
  ret i32 %add
}

attributes #0 = { nounwind uwtable }
attributes #1 = { noinline nounwind uwtable }

; Don't die if we don't know the callees
; RUN: adsaopt -disable-output -dsnodeequivs %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

@FP = external global i64 ()*

define i64 @main(i32 %argc, i8** %argv) uwtable {
entry:
  %fptr = load i64()** @FP
  %0 = call i64 %fptr()
  ret i64 %0
}

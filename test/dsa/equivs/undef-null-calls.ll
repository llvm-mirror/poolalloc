; Test call/invoke to null and undef (bugpoint)
; RUN: adsaopt -disable-output -dsnodeequivs %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
declare i32 @__gxx_personality_v0(...)

define i64 @main(i32 %argc, i8** %argv) uwtable {
entry:
  %0 = invoke i64 undef(i64 0)
                 to label %cont unwind label %lpad

cont:
  %1 = invoke i64 null(i64 %0)
                 to label %done unwind label %lpad

lpad:
  %2 = landingpad {i8*, i32} personality i32 (...)* @__gxx_personality_v0
          cleanup
  br label %done

done:
  %retval = phi i64 [0, %lpad], [%1, %cont]
  %test = call i64 undef(i64 %retval)
  %test2 = call i64 null(i64 %test)
  ret i64 %retval
}

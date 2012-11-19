; PR12786
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

declare i8* @getcwd(i8*) nounwind

; Result of getcwd should be merged with its argument
; RUN: dsaopt %s -dsa-stdlib -analyze -check-same-node=test_cwd:ret,test_cwd:arg
define i8* @test_cwd(i8* %arg) nounwind {
  %ret = call i8* @getcwd(i8* %arg) nounwind
  ret i8* %ret
}

declare i8* @strstr(i8*,i8*) nounwind

; Result of strstr should be merged with its first argument, second should be separate
; RUN: dsaopt %s -dsa-stdlib -analyze -check-same-node=test_strstr:ret,test_strstr:arg1
; RUN: dsaopt %s -dsa-stdlib -analyze -check-not-same-node=test_strstr:ret,test_strstr:arg2
; RUN: dsaopt %s -dsa-stdlib -analyze -check-not-same-node=test_strstr:arg1,test_strstr:arg2
define i8* @test_strstr(i8* %arg1, i8* %arg2) nounwind {
  %ret = call i8* @strstr(i8* %arg1, i8* %arg2) nounwind
  ret i8* %ret
}

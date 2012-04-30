; Global initializers cause node for global to collapse before processing it
; Reduced in spirit from 403.gcc
;RUN: dsaopt %s -dsa-local -disable-output
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.IntPair = type { i32, i32 }

@ptr = global i8* getelementptr (i8* bitcast (i32* getelementptr inbounds (%struct.IntPair* @ip, i64 0, i32 1) to i8*), i64 2), align 8
@ip = global %struct.IntPair { i32 5, i32 10 }, align 4

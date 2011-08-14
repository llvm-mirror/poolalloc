; RUN: dsaopt %s -dsa-local -analyze -check-same-node=test:P,test:Q
; RUN: llvm-as %s -o /dev/null
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define void @test(i32* %P, i32* %Q) {
entry:
        %tmp.1 = bitcast i32* %P to i8*         ; <i8*> [#uses=3]
        %tmp.3 = bitcast i32* %Q to i8*         ; <i8*> [#uses=4]
        tail call void @llvm.memcpy.p0i8.p0i8.i32( i8* %tmp.1, i8* %tmp.3, i32 100000, i32 0, i1 1 )
        tail call void @llvm.memcpy.p0i8.p0i8.i64( i8* %tmp.1, i8* %tmp.3, i64 100000, i32 0, i1 1 )
        tail call void @llvm.memset.p0i8.i32( i8* %tmp.3, i8 14, i32 10000, i32 0, i1 0 )
        tail call void @llvm.memmove.p0i8.p0i8.i32( i8* %tmp.1, i8* %tmp.3, i32 123124, i32 0, i1 1 )
        ret void
}

declare void @llvm.memcpy.p0i8.p0i8.i32(i8*, i8*, i32, i32, i1)

declare void @llvm.memcpy.p0i8.p0i8.i64(i8*, i8*, i64, i32, i1)

declare void @llvm.memset.p0i8.i32(i8*, i8, i32, i32, i1)

declare void @llvm.memmove.p0i8.p0i8.i32(i8*, i8*, i32, i32, i1)


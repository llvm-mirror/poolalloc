; ModuleID = 'foo.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128"
target triple = "x86_64-unknown-linux-gnu"
	%struct.pa = type <{ i8, i32*, i8 }>
@.str = private constant [25 x i8] c"Hello World %d %d %s %s\0A\00", align 1		; <[25 x i8]*> [#uses=1]
@.str1 = private constant [22 x i8] c"Hello World %d %d %s\0A\00", align 1		; <[22 x i8]*> [#uses=1]

define void @foo(i32* nocapture %y) nounwind noinline {
entry:
	store i32 3, i32* %y, align 4
	ret void
}

define void @bar(%struct.pa* nocapture %S, i32* %v) nounwind {
entry:
	%0 = getelementptr %struct.pa* %S, i64 0, i32 0		; <i8*> [#uses=1]
	store i8 0, i8* %0, align 1
	%1 = getelementptr %struct.pa* %S, i64 0, i32 1		; <i32**> [#uses=1]
	store i32* %v, i32** %1, align 1
	%2 = getelementptr %struct.pa* %S, i64 0, i32 2		; <i8*> [#uses=1]
	store i8 0, i8* %2, align 1
	ret void
}

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
	%0 = malloc i32		; <i32*> [#uses=2]
	store i32 %argc, i32* %0, align 8
	tail call void @foo(i32* %0) nounwind noinline
	%1 = getelementptr i8** %argv, i64 1		; <i8**> [#uses=1]
	%2 = load i8** %1, align 8		; <i8*> [#uses=1]
	%3 = tail call i32 (i8*, ...)* @printf(i8* noalias getelementptr ([25 x i8]* @.str, i64 0, i64 0), i32 %argc, i32 2, i8* %2) nounwind		; <i32> [#uses=0]
	%4 = load i8** %argv, align 8		; <i8*> [#uses=1]
	%5 = tail call i32 (i8*, ...)* @printf(i8* noalias getelementptr ([22 x i8]* @.str1, i64 0, i64 0), i32 %argc, i32 2, i8* %4) nounwind		; <i32> [#uses=0]
	ret i32 %argc
}

declare i32 @printf(i8* nocapture, ...) nounwind

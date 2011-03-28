; ModuleID = 'tt.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
;RUN: dsaopt %s -dsa-local -disable-output
;RUN: dsaopt %s -dsa-bu -disable-output
;RUN: dsaopt %s -dsa-td -disable-output
;RUN: dsaopt %s -dsa-eq -disable-output

%struct.__va_list_tag = type { i32, i32, i8*, i8* }

@.str = private constant [22 x i8] c"F %li %li %3.2f %3.2f\00", align 1 ; <[22 x i8]*> [#uses=1]
@.str1 = private constant [12 x i8] c"%s ID3:%s%s\00", align 1 ; <[12 x i8]*> [#uses=1]
@.str2 = private constant [5 x i8] c"TEST\00", align 1 ; <[5 x i8]*> [#uses=1]
@.str3 = private constant [4 x i8] c"AAA\00", align 1 ; <[4 x i8]*> [#uses=1]
@.str4 = private constant [8 x i8] c"Unknown\00", align 1 ; <[8 x i8]*> [#uses=1]
@.str5 = private constant [15 x i8] c"%s ID3:%s%s %p\00", align 1 ; <[15 x i8]*> [#uses=1]

define internal void @generic_sendmsg(i8* %fmt, ...) nounwind {
entry:
  %fmt_addr = alloca i8*                          ; <i8**> [#uses=2]
  %ap = alloca [1 x %struct.__va_list_tag]        ; <[1 x %struct.__va_list_tag]*> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i8* %fmt, i8** %fmt_addr
  %0 = call i32 @putchar(i32 64) nounwind         ; <i32> [#uses=0]
  %ap1 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap12 = bitcast %struct.__va_list_tag* %ap1 to i8* ; <i8*> [#uses=1]
  call void @llvm.va_start(i8* %ap12)
  %1 = load i8** %fmt_addr, align 8               ; <i8*> [#uses=1]
  %ap3 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %2 = call i32 @vprintf(i8* noalias %1, %struct.__va_list_tag* %ap3) nounwind ; <i32> [#uses=0]
  %ap4 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap45 = bitcast %struct.__va_list_tag* %ap4 to i8* ; <i8*> [#uses=1]
  call void @llvm.va_end(i8* %ap45)
  %3 = call i32 @putchar(i32 10) nounwind         ; <i32> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  ret void
}

declare i32 @putchar(i32)

declare void @llvm.va_start(i8*) nounwind

declare i32 @vprintf(i8* noalias, %struct.__va_list_tag*) nounwind

declare void @llvm.va_end(i8*) nounwind

define void @main() nounwind {
entry:
  %x = alloca i32*                                ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call noalias i8* @malloc(i64 4) nounwind   ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to i32*                     ; <i32*> [#uses=1]
  store i32* %1, i32** %x, align 8
  call void (i8*, ...)* @generic_sendmsg(i8* getelementptr inbounds ([22 x i8]* @.str, i64 0, i64 0), i32 1234, i32 1234, double 1.232200e+02, double 1.234500e+02) nounwind
  call void (i8*, ...)* @generic_sendmsg(i8* getelementptr inbounds ([12 x i8]* @.str1, i64 0, i64 0), i8* getelementptr inbounds ([5 x i8]* @.str2, i64 0, i64 0), i8* getelementptr inbounds ([4 x i8]* @.str3, i64 0, i64 0), i8* getelementptr inbounds ([8 x i8]* @.str4, i64 0, i64 0)) nounwind
  %2 = load i32** %x, align 8                     ; <i32*> [#uses=1]
  call void (i8*, ...)* @generic_sendmsg(i8* getelementptr inbounds ([15 x i8]* @.str5, i64 0, i64 0), i8* getelementptr inbounds ([5 x i8]* @.str2, i64 0, i64 0), i8* getelementptr inbounds ([4 x i8]* @.str3, i64 0, i64 0), i8* getelementptr inbounds ([8 x i8]* @.str4, i64 0, i64 0), i32* %2) nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

declare noalias i8* @malloc(i64) nounwind

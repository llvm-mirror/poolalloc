
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:ia:0,0:i16\|i32::4:i8

; ModuleID = 'bitfields1.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.anon = type <{ i16, i16, i8, [3 x i8] }>
%union.I_format_t = type { i32, [1 x i32] }

@.str = private constant [18 x i8] c"\0Ainstruction: %X\0A\00", align 1 ; <[18 x i8]*> [#uses=1]
@.str1 = private constant [12 x i8] c"opcode: %X\0A\00", align 1 ; <[12 x i8]*> [#uses=1]
@.str2 = private constant [23 x i8] c"rs: %d rt: %d rj: %d \0A\00", align 1 ; <[23 x i8]*> [#uses=1]
@.str3 = private constant [11 x i8] c"immed: %d\0A\00", align 1 ; <[11 x i8]*> [#uses=1]

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %ia = alloca %union.I_format_t*                 ; <%union.I_format_t**> [#uses=8]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = call noalias i8* @malloc(i64 8) nounwind   ; <i8*> [#uses=1]
  %2 = bitcast i8* %1 to %union.I_format_t*       ; <%union.I_format_t*> [#uses=1]
  store %union.I_format_t* %2, %union.I_format_t** %ia, align 8
  %3 = load %union.I_format_t** %ia, align 8      ; <%union.I_format_t*> [#uses=1]
  %4 = getelementptr inbounds %union.I_format_t* %3, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 -1346502640, i32* %4, align 4
  %5 = load %union.I_format_t** %ia, align 8      ; <%union.I_format_t*> [#uses=1]
  %6 = getelementptr inbounds %union.I_format_t* %5, i32 0, i32 0 ; <i32*> [#uses=1]
  %7 = load i32* %6, align 4                      ; <i32> [#uses=1]
  %8 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([18 x i8]* @.str, i64 0, i64 0), i32 %7) nounwind ; <i32> [#uses=0]
  %9 = load %union.I_format_t** %ia, align 8      ; <%union.I_format_t*> [#uses=1]
  %10 = getelementptr inbounds %union.I_format_t* %9, i32 0, i32 0 ; <i32*> [#uses=1]
  %11 = bitcast i32* %10 to %struct.anon*         ; <%struct.anon*> [#uses=1]
  %12 = getelementptr inbounds %struct.anon* %11, i32 0, i32 2 ; <i8*> [#uses=1]
  %13 = load i8* %12, align 1                     ; <i8> [#uses=1]
  %14 = shl i8 %13, 2                             ; <i8> [#uses=1]
  %15 = lshr i8 %14, 2                            ; <i8> [#uses=1]
  %16 = trunc i8 %15 to i6                        ; <i6> [#uses=1]
  %17 = zext i6 %16 to i32                        ; <i32> [#uses=1]
  %18 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([12 x i8]* @.str1, i64 0, i64 0), i32 %17) nounwind ; <i32> [#uses=0]
  %19 = load %union.I_format_t** %ia, align 8     ; <%union.I_format_t*> [#uses=1]
  %20 = getelementptr inbounds %union.I_format_t* %19, i32 0, i32 0 ; <i32*> [#uses=1]
  %21 = bitcast i32* %20 to %struct.anon*         ; <%struct.anon*> [#uses=1]
  %22 = getelementptr inbounds %struct.anon* %21, i32 0, i32 0 ; <i16*> [#uses=1]
  %23 = bitcast i16* %22 to i32*                  ; <i32*> [#uses=1]
  %24 = load i32* %23, align 1                    ; <i32> [#uses=1]
  %25 = shl i32 %24, 1                            ; <i32> [#uses=1]
  %26 = lshr i32 %25, 27                          ; <i32> [#uses=1]
  %27 = trunc i32 %26 to i5                       ; <i5> [#uses=1]
  %28 = zext i5 %27 to i32                        ; <i32> [#uses=1]
  %29 = load %union.I_format_t** %ia, align 8     ; <%union.I_format_t*> [#uses=1]
  %30 = getelementptr inbounds %union.I_format_t* %29, i32 0, i32 0 ; <i32*> [#uses=1]
  %31 = bitcast i32* %30 to %struct.anon*         ; <%struct.anon*> [#uses=1]
  %32 = getelementptr inbounds %struct.anon* %31, i32 0, i32 0 ; <i16*> [#uses=1]
  %33 = bitcast i16* %32 to i32*                  ; <i32*> [#uses=1]
  %34 = load i32* %33, align 1                    ; <i32> [#uses=1]
  %35 = shl i32 %34, 11                           ; <i32> [#uses=1]
  %36 = lshr i32 %35, 27                          ; <i32> [#uses=1]
  %37 = trunc i32 %36 to i5                       ; <i5> [#uses=1]
  %38 = zext i5 %37 to i32                        ; <i32> [#uses=1]
  %39 = load %union.I_format_t** %ia, align 8     ; <%union.I_format_t*> [#uses=1]
  %40 = getelementptr inbounds %union.I_format_t* %39, i32 0, i32 0 ; <i32*> [#uses=1]
  %41 = bitcast i32* %40 to %struct.anon*         ; <%struct.anon*> [#uses=1]
  %42 = getelementptr inbounds %struct.anon* %41, i32 0, i32 0 ; <i16*> [#uses=1]
  %43 = bitcast i16* %42 to i32*                  ; <i32*> [#uses=1]
  %44 = load i32* %43, align 1                    ; <i32> [#uses=1]
  %45 = shl i32 %44, 6                            ; <i32> [#uses=1]
  %46 = lshr i32 %45, 27                          ; <i32> [#uses=1]
  %47 = trunc i32 %46 to i5                       ; <i5> [#uses=1]
  %48 = zext i5 %47 to i32                        ; <i32> [#uses=1]
  %49 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([23 x i8]* @.str2, i64 0, i64 0), i32 %48, i32 %38, i32 %28) nounwind ; <i32> [#uses=0]
  %50 = load %union.I_format_t** %ia, align 8     ; <%union.I_format_t*> [#uses=1]
  %51 = getelementptr inbounds %union.I_format_t* %50, i32 0, i32 0 ; <i32*> [#uses=1]
  %52 = bitcast i32* %51 to %struct.anon*         ; <%struct.anon*> [#uses=1]
  %53 = getelementptr inbounds %struct.anon* %52, i32 0, i32 0 ; <i16*> [#uses=1]
  %54 = load i16* %53, align 1                    ; <i16> [#uses=1]
  %55 = sext i16 %54 to i32                       ; <i32> [#uses=1]
  %56 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([11 x i8]* @.str3, i64 0, i64 0), i32 %55) nounwind ; <i32> [#uses=0]
  store i32 0, i32* %0, align 4
  %57 = load i32* %0, align 4                     ; <i32> [#uses=1]
  store i32 %57, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

declare noalias i8* @malloc(i64) nounwind

declare i32 @printf(i8* noalias, ...) nounwind

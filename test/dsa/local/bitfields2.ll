
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:codes,0:i32Array
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:ia,0:i16\|i32

; ModuleID = 'bitfields2.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%0 = type <{ i16, i16 }>
%struct.anon = type <{ i16, [2 x i8] }>
%union.mips_format_t = type { i32 }

@C.0.1960 = private constant [4 x i32] [i32 666763240, i32 -1346502640, i32 60878881, i32 604110862] ; <[4 x i32]*> [#uses=4]
@.str = private constant [18 x i8] c"\0Ainstruction: %X\0A\00", align 1 ; <[18 x i8]*> [#uses=1]
@.str1 = private constant [12 x i8] c"opcode: %X\0A\00", align 1 ; <[12 x i8]*> [#uses=1]
@.str2 = private constant [15 x i8] c"rs: %d rt: %d\0A\00", align 1 ; <[15 x i8]*> [#uses=1]
@.str3 = private constant [24 x i8] c"rd: %d  sh: %d  fn: %X\0A\00", align 1 ; <[24 x i8]*> [#uses=1]
@.str4 = private constant [11 x i8] c"immed: %d\0A\00", align 1 ; <[11 x i8]*> [#uses=1]

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %i = alloca i32                                 ; <i32*> [#uses=5]
  %ia = alloca %union.mips_format_t               ; <%union.mips_format_t*> [#uses=10]
  %codes = alloca [4 x i32]                       ; <[4 x i32]*> [#uses=5]
  %n = alloca i32                                 ; <i32*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = getelementptr inbounds [4 x i32]* %codes, i32 0, i32 0 ; <i32*> [#uses=1]
  %2 = load i32* getelementptr inbounds ([4 x i32]* @C.0.1960, i64 0, i64 0), align 4 ; <i32> [#uses=1]
  store i32 %2, i32* %1, align 4
  %3 = getelementptr inbounds [4 x i32]* %codes, i32 0, i32 1 ; <i32*> [#uses=1]
  %4 = load i32* getelementptr inbounds ([4 x i32]* @C.0.1960, i64 0, i64 1), align 4 ; <i32> [#uses=1]
  store i32 %4, i32* %3, align 4
  %5 = getelementptr inbounds [4 x i32]* %codes, i32 0, i32 2 ; <i32*> [#uses=1]
  %6 = load i32* getelementptr inbounds ([4 x i32]* @C.0.1960, i64 0, i64 2), align 4 ; <i32> [#uses=1]
  store i32 %6, i32* %5, align 4
  %7 = getelementptr inbounds [4 x i32]* %codes, i32 0, i32 3 ; <i32*> [#uses=1]
  %8 = load i32* getelementptr inbounds ([4 x i32]* @C.0.1960, i64 0, i64 3), align 4 ; <i32> [#uses=1]
  store i32 %8, i32* %7, align 4
  store i32 4, i32* %n, align 4
  store i32 0, i32* %i, align 4
  br label %bb4

bb:                                               ; preds = %bb4
  %9 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %10 = sext i32 %9 to i64                        ; <i64> [#uses=1]
  %11 = getelementptr inbounds [4 x i32]* %codes, i64 0, i64 %10 ; <i32*> [#uses=1]
  %12 = load i32* %11, align 4                    ; <i32> [#uses=1]
  %13 = getelementptr inbounds %union.mips_format_t* %ia, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 %12, i32* %13, align 4
  %14 = getelementptr inbounds %union.mips_format_t* %ia, i32 0, i32 0 ; <i32*> [#uses=1]
  %15 = load i32* %14, align 4                    ; <i32> [#uses=1]
  %16 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([18 x i8]* @.str, i64 0, i64 0), i32 %15) nounwind ; <i32> [#uses=0]
  %17 = getelementptr inbounds %union.mips_format_t* %ia, i32 0, i32 0 ; <i32*> [#uses=1]
  %18 = bitcast i32* %17 to %0*                   ; <%0*> [#uses=1]
  %19 = getelementptr inbounds %0* %18, i32 0, i32 0 ; <i16*> [#uses=1]
  %20 = bitcast i16* %19 to i32*                  ; <i32*> [#uses=1]
  %21 = load i32* %20, align 1                    ; <i32> [#uses=1]
  %22 = lshr i32 %21, 26                          ; <i32> [#uses=1]
  %23 = trunc i32 %22 to i6                       ; <i6> [#uses=1]
  %24 = zext i6 %23 to i32                        ; <i32> [#uses=1]
  %25 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([12 x i8]* @.str1, i64 0, i64 0), i32 %24) nounwind ; <i32> [#uses=0]
  %26 = getelementptr inbounds %union.mips_format_t* %ia, i32 0, i32 0 ; <i32*> [#uses=1]
  %27 = bitcast i32* %26 to %0*                   ; <%0*> [#uses=1]
  %28 = getelementptr inbounds %0* %27, i32 0, i32 0 ; <i16*> [#uses=1]
  %29 = bitcast i16* %28 to i32*                  ; <i32*> [#uses=1]
  %30 = load i32* %29, align 1                    ; <i32> [#uses=1]
  %31 = shl i32 %30, 11                           ; <i32> [#uses=1]
  %32 = lshr i32 %31, 27                          ; <i32> [#uses=1]
  %33 = trunc i32 %32 to i5                       ; <i5> [#uses=1]
  %34 = zext i5 %33 to i32                        ; <i32> [#uses=1]
  %35 = getelementptr inbounds %union.mips_format_t* %ia, i32 0, i32 0 ; <i32*> [#uses=1]
  %36 = bitcast i32* %35 to %0*                   ; <%0*> [#uses=1]
  %37 = getelementptr inbounds %0* %36, i32 0, i32 0 ; <i16*> [#uses=1]
  %38 = bitcast i16* %37 to i32*                  ; <i32*> [#uses=1]
  %39 = load i32* %38, align 1                    ; <i32> [#uses=1]
  %40 = shl i32 %39, 6                            ; <i32> [#uses=1]
  %41 = lshr i32 %40, 27                          ; <i32> [#uses=1]
  %42 = trunc i32 %41 to i5                       ; <i5> [#uses=1]
  %43 = zext i5 %42 to i32                        ; <i32> [#uses=1]
  %44 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([15 x i8]* @.str2, i64 0, i64 0), i32 %43, i32 %34) nounwind ; <i32> [#uses=0]
  %45 = getelementptr inbounds %union.mips_format_t* %ia, i32 0, i32 0 ; <i32*> [#uses=1]
  %46 = bitcast i32* %45 to %0*                   ; <%0*> [#uses=1]
  %47 = getelementptr inbounds %0* %46, i32 0, i32 0 ; <i16*> [#uses=1]
  %48 = bitcast i16* %47 to i32*                  ; <i32*> [#uses=1]
  %49 = load i32* %48, align 1                    ; <i32> [#uses=1]
  %50 = lshr i32 %49, 26                          ; <i32> [#uses=1]
  %51 = trunc i32 %50 to i6                       ; <i6> [#uses=1]
  %52 = icmp eq i6 %51, 0                         ; <i1> [#uses=1]
  br i1 %52, label %bb1, label %bb2

bb1:                                              ; preds = %bb
  %53 = getelementptr inbounds %union.mips_format_t* %ia, i32 0, i32 0 ; <i32*> [#uses=1]
  %54 = bitcast i32* %53 to %struct.anon*         ; <%struct.anon*> [#uses=1]
  %55 = getelementptr inbounds %struct.anon* %54, i32 0, i32 0 ; <i16*> [#uses=1]
  %56 = load i16* %55, align 1                    ; <i16> [#uses=1]
  %57 = shl i16 %56, 10                           ; <i16> [#uses=1]
  %58 = lshr i16 %57, 10                          ; <i16> [#uses=1]
  %59 = trunc i16 %58 to i6                       ; <i6> [#uses=1]
  %60 = zext i6 %59 to i32                        ; <i32> [#uses=1]
  %61 = getelementptr inbounds %union.mips_format_t* %ia, i32 0, i32 0 ; <i32*> [#uses=1]
  %62 = bitcast i32* %61 to %struct.anon*         ; <%struct.anon*> [#uses=1]
  %63 = getelementptr inbounds %struct.anon* %62, i32 0, i32 0 ; <i16*> [#uses=1]
  %64 = load i16* %63, align 1                    ; <i16> [#uses=1]
  %65 = shl i16 %64, 5                            ; <i16> [#uses=1]
  %66 = lshr i16 %65, 11                          ; <i16> [#uses=1]
  %67 = trunc i16 %66 to i5                       ; <i5> [#uses=1]
  %68 = zext i5 %67 to i32                        ; <i32> [#uses=1]
  %69 = getelementptr inbounds %union.mips_format_t* %ia, i32 0, i32 0 ; <i32*> [#uses=1]
  %70 = bitcast i32* %69 to %struct.anon*         ; <%struct.anon*> [#uses=1]
  %71 = getelementptr inbounds %struct.anon* %70, i32 0, i32 0 ; <i16*> [#uses=1]
  %72 = load i16* %71, align 1                    ; <i16> [#uses=1]
  %73 = lshr i16 %72, 11                          ; <i16> [#uses=1]
  %74 = trunc i16 %73 to i5                       ; <i5> [#uses=1]
  %75 = zext i5 %74 to i32                        ; <i32> [#uses=1]
  %76 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([24 x i8]* @.str3, i64 0, i64 0), i32 %75, i32 %68, i32 %60) nounwind ; <i32> [#uses=0]
  br label %bb3

bb2:                                              ; preds = %bb
  %77 = getelementptr inbounds %union.mips_format_t* %ia, i32 0, i32 0 ; <i32*> [#uses=1]
  %78 = bitcast i32* %77 to %0*                   ; <%0*> [#uses=1]
  %79 = getelementptr inbounds %0* %78, i32 0, i32 0 ; <i16*> [#uses=1]
  %80 = load i16* %79, align 1                    ; <i16> [#uses=1]
  %81 = sext i16 %80 to i32                       ; <i32> [#uses=1]
  %82 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([11 x i8]* @.str4, i64 0, i64 0), i32 %81) nounwind ; <i32> [#uses=0]
  br label %bb3

bb3:                                              ; preds = %bb2, %bb1
  %83 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %84 = add nsw i32 %83, 1                        ; <i32> [#uses=1]
  store i32 %84, i32* %i, align 4
  br label %bb4

bb4:                                              ; preds = %bb3, %entry
  %85 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %86 = load i32* %n, align 4                     ; <i32> [#uses=1]
  %87 = icmp slt i32 %85, %86                     ; <i1> [#uses=1]
  br i1 %87, label %bb, label %bb5

bb5:                                              ; preds = %bb4
  store i32 0, i32* %0, align 4
  %88 = load i32* %0, align 4                     ; <i32> [#uses=1]
  store i32 %88, i32* %retval, align 4
  br label %return

return:                                           ; preds = %bb5
  %retval6 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval6
}

declare i32 @printf(i8* noalias, ...) nounwind

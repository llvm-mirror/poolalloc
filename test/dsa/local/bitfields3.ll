; example with a packed struct
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:t,0:i32

; ModuleID = 'bitfields3.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%0 = type { i8, i8, i8, i8 }
%1 = type { i64 }
%struct.taxonomy = type <{ i32 }>

@C.0.1944 = private constant %0 { i8 64, i8 5, i8 0, i8 0 }, align 4 ; <%0*> [#uses=1]
@.str = private constant [35 x i8] c"sizeof(struct taxonomy): %d bytes\0A\00", align 8 ; <[35 x i8]*> [#uses=1]
@.str1 = private constant [16 x i8] c"taxonomy: 0x%x\0A\00", align 1 ; <[16 x i8]*> [#uses=1]
@.str2 = private constant [13 x i8] c"kingdom: %d\0A\00", align 1 ; <[13 x i8]*> [#uses=1]
@.str3 = private constant [12 x i8] c"phylum: %d\0A\00", align 1 ; <[12 x i8]*> [#uses=1]
@.str4 = private constant [11 x i8] c"genus: %d\0A\00", align 1 ; <[11 x i8]*> [#uses=1]

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %t = alloca %struct.taxonomy, align 4           ; <%struct.taxonomy*> [#uses=7]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = getelementptr inbounds %struct.taxonomy* %t, i32 0, i32 0 ; <i32*> [#uses=1]
  %2 = load i32* bitcast (%0* @C.0.1944 to i32*), align 4 ; <i32> [#uses=1]
  store i32 %2, i32* %1, align 4
  %3 = getelementptr inbounds %struct.taxonomy* %t, i32 0, i32 0 ; <i32*> [#uses=2]
  %4 = load i32* %3, align 1                      ; <i32> [#uses=1]
  %5 = and i32 %4, -4                             ; <i32> [#uses=1]
  %6 = or i32 %5, 1                               ; <i32> [#uses=1]
  store i32 %6, i32* %3, align 1
  %7 = getelementptr inbounds %struct.taxonomy* %t, i32 0, i32 0 ; <i32*> [#uses=2]
  %8 = load i32* %7, align 1                      ; <i32> [#uses=1]
  %9 = and i32 %8, -61                            ; <i32> [#uses=1]
  %10 = or i32 %9, 28                             ; <i32> [#uses=1]
  store i32 %10, i32* %7, align 1
  %11 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([35 x i8]* @.str, i64 0, i64 0), i64 4) nounwind ; <i32> [#uses=0]
  %12 = bitcast %struct.taxonomy* %t to %1*       ; <%1*> [#uses=1]
  %elt = getelementptr inbounds %1* %12, i32 0, i32 0 ; <i64*> [#uses=1]
  %13 = bitcast i64* %elt to i32*                 ; <i32*> [#uses=1]
  %14 = load i32* %13                             ; <i32> [#uses=1]
  %15 = zext i32 %14 to i64                       ; <i64> [#uses=1]
  %16 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([16 x i8]* @.str1, i64 0, i64 0), i64 %15) nounwind ; <i32> [#uses=0]
  %17 = getelementptr inbounds %struct.taxonomy* %t, i32 0, i32 0 ; <i32*> [#uses=1]
  %18 = load i32* %17, align 1                    ; <i32> [#uses=1]
  %19 = shl i32 %18, 30                           ; <i32> [#uses=1]
  %20 = lshr i32 %19, 30                          ; <i32> [#uses=1]
  %21 = trunc i32 %20 to i2                       ; <i2> [#uses=1]
  %22 = zext i2 %21 to i32                        ; <i32> [#uses=1]
  %23 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([13 x i8]* @.str2, i64 0, i64 0), i32 %22) nounwind ; <i32> [#uses=0]
  %24 = getelementptr inbounds %struct.taxonomy* %t, i32 0, i32 0 ; <i32*> [#uses=1]
  %25 = load i32* %24, align 1                    ; <i32> [#uses=1]
  %26 = shl i32 %25, 26                           ; <i32> [#uses=1]
  %27 = lshr i32 %26, 28                          ; <i32> [#uses=1]
  %28 = trunc i32 %27 to i4                       ; <i4> [#uses=1]
  %29 = zext i4 %28 to i32                        ; <i32> [#uses=1]
  %30 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([12 x i8]* @.str3, i64 0, i64 0), i32 %29) nounwind ; <i32> [#uses=0]
  %31 = getelementptr inbounds %struct.taxonomy* %t, i32 0, i32 0 ; <i32*> [#uses=1]
  %32 = load i32* %31, align 1                    ; <i32> [#uses=1]
  %33 = shl i32 %32, 14                           ; <i32> [#uses=1]
  %34 = lshr i32 %33, 20                          ; <i32> [#uses=1]
  %35 = trunc i32 %34 to i12                      ; <i12> [#uses=1]
  %36 = zext i12 %35 to i32                       ; <i32> [#uses=1]
  %37 = call i32 (i8*, ...)* @printf(i8* noalias getelementptr inbounds ([11 x i8]* @.str4, i64 0, i64 0), i32 %36) nounwind ; <i32> [#uses=0]
  store i32 0, i32* %0, align 4
  %38 = load i32* %0, align 4                     ; <i32> [#uses=1]
  store i32 %38, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

declare i32 @printf(i8* noalias, ...) nounwind

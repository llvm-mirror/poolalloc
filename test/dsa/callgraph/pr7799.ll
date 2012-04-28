; ModuleID = 'test.opt.bc'
;RUN: dsaopt %s -dsa-td -analyze -check-callees=main,a,b
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=main,a,b
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128"
target triple = "x86_64-unknown-linux-gnu"

%struct.combo = type { i8*, i32 (i32)* }

@farray = internal global [2 x %struct.combo] [%struct.combo { i8* getelementptr inbounds ([6 x i8]* @.str, i64 0, i64 0), i32 (i32)* @a }, %struct.combo { i8* getelementptr inbounds ([7 x i8]* @.str1, i64 0, i64 0), i32 (i32)* @b }], align 32 ; <[2 x %struct.combo]*> [#uses=1]
@.str = private constant [6 x i8] c"first\00", align 1 ; <[6 x i8]*> [#uses=1]
@.str1 = private constant [7 x i8] c"second\00", align 1 ; <[7 x i8]*> [#uses=1]

define internal i32 @a(i32 %x) nounwind {
entry:
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = add nsw i32 %x, %x                         ; <i32> [#uses=1]
  br label %return

return:                                           ; preds = %entry
  ret i32 %0
}

define internal i32 @b(i32 %x) nounwind {
entry:
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = mul i32 %x, %x                             ; <i32> [#uses=1]
  br label %return

return:                                           ; preds = %entry
  ret i32 %0
}

define i32 @main(i32 %argc, i8*** %argv) nounwind {
entry:
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = sext i32 %argc to i64                      ; <i64> [#uses=1]
  %1 = getelementptr inbounds [2 x %struct.combo]* @farray, i64 0, i64 %0 ; <%struct.combo*> [#uses=1]
  %2 = getelementptr inbounds %struct.combo* %1, i32 0, i32 1 ; <i32 (i32)**> [#uses=1]
  %3 = load i32 (i32)** %2, align 8               ; <i32 (i32)*> [#uses=1]
  %4 = call i32 %3(i32 %argc) nounwind            ; <i32> [#uses=1]
  br label %return

return:                                           ; preds = %entry
  ret i32 %4
}

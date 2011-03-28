; Example from Milanova, Rountev, and Ryder(Precise Call graphs for C programs with Function Pointers)
; Because table is not const, func1, and func2 do not get inlined into main
; ModuleID = 'tt.o'
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=main,func1,func2
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.parse_table = type { i8*, i32 ()* }

@table = internal global [2 x %struct.parse_table] [%struct.parse_table { i8* getelementptr inbounds ([6 x i8]* @.str, i64 0, i64 0), i32 ()* @func1 }, %struct.parse_table { i8* getelementptr inbounds ([6 x i8]* @.str1, i64 0, i64 0), i32 ()* @func2 }], align 32 ; <[2 x %struct.parse_table]*> [#uses=2]
@.str = private constant [6 x i8] c"name1\00", align 1 ; <[6 x i8]*> [#uses=1]
@.str1 = private constant [6 x i8] c"name2\00", align 1 ; <[6 x i8]*> [#uses=1]

define internal i32 @func1() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 1, i32* %0, align 4
  %1 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %1, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

define internal i32 @func2() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 2, i32* %0, align 4
  %1 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %1, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

define internal i32 ()* @find_p_func(i8* %s) nounwind {
entry:
  %s_addr = alloca i8*                            ; <i8**> [#uses=2]
  %retval = alloca i32 ()*                        ; <i32 ()**> [#uses=2]
  %0 = alloca i32 ()*                             ; <i32 ()**> [#uses=3]
  %i = alloca i32                                 ; <i32*> [#uses=6]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i8* %s, i8** %s_addr
  store i32 0, i32* %i, align 4
  br label %bb3

bb:                                               ; preds = %bb3
  %1 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %2 = sext i32 %1 to i64                         ; <i64> [#uses=1]
  %3 = getelementptr inbounds [2 x %struct.parse_table]* @table, i64 0, i64 %2 ; <%struct.parse_table*> [#uses=1]
  %4 = getelementptr inbounds %struct.parse_table* %3, i32 0, i32 0 ; <i8**> [#uses=1]
  %5 = load i8** %4, align 8                      ; <i8*> [#uses=1]
  %6 = load i8** %s_addr, align 8                 ; <i8*> [#uses=1]
  %7 = call i32 (...)* bitcast (i32 (i8*, i8*)* @strcmp to i32 (...)*)(i8* %5, i8* %6) nounwind readonly ; <i32> [#uses=1]
  %8 = icmp eq i32 %7, 0                          ; <i1> [#uses=1]
  br i1 %8, label %bb1, label %bb2

bb1:                                              ; preds = %bb
  %9 = load i32* %i, align 4                      ; <i32> [#uses=1]
  %10 = sext i32 %9 to i64                        ; <i64> [#uses=1]
  %11 = getelementptr inbounds [2 x %struct.parse_table]* @table, i64 0, i64 %10 ; <%struct.parse_table*> [#uses=1]
  %12 = getelementptr inbounds %struct.parse_table* %11, i32 0, i32 1 ; <i32 ()**> [#uses=1]
  %13 = load i32 ()** %12, align 8                ; <i32 ()*> [#uses=1]
  store i32 ()* %13, i32 ()** %0, align 8
  br label %bb5

bb2:                                              ; preds = %bb
  %14 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %15 = add nsw i32 %14, 1                        ; <i32> [#uses=1]
  store i32 %15, i32* %i, align 4
  br label %bb3

bb3:                                              ; preds = %bb2, %entry
  %16 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %17 = icmp sle i32 %16, 1                       ; <i1> [#uses=1]
  br i1 %17, label %bb, label %bb4

bb4:                                              ; preds = %bb3
  store i32 ()* null, i32 ()** %0, align 8
  br label %bb5

bb5:                                              ; preds = %bb4, %bb1
  %18 = load i32 ()** %0, align 8                 ; <i32 ()*> [#uses=1]
  store i32 ()* %18, i32 ()** %retval, align 8
  br label %return

return:                                           ; preds = %bb5
  %retval6 = load i32 ()** %retval                ; <i32 ()*> [#uses=1]
  ret i32 ()* %retval6
}

declare i32 @strcmp(i8*, i8*) nounwind readonly

define i32 @main(i32 %argc, i8** %argv) nounwind {
entry:
  %argc_addr = alloca i32                         ; <i32*> [#uses=1]
  %argv_addr = alloca i8**                        ; <i8***> [#uses=2]
  %retval = alloca i32                            ; <i32*> [#uses=1]
  %parse_func = alloca i32 ()*                    ; <i32 ()**> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %argc, i32* %argc_addr
  store i8** %argv, i8*** %argv_addr
  %0 = load i8*** %argv_addr, align 8             ; <i8**> [#uses=1]
  %1 = getelementptr inbounds i8** %0, i64 1      ; <i8**> [#uses=1]
  %2 = load i8** %1, align 1                      ; <i8*> [#uses=1]
  %3 = call i32 ()* (i8*)* @find_p_func(i8* %2) nounwind ; <i32 ()*> [#uses=1]
  store i32 ()* %3, i32 ()** %parse_func, align 8
  %4 = load i32 ()** %parse_func, align 8         ; <i32 ()*> [#uses=1]
  %5 = icmp ne i32 ()* %4, null                   ; <i1> [#uses=1]
  br i1 %5, label %bb, label %bb1

bb:                                               ; preds = %entry
  %6 = load i32 ()** %parse_func, align 8         ; <i32 ()*> [#uses=1]
  %7 = call i32 %6() nounwind                     ; <i32> [#uses=0]
  br label %bb1

bb1:                                              ; preds = %bb, %entry
  br label %return

return:                                           ; preds = %bb1
  %retval2 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval2
}

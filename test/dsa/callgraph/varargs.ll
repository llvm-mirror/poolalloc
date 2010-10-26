;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=assign,A,B
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=assign,A,B

;Go through a list of functions passed as varargs, and call each one.

; ModuleID = 'multiple_callee.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag = type { i32, i32, i8*, i8* }

define internal void @A() nounwind {
entry:
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @B() nounwind {
entry:
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @C() nounwind {
entry:
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @assign(i32 %count, ...) nounwind {
entry:
  %count_addr = alloca i32                        ; <i32*> [#uses=2]
  %addr.2 = alloca i8*                            ; <i8**> [#uses=3]
  %addr.0 = alloca i8*                            ; <i8**> [#uses=3]
  %ap = alloca [1 x %struct.__va_list_tag]        ; <[1 x %struct.__va_list_tag]*> [#uses=16]
  %old = alloca void ()*                          ; <void ()**> [#uses=3]
  %i = alloca i32                                 ; <i32*> [#uses=4]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %count, i32* %count_addr
  %ap1 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap12 = bitcast %struct.__va_list_tag* %ap1 to i8* ; <i8*> [#uses=1]
  call void @llvm.va_start(i8* %ap12)
  %0 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %1 = getelementptr inbounds %struct.__va_list_tag* %0, i32 0, i32 0 ; <i32*> [#uses=1]
  %2 = load i32* %1, align 8                      ; <i32> [#uses=1]
  %3 = icmp uge i32 %2, 48                        ; <i1> [#uses=1]
  br i1 %3, label %bb3, label %bb

bb:                                               ; preds = %entry
  %4 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %5 = getelementptr inbounds %struct.__va_list_tag* %4, i32 0, i32 3 ; <i8**> [#uses=1]
  %6 = load i8** %5, align 8                      ; <i8*> [#uses=1]
  %7 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %8 = getelementptr inbounds %struct.__va_list_tag* %7, i32 0, i32 0 ; <i32*> [#uses=1]
  %9 = load i32* %8, align 8                      ; <i32> [#uses=1]
  %10 = inttoptr i32 %9 to i8*                    ; <i8*> [#uses=1]
  %11 = ptrtoint i8* %6 to i64                    ; <i64> [#uses=1]
  %12 = ptrtoint i8* %10 to i64                   ; <i64> [#uses=1]
  %13 = add i64 %11, %12                          ; <i64> [#uses=1]
  %14 = inttoptr i64 %13 to i8*                   ; <i8*> [#uses=1]
  store i8* %14, i8** %addr.0, align 8
  %15 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %16 = getelementptr inbounds %struct.__va_list_tag* %15, i32 0, i32 0 ; <i32*> [#uses=1]
  %17 = load i32* %16, align 8                    ; <i32> [#uses=1]
  %18 = add i32 %17, 8                            ; <i32> [#uses=1]
  %19 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %20 = getelementptr inbounds %struct.__va_list_tag* %19, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 %18, i32* %20, align 8
  br label %bb4

bb3:                                              ; preds = %entry
  %21 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %22 = getelementptr inbounds %struct.__va_list_tag* %21, i32 0, i32 2 ; <i8**> [#uses=1]
  %23 = load i8** %22, align 8                    ; <i8*> [#uses=2]
  store i8* %23, i8** %addr.0, align 8
  %24 = getelementptr inbounds i8* %23, i64 8     ; <i8*> [#uses=1]
  %25 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %26 = getelementptr inbounds %struct.__va_list_tag* %25, i32 0, i32 2 ; <i8**> [#uses=1]
  store i8* %24, i8** %26, align 8
  br label %bb4

bb4:                                              ; preds = %bb3, %bb
  %27 = load i8** %addr.0, align 8                ; <i8*> [#uses=1]
  %28 = bitcast i8* %27 to void ()**              ; <void ()**> [#uses=1]
  %29 = load void ()** %28, align 8               ; <void ()*> [#uses=1]
  store void ()* %29, void ()** %old, align 8
  store i32 0, i32* %i, align 4
  br label %bb9

bb5:                                              ; preds = %bb9
  %30 = load void ()** %old, align 8              ; <void ()*> [#uses=1]
  call void %30() nounwind
  %31 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %32 = getelementptr inbounds %struct.__va_list_tag* %31, i32 0, i32 0 ; <i32*> [#uses=1]
  %33 = load i32* %32, align 8                    ; <i32> [#uses=1]
  %34 = icmp uge i32 %33, 48                      ; <i1> [#uses=1]
  br i1 %34, label %bb7, label %bb6

bb6:                                              ; preds = %bb5
  %35 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %36 = getelementptr inbounds %struct.__va_list_tag* %35, i32 0, i32 3 ; <i8**> [#uses=1]
  %37 = load i8** %36, align 8                    ; <i8*> [#uses=1]
  %38 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %39 = getelementptr inbounds %struct.__va_list_tag* %38, i32 0, i32 0 ; <i32*> [#uses=1]
  %40 = load i32* %39, align 8                    ; <i32> [#uses=1]
  %41 = inttoptr i32 %40 to i8*                   ; <i8*> [#uses=1]
  %42 = ptrtoint i8* %37 to i64                   ; <i64> [#uses=1]
  %43 = ptrtoint i8* %41 to i64                   ; <i64> [#uses=1]
  %44 = add i64 %42, %43                          ; <i64> [#uses=1]
  %45 = inttoptr i64 %44 to i8*                   ; <i8*> [#uses=1]
  store i8* %45, i8** %addr.2, align 8
  %46 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %47 = getelementptr inbounds %struct.__va_list_tag* %46, i32 0, i32 0 ; <i32*> [#uses=1]
  %48 = load i32* %47, align 8                    ; <i32> [#uses=1]
  %49 = add i32 %48, 8                            ; <i32> [#uses=1]
  %50 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %51 = getelementptr inbounds %struct.__va_list_tag* %50, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 %49, i32* %51, align 8
  br label %bb8

bb7:                                              ; preds = %bb5
  %52 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %53 = getelementptr inbounds %struct.__va_list_tag* %52, i32 0, i32 2 ; <i8**> [#uses=1]
  %54 = load i8** %53, align 8                    ; <i8*> [#uses=2]
  store i8* %54, i8** %addr.2, align 8
  %55 = getelementptr inbounds i8* %54, i64 8     ; <i8*> [#uses=1]
  %56 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %57 = getelementptr inbounds %struct.__va_list_tag* %56, i32 0, i32 2 ; <i8**> [#uses=1]
  store i8* %55, i8** %57, align 8
  br label %bb8

bb8:                                              ; preds = %bb7, %bb6
  %58 = load i8** %addr.2, align 8                ; <i8*> [#uses=1]
  %59 = bitcast i8* %58 to i8**                   ; <i8**> [#uses=1]
  %60 = load i8** %59, align 8                    ; <i8*> [#uses=1]
  %61 = bitcast i8* %60 to void ()*               ; <void ()*> [#uses=1]
  store void ()* %61, void ()** %old, align 8
  %62 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %63 = add nsw i32 %62, 1                        ; <i32> [#uses=1]
  store i32 %63, i32* %i, align 4
  br label %bb9

bb9:                                              ; preds = %bb8, %bb4
  %64 = load i32* %i, align 4                     ; <i32> [#uses=1]
  %65 = load i32* %count_addr, align 4            ; <i32> [#uses=1]
  %66 = icmp slt i32 %64, %65                     ; <i1> [#uses=1]
  br i1 %66, label %bb5, label %bb10

bb10:                                             ; preds = %bb9
  %ap11 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap1112 = bitcast %struct.__va_list_tag* %ap11 to i8* ; <i8*> [#uses=1]
  call void @llvm.va_end(i8* %ap1112)
  br label %return

return:                                           ; preds = %bb10
  ret void
}

declare void @llvm.va_start(i8*) nounwind

declare void @llvm.va_end(i8*) nounwind

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  call void (i32, ...)* @assign(i32 2, void ()* @A, void ()* @B) nounwind
  store i32 1, i32* %0, align 4
  %1 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %1, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

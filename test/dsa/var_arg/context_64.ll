; ModuleID = 'context.c'
;--Verify 'ret1' and 'ret2' don't point to the same node (they weren't merged!)
;RUN: dsaopt %s -dsa-local -analyze -check-not-same-node "main:ret1:0,main:ret2:0"
;RUN: dsaopt %s -dsa-bu -analyze -check-not-same-node "main:ret1:0,main:ret2:0"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag = type { i32, i32, i8*, i8* }

define internal i32* @get(i32 %unused, ...) nounwind {
entry:
  %unused_addr = alloca i32                       ; <i32*> [#uses=1]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %addr.0 = alloca i8*                            ; <i8**> [#uses=3]
  %ap = alloca [1 x %struct.__va_list_tag]        ; <[1 x %struct.__va_list_tag]*> [#uses=9]
  %ret = alloca i32*                              ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %unused, i32* %unused_addr
  %ap1 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap12 = bitcast %struct.__va_list_tag* %ap1 to i8* ; <i8*> [#uses=1]
  call void @llvm.va_start(i8* %ap12)
  %1 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %2 = getelementptr inbounds %struct.__va_list_tag* %1, i32 0, i32 0 ; <i32*> [#uses=1]
  %3 = load i32* %2, align 8                      ; <i32> [#uses=1]
  %4 = icmp uge i32 %3, 48                        ; <i1> [#uses=1]
  br i1 %4, label %bb3, label %bb

bb:                                               ; preds = %entry
  %5 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %6 = getelementptr inbounds %struct.__va_list_tag* %5, i32 0, i32 3 ; <i8**> [#uses=1]
  %7 = load i8** %6, align 8                      ; <i8*> [#uses=1]
  %8 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %9 = getelementptr inbounds %struct.__va_list_tag* %8, i32 0, i32 0 ; <i32*> [#uses=1]
  %10 = load i32* %9, align 8                     ; <i32> [#uses=1]
  %11 = inttoptr i32 %10 to i8*                   ; <i8*> [#uses=1]
  %12 = ptrtoint i8* %7 to i64                    ; <i64> [#uses=1]
  %13 = ptrtoint i8* %11 to i64                   ; <i64> [#uses=1]
  %14 = add i64 %12, %13                          ; <i64> [#uses=1]
  %15 = inttoptr i64 %14 to i8*                   ; <i8*> [#uses=1]
  store i8* %15, i8** %addr.0, align 8
  %16 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %17 = getelementptr inbounds %struct.__va_list_tag* %16, i32 0, i32 0 ; <i32*> [#uses=1]
  %18 = load i32* %17, align 8                    ; <i32> [#uses=1]
  %19 = add i32 %18, 8                            ; <i32> [#uses=1]
  %20 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %21 = getelementptr inbounds %struct.__va_list_tag* %20, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 %19, i32* %21, align 8
  br label %bb4

bb3:                                              ; preds = %entry
  %22 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %23 = getelementptr inbounds %struct.__va_list_tag* %22, i32 0, i32 2 ; <i8**> [#uses=1]
  %24 = load i8** %23, align 8                    ; <i8*> [#uses=2]
  store i8* %24, i8** %addr.0, align 8
  %25 = getelementptr inbounds i8* %24, i64 8     ; <i8*> [#uses=1]
  %26 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %27 = getelementptr inbounds %struct.__va_list_tag* %26, i32 0, i32 2 ; <i8**> [#uses=1]
  store i8* %25, i8** %27, align 8
  br label %bb4

bb4:                                              ; preds = %bb3, %bb
  %28 = load i8** %addr.0, align 8                ; <i8*> [#uses=1]
  %29 = bitcast i8* %28 to i32**                  ; <i32**> [#uses=1]
  %30 = load i32** %29, align 8                   ; <i32*> [#uses=1]
  store i32* %30, i32** %ret, align 8
  %ap5 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap56 = bitcast %struct.__va_list_tag* %ap5 to i8* ; <i8*> [#uses=1]
  call void @llvm.va_end(i8* %ap56)
  %31 = load i32** %ret, align 8                  ; <i32*> [#uses=1]
  store i32* %31, i32** %0, align 8
  %32 = load i32** %0, align 8                    ; <i32*> [#uses=1]
  store i32* %32, i32** %retval, align 8
  br label %return

return:                                           ; preds = %bb4
  %retval7 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval7
}

declare void @llvm.va_start(i8*) nounwind

declare void @llvm.va_end(i8*) nounwind

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=3]
  %val1 = alloca i32                              ; <i32*> [#uses=2]
  %val2 = alloca i32                              ; <i32*> [#uses=2]
  %p1 = alloca i32*                               ; <i32**> [#uses=2]
  %p2 = alloca i32*                               ; <i32**> [#uses=2]
  %ret1 = alloca i32*                             ; <i32**> [#uses=2]
  %ret2 = alloca i32*                             ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 1, i32* %val1, align 4
  store i32 2, i32* %val2, align 4
  store i32* %val1, i32** %p1, align 8
  store i32* %val2, i32** %p2, align 8
  %1 = load i32** %p1, align 8                    ; <i32*> [#uses=1]
  %2 = call i32* (i32, ...)* @get(i32 0, i32* %1) nounwind ; <i32*> [#uses=1]
  store i32* %2, i32** %ret1, align 8
  %3 = load i32** %p2, align 8                    ; <i32*> [#uses=1]
  %4 = call i32* (i32, ...)* @get(i32 0, i32* %3) nounwind ; <i32*> [#uses=1]
  store i32* %4, i32** %ret2, align 8
  %5 = load i32** %ret1, align 8                  ; <i32*> [#uses=1]
  %6 = load i32* %5, align 4                      ; <i32> [#uses=1]
  %7 = add nsw i32 %6, 1                          ; <i32> [#uses=1]
  %8 = load i32** %ret2, align 8                  ; <i32*> [#uses=1]
  %9 = load i32* %8, align 4                      ; <i32> [#uses=1]
  %10 = icmp eq i32 %7, %9                        ; <i1> [#uses=1]
  br i1 %10, label %bb, label %bb1

bb:                                               ; preds = %entry
  store i32 0, i32* %0, align 4
  br label %bb2

bb1:                                              ; preds = %entry
  store i32 -1, i32* %0, align 4
  br label %bb2

bb2:                                              ; preds = %bb1, %bb
  %11 = load i32* %0, align 4                     ; <i32> [#uses=1]
  store i32 %11, i32* %retval, align 4
  br label %return

return:                                           ; preds = %bb2
  %retval3 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval3
}

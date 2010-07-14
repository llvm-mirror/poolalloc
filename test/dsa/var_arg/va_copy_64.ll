; ModuleID = 'va_copy.c'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
; Verify all the passes run on this
;RUN: dsaopt %s -dsa-local -disable-output
;RUN: dsaopt %s -dsa-bu -disable-output
;RUN: dsaopt %s -dsa-td -disable-output
;RUN: dsaopt %s -dsa-eq -disable-output
; Verify that val1 and val2 are merged
;RUN: dsaopt %s -dsa-local -analyze -check-same-node "val1:0,val2:0"

%struct.__va_list_tag = type { i32, i32, i8*, i8* }

@val1 = common global i32* null                   ; <i32**> [#uses=2]
@val2 = common global i32* null                   ; <i32**> [#uses=2]

define internal i32 @get(i32 %unused, ...) nounwind {
entry:
  %unused_addr = alloca i32                       ; <i32*> [#uses=1]
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %addr.2 = alloca i8*                            ; <i8**> [#uses=3]
  %addr.0 = alloca i8*                            ; <i8**> [#uses=3]
  %ap = alloca [1 x %struct.__va_list_tag]        ; <[1 x %struct.__va_list_tag]*> [#uses=10]
  %ap_copy = alloca [1 x %struct.__va_list_tag]   ; <[1 x %struct.__va_list_tag]*> [#uses=9]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %unused, i32* %unused_addr
  %ap1 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap12 = bitcast %struct.__va_list_tag* %ap1 to i8* ; <i8*> [#uses=1]
  call void @llvm.va_start(i8* %ap12)
  %ap_copy3 = bitcast [1 x %struct.__va_list_tag]* %ap_copy to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap4 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap_copy35 = bitcast %struct.__va_list_tag* %ap_copy3 to i8* ; <i8*> [#uses=1]
  %ap46 = bitcast %struct.__va_list_tag* %ap4 to i8* ; <i8*> [#uses=1]
  call void @llvm.va_copy(i8* %ap_copy35, i8* %ap46)
  %1 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %2 = getelementptr inbounds %struct.__va_list_tag* %1, i32 0, i32 0 ; <i32*> [#uses=1]
  %3 = load i32* %2, align 8                      ; <i32> [#uses=1]
  %4 = icmp uge i32 %3, 48                        ; <i1> [#uses=1]
  br i1 %4, label %bb7, label %bb

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
  br label %bb8

bb7:                                              ; preds = %entry
  %22 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %23 = getelementptr inbounds %struct.__va_list_tag* %22, i32 0, i32 2 ; <i8**> [#uses=1]
  %24 = load i8** %23, align 8                    ; <i8*> [#uses=2]
  store i8* %24, i8** %addr.0, align 8
  %25 = getelementptr inbounds i8* %24, i64 8     ; <i8*> [#uses=1]
  %26 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %27 = getelementptr inbounds %struct.__va_list_tag* %26, i32 0, i32 2 ; <i8**> [#uses=1]
  store i8* %25, i8** %27, align 8
  br label %bb8

bb8:                                              ; preds = %bb7, %bb
  %28 = load i8** %addr.0, align 8                ; <i8*> [#uses=1]
  %29 = bitcast i8* %28 to i32**                  ; <i32**> [#uses=1]
  %30 = load i32** %29, align 8                   ; <i32*> [#uses=1]
  store i32* %30, i32** @val1, align 8
  %ap9 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap910 = bitcast %struct.__va_list_tag* %ap9 to i8* ; <i8*> [#uses=1]
  call void @llvm.va_end(i8* %ap910)
  %31 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap_copy, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %32 = getelementptr inbounds %struct.__va_list_tag* %31, i32 0, i32 0 ; <i32*> [#uses=1]
  %33 = load i32* %32, align 8                    ; <i32> [#uses=1]
  %34 = icmp uge i32 %33, 48                      ; <i1> [#uses=1]
  br i1 %34, label %bb12, label %bb11

bb11:                                             ; preds = %bb8
  %35 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap_copy, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %36 = getelementptr inbounds %struct.__va_list_tag* %35, i32 0, i32 3 ; <i8**> [#uses=1]
  %37 = load i8** %36, align 8                    ; <i8*> [#uses=1]
  %38 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap_copy, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %39 = getelementptr inbounds %struct.__va_list_tag* %38, i32 0, i32 0 ; <i32*> [#uses=1]
  %40 = load i32* %39, align 8                    ; <i32> [#uses=1]
  %41 = inttoptr i32 %40 to i8*                   ; <i8*> [#uses=1]
  %42 = ptrtoint i8* %37 to i64                   ; <i64> [#uses=1]
  %43 = ptrtoint i8* %41 to i64                   ; <i64> [#uses=1]
  %44 = add i64 %42, %43                          ; <i64> [#uses=1]
  %45 = inttoptr i64 %44 to i8*                   ; <i8*> [#uses=1]
  store i8* %45, i8** %addr.2, align 8
  %46 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap_copy, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %47 = getelementptr inbounds %struct.__va_list_tag* %46, i32 0, i32 0 ; <i32*> [#uses=1]
  %48 = load i32* %47, align 8                    ; <i32> [#uses=1]
  %49 = add i32 %48, 8                            ; <i32> [#uses=1]
  %50 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap_copy, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %51 = getelementptr inbounds %struct.__va_list_tag* %50, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 %49, i32* %51, align 8
  br label %bb13

bb12:                                             ; preds = %bb8
  %52 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap_copy, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %53 = getelementptr inbounds %struct.__va_list_tag* %52, i32 0, i32 2 ; <i8**> [#uses=1]
  %54 = load i8** %53, align 8                    ; <i8*> [#uses=2]
  store i8* %54, i8** %addr.2, align 8
  %55 = getelementptr inbounds i8* %54, i64 8     ; <i8*> [#uses=1]
  %56 = getelementptr inbounds [1 x %struct.__va_list_tag]* %ap_copy, i64 0, i64 0 ; <%struct.__va_list_tag*> [#uses=1]
  %57 = getelementptr inbounds %struct.__va_list_tag* %56, i32 0, i32 2 ; <i8**> [#uses=1]
  store i8* %55, i8** %57, align 8
  br label %bb13

bb13:                                             ; preds = %bb12, %bb11
  %58 = load i8** %addr.2, align 8                ; <i8*> [#uses=1]
  %59 = bitcast i8* %58 to i32**                  ; <i32**> [#uses=1]
  %60 = load i32** %59, align 8                   ; <i32*> [#uses=1]
  store i32* %60, i32** @val2, align 8
  %ap_copy14 = bitcast [1 x %struct.__va_list_tag]* %ap_copy to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap_copy1415 = bitcast %struct.__va_list_tag* %ap_copy14 to i8* ; <i8*> [#uses=1]
  call void @llvm.va_end(i8* %ap_copy1415)
  %61 = load i32** @val1, align 8                 ; <i32*> [#uses=1]
  %62 = load i32* %61, align 4                    ; <i32> [#uses=1]
  %63 = load i32** @val2, align 8                 ; <i32*> [#uses=1]
  %64 = load i32* %63, align 4                    ; <i32> [#uses=1]
  %65 = sub nsw i32 %62, %64                      ; <i32> [#uses=1]
  store i32 %65, i32* %0, align 4
  %66 = load i32* %0, align 4                     ; <i32> [#uses=1]
  store i32 %66, i32* %retval, align 4
  br label %return

return:                                           ; preds = %bb13
  %retval16 = load i32* %retval                   ; <i32> [#uses=1]
  ret i32 %retval16
}

declare void @llvm.va_start(i8*) nounwind

declare void @llvm.va_copy(i8*, i8*) nounwind

declare void @llvm.va_end(i8*) nounwind

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %stack_val = alloca i32                         ; <i32*> [#uses=2]
  %ret = alloca i32                               ; <i32*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 5, i32* %stack_val, align 4
  %1 = call i32 (i32, ...)* @get(i32 0, i32* %stack_val) nounwind ; <i32> [#uses=1]
  store i32 %1, i32* %ret, align 4
  %2 = load i32* %ret, align 4                    ; <i32> [#uses=1]
  store i32 %2, i32* %0, align 4
  %3 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %3, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

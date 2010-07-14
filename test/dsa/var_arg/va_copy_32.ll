; ModuleID = 'va_copy.c'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-f80:32:32-n8:16:32"
target triple = "i386-unknown-linux-gnu"
; Verify all the passes run on this
;RUN: dsaopt %s -dsa-local -disable-output
;RUN: dsaopt %s -dsa-bu -disable-output
;RUN: dsaopt %s -dsa-td -disable-output
;RUN: dsaopt %s -dsa-eq -disable-output
; Verify that val1 and val2 are merged
;RUN: dsaopt %s -dsa-local -analyze -check-same-node "val1:0,val2:0"


@val1 = common global i32* null                   ; <i32**> [#uses=2]
@val2 = common global i32* null                   ; <i32**> [#uses=2]

define internal i32 @get(i32 %unused, ...) nounwind {
entry:
  %unused_addr = alloca i32                       ; <i32*> [#uses=1]
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %ap_copy.3 = alloca i8*                         ; <i8**> [#uses=3]
  %ap.1 = alloca i8*                              ; <i8**> [#uses=3]
  %ap = alloca i8*                                ; <i8**> [#uses=5]
  %ap_copy = alloca i8*                           ; <i8**> [#uses=4]
  %memtmp = alloca i8*                            ; <i8**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %unused, i32* %unused_addr
  %ap1 = bitcast i8** %ap to i8*                  ; <i8*> [#uses=1]
  call void @llvm.va_start(i8* %ap1)
  %1 = load i8** %ap, align 4                     ; <i8*> [#uses=1]
  store i8* %1, i8** %memtmp
  %ap_copy2 = bitcast i8** %ap_copy to i8*        ; <i8*> [#uses=1]
  %memtmp3 = bitcast i8** %memtmp to i8*          ; <i8*> [#uses=1]
  call void @llvm.va_copy(i8* %ap_copy2, i8* %memtmp3)
  %2 = load i8** %ap, align 4                     ; <i8*> [#uses=1]
  store i8* %2, i8** %ap.1, align 4
  %3 = load i8** %ap.1, align 4                   ; <i8*> [#uses=1]
  %4 = getelementptr inbounds i8* %3, i64 4       ; <i8*> [#uses=1]
  store i8* %4, i8** %ap, align 4
  %5 = load i8** %ap.1, align 4                   ; <i8*> [#uses=1]
  %6 = bitcast i8* %5 to i32**                    ; <i32**> [#uses=1]
  %7 = load i32** %6, align 4                     ; <i32*> [#uses=1]
  store i32* %7, i32** @val1, align 4
  %ap4 = bitcast i8** %ap to i8*                  ; <i8*> [#uses=1]
  call void @llvm.va_end(i8* %ap4)
  %8 = load i8** %ap_copy, align 4                ; <i8*> [#uses=1]
  store i8* %8, i8** %ap_copy.3, align 4
  %9 = load i8** %ap_copy.3, align 4              ; <i8*> [#uses=1]
  %10 = getelementptr inbounds i8* %9, i64 4      ; <i8*> [#uses=1]
  store i8* %10, i8** %ap_copy, align 4
  %11 = load i8** %ap_copy.3, align 4             ; <i8*> [#uses=1]
  %12 = bitcast i8* %11 to i32**                  ; <i32**> [#uses=1]
  %13 = load i32** %12, align 4                   ; <i32*> [#uses=1]
  store i32* %13, i32** @val2, align 4
  %ap_copy5 = bitcast i8** %ap_copy to i8*        ; <i8*> [#uses=1]
  call void @llvm.va_end(i8* %ap_copy5)
  %14 = load i32** @val1, align 4                 ; <i32*> [#uses=1]
  %15 = load i32* %14, align 4                    ; <i32> [#uses=1]
  %16 = load i32** @val2, align 4                 ; <i32*> [#uses=1]
  %17 = load i32* %16, align 4                    ; <i32> [#uses=1]
  %18 = sub nsw i32 %15, %17                      ; <i32> [#uses=1]
  store i32 %18, i32* %0, align 4
  %19 = load i32* %0, align 4                     ; <i32> [#uses=1]
  store i32 %19, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval6 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval6
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

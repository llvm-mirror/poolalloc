; ModuleID = 'context.c'
;--Verify 'ret1' and 'ret2' don't point to the same node (they weren't merged!)
;RUN: dsaopt %s -dsa-local -analyze -check-not-same-node "main:ret1:0,main:ret2:0"
;RUN: dsaopt %s -dsa-bu -analyze -check-not-same-node "main:ret1:0,main:ret2:0"

target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-f80:32:32-n8:16:32"
target triple = "i386-unknown-linux-gnu"

define internal i32* @get(i32 %unused, ...) nounwind {
entry:
  %unused_addr = alloca i32                       ; <i32*> [#uses=1]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %ap.0 = alloca i8*                              ; <i8**> [#uses=3]
  %ap = alloca i8*                                ; <i8**> [#uses=4]
  %ret = alloca i32*                              ; <i32**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %unused, i32* %unused_addr
  %ap1 = bitcast i8** %ap to i8*                  ; <i8*> [#uses=1]
  call void @llvm.va_start(i8* %ap1)
  %1 = load i8** %ap, align 4                     ; <i8*> [#uses=1]
  store i8* %1, i8** %ap.0, align 4
  %2 = load i8** %ap.0, align 4                   ; <i8*> [#uses=1]
  %3 = getelementptr inbounds i8* %2, i64 4       ; <i8*> [#uses=1]
  store i8* %3, i8** %ap, align 4
  %4 = load i8** %ap.0, align 4                   ; <i8*> [#uses=1]
  %5 = bitcast i8* %4 to i32**                    ; <i32**> [#uses=1]
  %6 = load i32** %5, align 4                     ; <i32*> [#uses=1]
  store i32* %6, i32** %ret, align 4
  %ap2 = bitcast i8** %ap to i8*                  ; <i8*> [#uses=1]
  call void @llvm.va_end(i8* %ap2)
  %7 = load i32** %ret, align 4                   ; <i32*> [#uses=1]
  store i32* %7, i32** %0, align 4
  %8 = load i32** %0, align 4                     ; <i32*> [#uses=1]
  store i32* %8, i32** %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval3 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval3
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
  store i32* %val1, i32** %p1, align 4
  store i32* %val2, i32** %p2, align 4
  %1 = load i32** %p1, align 4                    ; <i32*> [#uses=1]
  %2 = call i32* (i32, ...)* @get(i32 0, i32* %1) nounwind ; <i32*> [#uses=1]
  store i32* %2, i32** %ret1, align 4
  %3 = load i32** %p2, align 4                    ; <i32*> [#uses=1]
  %4 = call i32* (i32, ...)* @get(i32 0, i32* %3) nounwind ; <i32*> [#uses=1]
  store i32* %4, i32** %ret2, align 4
  %5 = load i32** %ret1, align 4                  ; <i32*> [#uses=1]
  %6 = load i32* %5, align 4                      ; <i32> [#uses=1]
  %7 = add nsw i32 %6, 1                          ; <i32> [#uses=1]
  %8 = load i32** %ret2, align 4                  ; <i32*> [#uses=1]
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

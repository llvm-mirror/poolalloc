; ModuleID = 'basic.c'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-f80:32:32-n8:16:32"
target triple = "i386-unknown-linux-gnu"
; Verify all the passes run on this
;RUN: dsaopt %s -dsa-local -disable-output
;RUN: dsaopt %s -dsa-bu -disable-output
;RUN: dsaopt %s -dsa-td -disable-output
;RUN: dsaopt %s -dsa-eq -disable-output
;=== Local Tests ===
; Verify VAStart flag
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "get:ap+V"
; Don't propagate the VAStart flag to children...
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "get:ap:0-V"
; On 32-bit, VAStart node should point to an array of the arguments
;RUN: dsaopt %s -dsa-local -analyze -check-type=get:ap:0,FoldedVOID
; And the argument node should be of the proper type (in this case an i32)
;RUN: dsaopt %s -dsa-local -analyze -check-type=get:ap:0:0,0:i32
; The argument should be ref'd, but not mod'd
;RUN: dsaopt %s -dsa-local -analyze --verify-flags "get:ap:0:0+R-M"

define internal i32 @get(i32 %unused, ...) nounwind {
entry:
  %unused_addr = alloca i32                       ; <i32*> [#uses=1]
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %ap.0 = alloca i8*                              ; <i8**> [#uses=3]
  %ap = alloca i8*                                ; <i8**> [#uses=4]
  %val = alloca i32*                              ; <i32**> [#uses=2]
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
  store i32* %6, i32** %val, align 4
  %ap2 = bitcast i8** %ap to i8*                  ; <i8*> [#uses=1]
  call void @llvm.va_end(i8* %ap2)
  %7 = load i32** %val, align 4                   ; <i32*> [#uses=1]
  %8 = load i32* %7, align 4                      ; <i32> [#uses=1]
  store i32 %8, i32* %0, align 4
  %9 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %9, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval3 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval3
}

declare void @llvm.va_start(i8*) nounwind

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
  %3 = sub nsw i32 %2, 5                          ; <i32> [#uses=1]
  store i32 %3, i32* %0, align 4
  %4 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %4, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

; Same as basic_32.ll and basic_64.ll, but uses
; the 'va_arg' instruction directly.
; Verify all the passes run on this
;RUN: dsaopt %s -dsa-local -disable-output
;RUN: dsaopt %s -dsa-bu -disable-output
;RUN: dsaopt %s -dsa-td -disable-output
;RUN: dsaopt %s -dsa-eq -disable-output
;=== Local Tests ===
; Verify VAStart flag
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "get:ap+V"
; Don't propagate the VAStart flag to children...
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "get:ap:8-V"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "get:ap:16-V"
; On 64-bit, VAStart node should be a struct pointing to an array of the arguments
;RUN: dsaopt %s -dsa-local -analyze -check-type=get:ap:8,FoldedVOID
; Both offsets 8 and 16 point to the array as a simplification.
; Verify that these are indeed the same node.
;RUN: dsaopt %s -dsa-local -analyze -check-same-node \
;RUN: "get:ap:8,get:ap:16"
; And the argument node should be of the proper type (in this case an i32)
;RUN: dsaopt %s -dsa-local -analyze -check-type=get:ap:8:0,0:i32
; The argument should be ref'd and mod'd.
; NOTE: This behavior differs from that of the non-instruction version.
; Investigate?
;RUN: dsaopt %s -dsa-local -analyze --verify-flags "get:ap:8:0+R+M"
; ModuleID = 'basic.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag = type { i32, i32, i8*, i8* }

define internal i32 @get(i32 %unused, ...) nounwind {
entry:
  %unused_addr = alloca i32                       ; <i32*> [#uses=1]
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %ap = alloca [1 x %struct.__va_list_tag]        ; <[1 x %struct.__va_list_tag]*> [#uses=4]
  %val = alloca i32*                              ; <i32**> [#uses=2]
  %val1 = alloca i8*                              ; <i8**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %unused, i32* %unused_addr
  %ap1 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap12 = bitcast %struct.__va_list_tag* %ap1 to i8* ; <i8*> [#uses=1]
  call void @llvm.va_start(i8* %ap12)
  %ap3 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %1 = va_arg %struct.__va_list_tag* %ap3, i32*   ; <i32*> [#uses=1]
  store i32* %1, i32** %val, align 8
  %ap4 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %2 = va_arg %struct.__va_list_tag* %ap4, i8*    ; <i8*> [#uses=1]
  store i8* %2, i8** %val1, align 8
  %ap5 = bitcast [1 x %struct.__va_list_tag]* %ap to %struct.__va_list_tag* ; <%struct.__va_list_tag*> [#uses=1]
  %ap56 = bitcast %struct.__va_list_tag* %ap5 to i8* ; <i8*> [#uses=1]
  call void @llvm.va_end(i8* %ap56)
  %3 = load i32** %val, align 8                   ; <i32*> [#uses=1]
  %4 = load i32* %3, align 4                      ; <i32> [#uses=1]
  store i32 %4, i32* %0, align 4
  %5 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %5, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval7 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval7
}

declare void @llvm.va_start(i8*) nounwind

declare void @llvm.va_end(i8*) nounwind

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %stack_val = alloca i32                         ; <i32*> [#uses=2]
  %stack_val1 = alloca i8                         ; <i8*> [#uses=2]
  %ret = alloca i32                               ; <i32*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 5, i32* %stack_val, align 4
  store i8 97, i8* %stack_val1, align 1
  %1 = call i32 (i32, ...)* @get(i32 0, i32* %stack_val, i8* %stack_val1) nounwind ; <i32> [#uses=1]
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

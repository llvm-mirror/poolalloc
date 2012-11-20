; Verify that TD works as expected
; Verify call edges are found
; RUN: dsaopt %s -dsa-td -analyze -check-callees=indirect,foo,bar

; Verify aliasing with @G is certainly known in @main
; We can't do this with current testing, so instead look for +G
; This should be known by end of BU
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags=@main:barptr+G
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags=@main:fooptr2+G
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags=@main:barptr2+G
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@main:barptr+G
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@main:fooptr2+G
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@main:barptr2+G

; Verify %ptr in both @foo and @bar is also known to alias a global,
; and that this isn't known until TD is run (BU shouldn't know this yet)
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags=@foo:ptr-G
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags=@bar:ptr-G
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@foo:ptr+G
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@bar:ptr+G
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@G = global i8 0

define i8* @foo(i8* %ptr) nounwind {
  ret i8* %ptr
}

define i8* @bar(i8* %ptr) nounwind {
  ret i8* @G
}

define i8* @indirect(i8* (i8*) * %fp, i8* %ptr) {
  %retptr = call i8* %fp(i8* %ptr)
  ret i8* %retptr
}

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
  ; Conjure some i8*
  %ptr = load i8** %argv, align 8

  ; %fooptr = %ptr
  %fooptr = call i8* @indirect(i8* (i8*)* @foo, i8* %ptr)
  ; %barptr = @G
  %barptr = call i8* @indirect(i8* (i8*)* @bar, i8* %fooptr)
  ; fooptr2 = @G
  %fooptr2 = call i8* @indirect(i8* (i8*)* @foo, i8* %barptr)
  ; barptr2 = @G
  %barptr2 = call i8* @indirect(i8* (i8*)* @bar, i8* %fooptr2)

  ret i32 0
}

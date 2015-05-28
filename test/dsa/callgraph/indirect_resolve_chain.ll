; This test fails for now, since TD reports same callgraph as BU.
;XFAIL: *

; BU should be able to find the callees of this indirect-indirect chain
; Test that it does:
; RUN: dsaopt %s -dsa-bu -analyze -check-callees=main,bounce,bounce2
; RUN: dsaopt %s -dsa-bu -analyze -check-callees=bounce,call
; RUN: dsaopt %s -dsa-bu -analyze -check-callees=bounce2,call2
; RUN: dsaopt %s -dsa-bu -analyze -check-callees=call,foo
; RUN: dsaopt %s -dsa-bu -analyze -check-callees=call2,bar
; Note that the indirect nodes will still be incomplete, however:
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags=@bounce:target+I
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags=@bounce2:target+I
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags=@call:target+I
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags=@call2:target+I
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags=@foo:ptr+I
; RUN: dsaopt %s -dsa-bu -analyze -verify-flags=@bar:ptr+I

; TD only is able to 'complete' one level deep.
; Check flags:
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@bounce:target-I
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@bounce2:target-I
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@call:target+I
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@call2:target+I
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@foo:ptr+I
; RUN: dsaopt %s -dsa-td -analyze -verify-flags=@bar:ptr+I

; And check TD's callgraph is superset of correct callgraph (as above):
; RUN: dsaopt %s -dsa-td -analyze -check-callees=main,bounce,bounce2
; RUN: dsaopt %s -dsa-td -analyze -check-callees=bounce,call
; RUN: dsaopt %s -dsa-td -analyze -check-callees=bounce2,call2
; RUN: dsaopt %s -dsa-td -analyze -check-callees=call,foo
; RUN: dsaopt %s -dsa-td -analyze -check-callees=call2,bar
; TD resolves one level deep:
; RUN: dsaopt %s -dsa-td -analyze -check-callees=bounce,call
; RUN: dsaopt %s -dsa-td -analyze -check-not-callees=bounce,main,foo,bar,call2
; RUN: dsaopt %s -dsa-td -analyze -check-callees=bounce2,call2
; RUN: dsaopt %s -dsa-td -analyze -check-not-callees=bounce2,main,foo,bar,call
; But not two levels deep:
; RUN: dsaopt %s -dsa-td -analyze -check-callees=call,foo,bar
; RUN: dsaopt %s -dsa-td -analyze -check-callees=call2,foo,bar

define internal i8* @foo(i8* %ptr) {
entry:
  ret i8* %ptr
}
define internal i8* @bar(i8* %ptr) {
entry:
  ret i8* %ptr
}

define i8* @call(i8 *(i8*)* %target, i8* %arg) {
entry:
  %retptr = call i8* %target(i8* %arg)
  ret i8* %retptr
}

define i8* @call2(i8 *(i8*)* %target, i8* %arg) {
entry:
  %retptr = call i8* %target(i8* %arg)
  ret i8* %retptr
}

define internal i8* @bounce(i8 *(i8* (i8*)*, i8*)* %target, i8 *(i8*)* %fp, i8* %arg) {
entry:
  %retptr = call i8* %target(i8 *(i8*)* %fp, i8* %arg)
  ret i8* %retptr
}

define internal i8* @bounce2(i8 *(i8* (i8*)*, i8*)* %target, i8 *(i8*)* %fp, i8* %arg) {
entry:
  %retptr = call i8* %target(i8 *(i8*)* %fp, i8* %arg)
  ret i8* %retptr
}

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  ; Conjure some i8*
  %ptr = load i8*, i8** %argv, align 8

  %ptr2 = call i8* @bounce(i8 *(i8* (i8*)*, i8*)* @call, i8 *(i8*)* @foo, i8* %ptr)
  %ptr3 = call i8* @bounce2(i8 *(i8* (i8*)*, i8*)* @call2, i8 *(i8*)* @bar, i8* %ptr)

  %val = load i8, i8* %ptr2

  ret i32 0
}

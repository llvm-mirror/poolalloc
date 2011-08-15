; ModuleID = 'bugpoint-reduced-simplified.bc'
; Typechecks should fail, since we don't have a main.
; RUN: not adsaopt -internalize -mem2reg -typechecks %s 2> %t.err >/dev/null
; RUN: grep "Assertion.*No main function found" %t.err
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define void @fmtfp() nounwind uwtable {
entry:
  unreachable
}

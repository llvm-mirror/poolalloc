;RUN: dsaopt %s -dsa-td -analyze -check-callees=main,A
;RUN: dsaopt %s -dsa-td -analyze -check-not-callees=main
; Verify -check-not-callee behavior as error if any not all
; specified callees are reported by the callgraph.
; Test this using an empty set of callees.
; ModuleID = 'calltargets.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define internal void @A() nounwind {
entry:
	ret void
}

define internal void @B() nounwind {
entry:
	ret void
}

define void @main() nounwind {
entry:
	call void @A() nounwind
	ret void
}


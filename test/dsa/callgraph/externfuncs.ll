target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

; Things tested here:
; * Direct calls to extern functions
; * Indirect calls to extern functions
; * Indirect calls through extern nodes
; Presently this test is a bit overkill since we say external call nodes call
; call functions, apparently.
; However, this test does verify we at least model the actual dynamic behavior

declare i32* @externFunc() nounwind

;RUN: dsaopt %s -dsa-bu -analyze -check-callees=callExternFunc,externFunc
;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=callExternFunc,externFunc
;RUN: dsaopt %s -dsa-td -analyze -check-callees=callExternFunc,externFunc
;RUN: dsaopt %s -dsa-eq -analyze -check-callees=callExternFunc,externFunc
;RUN: dsaopt %s -dsa-eqtd -analyze -check-callees=callExternFunc,externFunc
define internal void @callExternFunc() nounwind {
entry:
  %0 = call i32* @externFunc() nounwind
  ret void
}

declare i32* @externFunc2() nounwind

;RUN: dsaopt %s -dsa-bu -analyze -check-callees=callIndirectExternFunc,externFunc2
;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=callIndirectExternFunc,externFunc2
;RUN: dsaopt %s -dsa-td -analyze -check-callees=callIndirectExternFunc,externFunc2
;RUN: dsaopt %s -dsa-eq -analyze -check-callees=callIndirectExternFunc,externFunc2
;RUN: dsaopt %s -dsa-eqtd -analyze -check-callees=callIndirectExternFunc,externFunc2
define internal void @callIndirectExternFunc() nounwind {
entry:
  %0 = alloca i32* ()*
  store i32* ()* @externFunc2, i32* ()** %0
  %fp = load i32* ()** %0
  %res = call i32* ()* %fp() nounwind
  ret void
}


declare i32* ()* @externFunc3() nounwind
declare i32* @externFunc4() nounwind

;RUN: dsaopt %s -dsa-bu -analyze -check-callees=getExternFP,externFunc3,externFunc4
;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=getExternFP,externFunc3,externFunc4
;RUN: dsaopt %s -dsa-td -analyze -check-callees=getExternFP,externFunc3,externFunc4
;RUN: dsaopt %s -dsa-eq -analyze -check-callees=getExternFP,externFunc3,externFunc4
;RUN: dsaopt %s -dsa-eqtd -analyze -check-callees=getExternFP,externFunc3,externFunc4
define i32* ()* @getExternFP() nounwind {
entry:
  %0 = alloca i32* ()*
  %1 = call i32* ()* ()* @externFunc3() nounwind
  %2 = call i32* ()* %1()
  %3 = call i32* ()* ()* @externFunc3() nounwind
  store i32* ()* %3, i32* ()** %0
  store i32* ()* @externFunc4, i32* ()** %0
  %res = load i32* ()** %0
  ret i32* ()* %res
}

;RUN: dsaopt %s -dsa-bu -analyze -check-callees=callThroughExternFP,getExternFP,externFunc4
;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=callThroughExternFP,getExternFP,externFunc4
;RUN: dsaopt %s -dsa-td -analyze -check-callees=callThroughExternFP,getExternFP,externFunc4
;RUN: dsaopt %s -dsa-eq -analyze -check-callees=callThroughExternFP,getExternFP,externFunc4
;RUN: dsaopt %s -dsa-eqtd -analyze -check-callees=callThroughExternFP,getExternFP,externFunc4
define void @callThroughExternFP() nounwind {
entry:
  %0 = call i32* ()* ()* @getExternFP() nounwind
  %1 = call i32* %0() nounwind
  ret void
}


; Very basic -calltarget-{,eq}td pass testing.

;RUN: dsaopt %s -calltarget-td -analyze | FileCheck %s
;RUN: dsaopt %s -calltarget-eqtd -analyze | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

; Notes:
; * "A" and "B" are in an SCC so callgraph reports both when
;   calling one of them.
; * Both calls in @B are reported to potentially call A and
;   C due to lack of flow-sensitivity.

define internal void @C() nounwind {
entry:
  ret void
}

define internal void @A() nounwind {
entry:
  ; indirect call to B()
  %Bfp = alloca void ()*
  store void ()* @B, void ()** %Bfp, align 8
  %B = load void ()*, void ()** %Bfp, align 8

;CHECK-DAG: call void %B() #0: A B
  call void %B() nounwind
  ret void
}

define internal void @B() nounwind {
entry:
  %fp = alloca void ()*

  ; indirect call to A()
  store void ()* @A, void ()** %fp, align 8
  %A = load void ()*, void ()** %fp, align 8
;CHECK-DAG: call void %A() #0: A B C
  call void %A() nounwind

  ; indirect call to C()
  store void ()* @C, void ()** %fp, align 8
  %C = load void ()*, void ()** %fp, align 8
;CHECK-DAG: call void %C() #0: A B C
  call void %C() nounwind
  ret void
}


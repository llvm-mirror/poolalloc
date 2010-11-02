;This test does some very basic checking on the attribute copying of arguments
;RUN: paopt %s -paheur-AllButUnreachableFromMemory -poolalloc -o %t.bc |& grep "Pool allocating.*nodes!"
;RUN: llvm-dis %t.bc -o %t.ll
;Make sure 'nocapture' attribute isn't copied to new PD argument
;RUN: cat %t.ll | grep -v ".*@.*(.*nocapture.*,.*,.*)"
;But ensure the other arguments have their original attributes
;RUN: cat %t.ll | grep ".*@.*(.*,.*zeroext.*,.*nocapture.*)"
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define i32 @main(i32 %argc, i8** nocapture %argv) nounwind {
entry:
  unreachable
}

define internal void @attr(i16 zeroext %IntParam, i8** nocapture %NeedsPool) {
entry:
  unreachable
}


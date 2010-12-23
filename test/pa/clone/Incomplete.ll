;This test checks that functions callable from incomplete call sites 
;are not cloned.
;RUN: paopt %s -paheur-AllButUnreachableFromMemory -poolalloc -o %t.bc |& grep "Pool allocating.*nodes!"
;RUN: llvm-dis %t.bc -o %t.ll
;Make sure address taken functions A and C are not cloned
;RUN: cat %t.ll | grep -v "A_clone"
;RUN: cat %t.ll | grep -v "C_clone"
;But ensure that B is cloned.
;RUN: cat %t.ll | grep  "B_clone"

; ModuleID = 'fptr.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

define void @A(void (i8*)** %f) nounwind {
entry:
  %f_addr = alloca void (i8*)**                   ; <void (i8*)***> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store void (i8*)** %f, void (i8*)*** %f_addr
  br label %return

return:                                           ; preds = %entry
  ret void
}

define void @B(void (i8*)** %f) nounwind {
entry:
  %f_addr = alloca void (i8*)**                   ; <void (i8*)***> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store void (i8*)** %f, void (i8*)*** %f_addr
  br label %return

return:                                           ; preds = %entry
  ret void
}

define void @C(void (i8*)** %f) nounwind {
entry:
  %f_addr = alloca void (i8*)**                   ; <void (i8*)***> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store void (i8*)** %f, void (i8*)*** %f_addr
  br label %return

return:                                           ; preds = %entry
  ret void
}

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=1]
  %F = alloca void (i8*)*                         ; <void (i8*)**> [#uses=4]
  %F1 = alloca void (i8*)*                        ; <void (i8*)**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store void (i8*)* bitcast (void (void (i8*)**)* @A to void (i8*)*), void (i8*)** %F, align 8
  store void (i8*)* bitcast (void (void (i8*)**)* @C to void (i8*)*), void (i8*)** %F1, align 8
  %0 = load void (i8*)** %F, align 8              ; <void (i8*)*> [#uses=1]
  %F2 = bitcast void (i8*)** %F to i8*            ; <i8*> [#uses=1]
  call void %0(i8* %F2) nounwind
  call void @B(void (i8*)** %F) nounwind
  br label %return

return:                                           ; preds = %entry
  %retval3 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval3
}

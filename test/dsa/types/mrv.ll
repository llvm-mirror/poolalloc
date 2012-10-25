; ModuleID = 'mrv.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

;RUN: dsaopt %s -dsa-local -analyze -check-type=main:s,0:float\|double::4:float::8:float
;RUN: dsaopt %s -dsa-local -enable-type-inference-opts -analyze -check-type=main:s,0:float\|double::4:float::8:float
;RUN: adsaopt %s -ld-args -deadargelim -dce -o %t.bc
;RUN: dsaopt %t.bc -dsa-local -enable-type-inference-opts -analyze -check-type=main:s,0:float::4:float::8:float
;RUN: adsaopt %s -ld-args -gep-expr-arg -deadargelim -dce -o %t.bc
;RUN: dsaopt %t.bc -dsa-local -enable-type-inference-opts -analyze -check-type=main:s,0:float::4:float::8:float

; Fails because -enable-type-inferences-opts doesn't do what's expected.
; Since that code's unmaintained for now, don't report these as DSA failures.
; XFAIL: *

; Function foo, actually accepts an object of struct S. But as
; per calling conventions, the value is passed in registers, after
; conversion to a float and a double.
; See http://lists.cs.uiuc.edu/pipermail/llvmdev/2010-January/028986.html

%0 = type { double, float }
%struct.S = type { float, float, float }

define void @_Z3foo1S(double %obj.0, float %obj.1) nounwind {
entry:
  %obj_addr = alloca %struct.S                    ; <%struct.S*> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = bitcast %struct.S* %obj_addr to %0*        ; <%0*> [#uses=1]
  %1 = getelementptr inbounds %0* %0, i32 0, i32 0 ; <double*> [#uses=1]
  store double %obj.0, double* %1
  %2 = bitcast %struct.S* %obj_addr to %0*        ; <%0*> [#uses=1]
  %3 = getelementptr inbounds %0* %2, i32 0, i32 1 ; <float*> [#uses=1]
  store float %obj.1, float* %3
  %4 = getelementptr inbounds %struct.S* %obj_addr, i32 0, i32 0 ; <float*> [#uses=1]
  store float 0x3FF3333340000000, float* %4, align 4
  br label %return

return:                                           ; preds = %entry
  ret void
}

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %s = alloca %struct.S                           ; <%struct.S*> [#uses=5]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = getelementptr inbounds %struct.S* %s, i32 0, i32 0 ; <float*> [#uses=1]
  store float 1.000000e+00, float* %1, align 4
  %2 = getelementptr inbounds %struct.S* %s, i32 0, i32 1 ; <float*> [#uses=1]
  store float 1.000000e+00, float* %2, align 4
  %3 = getelementptr inbounds %struct.S* %s, i32 0, i32 2 ; <float*> [#uses=1]
  store float 1.000000e+00, float* %3, align 4
  %4 = bitcast %struct.S* %s to %0*               ; <%0*> [#uses=1]
  %elt = getelementptr inbounds %0* %4, i32 0, i32 0 ; <double*> [#uses=1]
  %val = load double* %elt                        ; <double> [#uses=1]
  %5 = bitcast %struct.S* %s to %0*               ; <%0*> [#uses=1]
  %elt1 = getelementptr inbounds %0* %5, i32 0, i32 1 ; <float*> [#uses=1]
  %val2 = load float* %elt1                       ; <float> [#uses=1]
  call void @_Z3foo1S(double %val, float %val2) nounwind
  store i32 0, i32* %0, align 4
  %6 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %6, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval3 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval3
}

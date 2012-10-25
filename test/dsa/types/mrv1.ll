;similiar to mrv.ll. Also shows conversion for return values.

; ModuleID = 'mrv.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

;RUN: dsaopt %s -dsa-local -analyze -check-type=main:s,0:float\|double::4:float::8:float
;RUN: dsaopt %s -dsa-local -analyze -check-type=main:s1,0:float::4:float::8:float
;RUN: dsaopt %s -dsa-local -enable-type-inference-opts -analyze -check-type=main:s1,VOID
;RUN: adsaopt %s -ld-args -deadargelim -dce -o %t.bc
;RUN: dsaopt %t.bc -dsa-local -enable-type-inference-opts -analyze -check-type=main:s,0:float::4:float::8:float
;RUN: dsaopt %t.bc -dsa-local -enable-type-inference-opts -analyze -check-type=main:s1,VOID
;RUN: adsaopt %s -ld-args -gep-expr-arg -deadargelim -dce -o %t.bc
;RUN: dsaopt %t.bc -dsa-local -enable-type-inference-opts -analyze -check-type=main:s,0:float::4:float::8:float
;RUN: dsaopt %t.bc -dsa-local -enable-type-inference-opts -analyze -check-type=main:s1,VOID

; Fails because -enable-type-inferences-opts doesn't do what's expected.
; Since that code's unmaintained for now, don't report these as DSA failures.
; XFAIL: *

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

define %0 @_Z3barv() nounwind {
entry:
  %retval = alloca %struct.S                      ; <%struct.S*> [#uses=4]
  %0 = alloca %struct.S                           ; <%struct.S*> [#uses=6]
  %obj = alloca %struct.S                         ; <%struct.S*> [#uses=4]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = getelementptr inbounds %struct.S* %obj, i32 0, i32 0 ; <float*> [#uses=1]
  store float 0x3FF3333340000000, float* %1, align 4
  %2 = getelementptr inbounds %struct.S* %0, i32 0, i32 0 ; <float*> [#uses=1]
  %3 = getelementptr inbounds %struct.S* %obj, i32 0, i32 0 ; <float*> [#uses=1]
  %4 = load float* %3, align 4                    ; <float> [#uses=1]
  store float %4, float* %2, align 4
  %5 = getelementptr inbounds %struct.S* %0, i32 0, i32 1 ; <float*> [#uses=1]
  %6 = getelementptr inbounds %struct.S* %obj, i32 0, i32 1 ; <float*> [#uses=1]
  %7 = load float* %6, align 4                    ; <float> [#uses=1]
  store float %7, float* %5, align 4
  %8 = getelementptr inbounds %struct.S* %0, i32 0, i32 2 ; <float*> [#uses=1]
  %9 = getelementptr inbounds %struct.S* %obj, i32 0, i32 2 ; <float*> [#uses=1]
  %10 = load float* %9, align 4                   ; <float> [#uses=1]
  store float %10, float* %8, align 4
  %11 = getelementptr inbounds %struct.S* %retval, i32 0, i32 0 ; <float*> [#uses=1]
  %12 = getelementptr inbounds %struct.S* %0, i32 0, i32 0 ; <float*> [#uses=1]
  %13 = load float* %12, align 4                  ; <float> [#uses=1]
  store float %13, float* %11, align 4
  %14 = getelementptr inbounds %struct.S* %retval, i32 0, i32 1 ; <float*> [#uses=1]
  %15 = getelementptr inbounds %struct.S* %0, i32 0, i32 1 ; <float*> [#uses=1]
  %16 = load float* %15, align 4                  ; <float> [#uses=1]
  store float %16, float* %14, align 4
  %17 = getelementptr inbounds %struct.S* %retval, i32 0, i32 2 ; <float*> [#uses=1]
  %18 = getelementptr inbounds %struct.S* %0, i32 0, i32 2 ; <float*> [#uses=1]
  %19 = load float* %18, align 4                  ; <float> [#uses=1]
  store float %19, float* %17, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = bitcast %struct.S* %retval to %0*    ; <%0*> [#uses=2]
  %mrv_gep = getelementptr %0* %retval1, i32 0, i32 0 ; <double*> [#uses=1]
  %mrv = load double* %mrv_gep                    ; <double> [#uses=1]
  %mrv_gep2 = getelementptr %0* %retval1, i32 0, i32 1 ; <float*> [#uses=1]
  %mrv3 = load float* %mrv_gep2                   ; <float> [#uses=1]
  %mrv4 = insertvalue %0 undef, double %mrv, 0    ; <%0> [#uses=1]
  %mrv5 = insertvalue %0 %mrv4, float %mrv3, 1    ; <%0> [#uses=1]
  ret %0 %mrv5
}

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %s = alloca %struct.S                           ; <%struct.S*> [#uses=5]
  %s1 = alloca %struct.S                          ; <%struct.S*> [#uses=3]
  %memtmp = alloca %0                             ; <%0*> [#uses=3]
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
  %6 = call %0 @_Z3barv() nounwind                ; <%0> [#uses=2]
  %mrv_gep = getelementptr inbounds %0* %memtmp, i32 0, i32 0 ; <double*> [#uses=1]
  %mrv_gr = extractvalue %0 %6, 0                 ; <double> [#uses=1]
  store double %mrv_gr, double* %mrv_gep
  %mrv_gep3 = getelementptr inbounds %0* %memtmp, i32 0, i32 1 ; <float*> [#uses=1]
  %mrv_gr4 = extractvalue %0 %6, 1                ; <float> [#uses=1]
  store float %mrv_gr4, float* %mrv_gep3
  %memtmp5 = bitcast %0* %memtmp to %struct.S*    ; <%struct.S*> [#uses=3]
  %7 = getelementptr inbounds %struct.S* %s1, i32 0, i32 0 ; <float*> [#uses=1]
  %8 = getelementptr inbounds %struct.S* %memtmp5, i32 0, i32 0 ; <float*> [#uses=1]
  %9 = load float* %8, align 4                    ; <float> [#uses=1]
  store float %9, float* %7, align 4
  %10 = getelementptr inbounds %struct.S* %s1, i32 0, i32 1 ; <float*> [#uses=1]
  %11 = getelementptr inbounds %struct.S* %memtmp5, i32 0, i32 1 ; <float*> [#uses=1]
  %12 = load float* %11, align 4                  ; <float> [#uses=1]
  store float %12, float* %10, align 4
  %13 = getelementptr inbounds %struct.S* %s1, i32 0, i32 2 ; <float*> [#uses=1]
  %14 = getelementptr inbounds %struct.S* %memtmp5, i32 0, i32 2 ; <float*> [#uses=1]
  %15 = load float* %14, align 4                  ; <float> [#uses=1]
  store float %15, float* %13, align 4
  store i32 0, i32* %0, align 4
  %16 = load i32* %0, align 4                     ; <i32> [#uses=1]
  store i32 %16, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval6 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval6
}

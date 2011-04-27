;RUN: dsaopt %s -dsa-local -analyze -check-same-node=initialize:temp:0,initialize:arr:0
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "initialize:temp:0+IR-E"
;RUN: dsaopt %s -dsa-local -analyze -verify-flags "initialize:arr+IR-E"

;RUN: dsaopt %s -dsa-bu -analyze -check-same-node=initialize:temp:0,initialize:arr:0
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "initialize:temp:0+IR-E"
;RUN: dsaopt %s -dsa-bu -analyze -verify-flags "initialize:arr+IR-E"

;RUN: dsaopt %s -dsa-td -analyze -check-same-node=initialize:temp:0,initialize:arr:0
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "initialize:temp:0+HMRE-I"
;RUN: dsaopt %s -dsa-td -analyze -verify-flags "initialize:arr+SMRE-I"

;RUN: dsaopt %s -dsa-td -analyze -verify-flags "main:InfoArray:0+HMR-IE"

; ModuleID = 'params1.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.InfoStruct = type { i32, i32, float }

define void @initialize(%struct.InfoStruct** %arr, i32 %size) nounwind {
entry:
  %arr_addr = alloca %struct.InfoStruct**         ; <%struct.InfoStruct***> [#uses=3]
  %size_addr = alloca i32                         ; <i32*> [#uses=2]
  %temp = alloca %struct.InfoStruct*              ; <%struct.InfoStruct**> [#uses=5]
  %elem = alloca %struct.InfoStruct               ; <%struct.InfoStruct*> [#uses=6]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.InfoStruct** %arr, %struct.InfoStruct*** %arr_addr
  store i32 %size, i32* %size_addr
  %0 = load %struct.InfoStruct*** %arr_addr, align 8 ; <%struct.InfoStruct**> [#uses=1]
  %1 = load %struct.InfoStruct** %0, align 8      ; <%struct.InfoStruct*> [#uses=1]
  store %struct.InfoStruct* %1, %struct.InfoStruct** %temp, align 8
  br label %bb1

bb:                                               ; preds = %bb1
  %2 = load %struct.InfoStruct** %temp, align 8   ; <%struct.InfoStruct*> [#uses=3]
  %3 = getelementptr inbounds %struct.InfoStruct* %elem, i32 0, i32 0 ; <i32*> [#uses=1]
  %4 = getelementptr inbounds %struct.InfoStruct* %2, i32 0, i32 0 ; <i32*> [#uses=1]
  %5 = load i32* %4, align 4                      ; <i32> [#uses=1]
  store i32 %5, i32* %3, align 4
  %6 = getelementptr inbounds %struct.InfoStruct* %elem, i32 0, i32 1 ; <i32*> [#uses=1]
  %7 = getelementptr inbounds %struct.InfoStruct* %2, i32 0, i32 1 ; <i32*> [#uses=1]
  %8 = load i32* %7, align 4                      ; <i32> [#uses=1]
  store i32 %8, i32* %6, align 4
  %9 = getelementptr inbounds %struct.InfoStruct* %elem, i32 0, i32 2 ; <float*> [#uses=1]
  %10 = getelementptr inbounds %struct.InfoStruct* %2, i32 0, i32 2 ; <float*> [#uses=1]
  %11 = load float* %10, align 4                  ; <float> [#uses=1]
  store float %11, float* %9, align 4
  %12 = getelementptr inbounds %struct.InfoStruct* %elem, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 0, i32* %12, align 4
  %13 = getelementptr inbounds %struct.InfoStruct* %elem, i32 0, i32 1 ; <i32*> [#uses=1]
  store i32 0, i32* %13, align 4
  %14 = getelementptr inbounds %struct.InfoStruct* %elem, i32 0, i32 2 ; <float*> [#uses=1]
  store float 0.000000e+00, float* %14, align 4
  %15 = load %struct.InfoStruct** %temp, align 8  ; <%struct.InfoStruct*> [#uses=1]
  %16 = getelementptr inbounds %struct.InfoStruct* %15, i64 1 ; <%struct.InfoStruct*> [#uses=1]
  store %struct.InfoStruct* %16, %struct.InfoStruct** %temp, align 8
  br label %bb1

bb1:                                              ; preds = %bb, %entry
  %17 = load %struct.InfoStruct*** %arr_addr, align 8 ; <%struct.InfoStruct**> [#uses=1]
  %18 = load %struct.InfoStruct** %17, align 8    ; <%struct.InfoStruct*> [#uses=1]
  %19 = load i32* %size_addr, align 4             ; <i32> [#uses=1]
  %20 = sext i32 %19 to i64                       ; <i64> [#uses=1]
  %21 = getelementptr inbounds %struct.InfoStruct* %18, i64 %20 ; <%struct.InfoStruct*> [#uses=1]
  %22 = load %struct.InfoStruct** %temp, align 8  ; <%struct.InfoStruct*> [#uses=1]
  %23 = icmp ugt %struct.InfoStruct* %21, %22     ; <i1> [#uses=1]
  br i1 %23, label %bb, label %bb2

bb2:                                              ; preds = %bb1
  br label %return

return:                                           ; preds = %bb2
  ret void
}

define void @process(%struct.InfoStruct** %arr, i32 %loc, i32 %count, float %fact) nounwind {
entry:
  %arr_addr = alloca %struct.InfoStruct**         ; <%struct.InfoStruct***> [#uses=2]
  %loc_addr = alloca i32                          ; <i32*> [#uses=2]
  %count_addr = alloca i32                        ; <i32*> [#uses=2]
  %fact_addr = alloca float                       ; <float*> [#uses=2]
  %ptr = alloca %struct.InfoStruct*               ; <%struct.InfoStruct**> [#uses=2]
  %obj = alloca %struct.InfoStruct                ; <%struct.InfoStruct*> [#uses=6]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.InfoStruct** %arr, %struct.InfoStruct*** %arr_addr
  store i32 %loc, i32* %loc_addr
  store i32 %count, i32* %count_addr
  store float %fact, float* %fact_addr
  %0 = load %struct.InfoStruct*** %arr_addr, align 8 ; <%struct.InfoStruct**> [#uses=1]
  %1 = load %struct.InfoStruct** %0, align 8      ; <%struct.InfoStruct*> [#uses=1]
  store %struct.InfoStruct* %1, %struct.InfoStruct** %ptr, align 8
  %2 = load %struct.InfoStruct** %ptr, align 8    ; <%struct.InfoStruct*> [#uses=1]
  %3 = load i32* %loc_addr, align 4               ; <i32> [#uses=1]
  %4 = sext i32 %3 to i64                         ; <i64> [#uses=1]
  %5 = getelementptr inbounds %struct.InfoStruct* %2, i64 %4 ; <%struct.InfoStruct*> [#uses=3]
  %6 = getelementptr inbounds %struct.InfoStruct* %obj, i32 0, i32 0 ; <i32*> [#uses=1]
  %7 = getelementptr inbounds %struct.InfoStruct* %5, i32 0, i32 0 ; <i32*> [#uses=1]
  %8 = load i32* %7, align 1                      ; <i32> [#uses=1]
  store i32 %8, i32* %6, align 1
  %9 = getelementptr inbounds %struct.InfoStruct* %obj, i32 0, i32 1 ; <i32*> [#uses=1]
  %10 = getelementptr inbounds %struct.InfoStruct* %5, i32 0, i32 1 ; <i32*> [#uses=1]
  %11 = load i32* %10, align 1                    ; <i32> [#uses=1]
  store i32 %11, i32* %9, align 1
  %12 = getelementptr inbounds %struct.InfoStruct* %obj, i32 0, i32 2 ; <float*> [#uses=1]
  %13 = getelementptr inbounds %struct.InfoStruct* %5, i32 0, i32 2 ; <float*> [#uses=1]
  %14 = load float* %13, align 1                  ; <float> [#uses=1]
  store float %14, float* %12, align 1
  %15 = getelementptr inbounds %struct.InfoStruct* %obj, i32 0, i32 0 ; <i32*> [#uses=1]
  %16 = load i32* %count_addr, align 4            ; <i32> [#uses=1]
  store i32 %16, i32* %15, align 4
  %17 = getelementptr inbounds %struct.InfoStruct* %obj, i32 0, i32 2 ; <float*> [#uses=1]
  %18 = load float* %fact_addr, align 4           ; <float> [#uses=1]
  store float %18, float* %17, align 4
  %19 = getelementptr inbounds %struct.InfoStruct* %obj, i32 0, i32 1 ; <i32*> [#uses=1]
  store i32 1, i32* %19, align 4
  br label %return

return:                                           ; preds = %entry
  ret void
}

define i32 @main() nounwind {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=1]
  %InfoArray = alloca %struct.InfoStruct*         ; <%struct.InfoStruct**> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call noalias i8* @malloc(i64 120) nounwind ; <i8*> [#uses=1]
  %1 = bitcast i8* %0 to %struct.InfoStruct*      ; <%struct.InfoStruct*> [#uses=1]
  store %struct.InfoStruct* %1, %struct.InfoStruct** %InfoArray, align 8
  call void @initialize(%struct.InfoStruct** %InfoArray, i32 10) nounwind
  call void @process(%struct.InfoStruct** %InfoArray, i32 4, i32 3, float 5.500000e+00) nounwind
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

declare noalias i8* @malloc(i64) nounwind

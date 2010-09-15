; ModuleID = 'params2.bc'
;FIXME: Remove this test entirely or add a RUN line!
;XFAIL:*
;RUN: not
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.InfoStruct = type { i32, i32, float }

define void @initialize(%struct.InfoStruct** %arr, i32 %size) nounwind {
entry:
  %arr_addr = alloca %struct.InfoStruct**         ; <%struct.InfoStruct***> [#uses=3]
  %size_addr = alloca i32                         ; <i32*> [#uses=2]
  %temp = alloca %struct.InfoStruct*              ; <%struct.InfoStruct**> [#uses=7]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.InfoStruct** %arr, %struct.InfoStruct*** %arr_addr
  store i32 %size, i32* %size_addr
  %0 = load %struct.InfoStruct*** %arr_addr, align 8 ; <%struct.InfoStruct**> [#uses=1]
  %1 = load %struct.InfoStruct** %0, align 8      ; <%struct.InfoStruct*> [#uses=1]
  store %struct.InfoStruct* %1, %struct.InfoStruct** %temp, align 8
  br label %bb1

bb:                                               ; preds = %bb1
  %2 = load %struct.InfoStruct** %temp, align 8   ; <%struct.InfoStruct*> [#uses=1]
  %3 = getelementptr inbounds %struct.InfoStruct* %2, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 0, i32* %3, align 4
  %4 = load %struct.InfoStruct** %temp, align 8   ; <%struct.InfoStruct*> [#uses=1]
  %5 = getelementptr inbounds %struct.InfoStruct* %4, i32 0, i32 1 ; <i32*> [#uses=1]
  store i32 0, i32* %5, align 4
  %6 = load %struct.InfoStruct** %temp, align 8   ; <%struct.InfoStruct*> [#uses=1]
  %7 = getelementptr inbounds %struct.InfoStruct* %6, i32 0, i32 2 ; <float*> [#uses=1]
  store float 0.000000e+00, float* %7, align 4
  %8 = load %struct.InfoStruct** %temp, align 8   ; <%struct.InfoStruct*> [#uses=1]
  %9 = getelementptr inbounds %struct.InfoStruct* %8, i64 1 ; <%struct.InfoStruct*> [#uses=1]
  store %struct.InfoStruct* %9, %struct.InfoStruct** %temp, align 8
  br label %bb1

bb1:                                              ; preds = %bb, %entry
  %10 = load %struct.InfoStruct*** %arr_addr, align 8 ; <%struct.InfoStruct**> [#uses=1]
  %11 = load %struct.InfoStruct** %10, align 8    ; <%struct.InfoStruct*> [#uses=1]
  %12 = load i32* %size_addr, align 4             ; <i32> [#uses=1]
  %13 = sext i32 %12 to i64                       ; <i64> [#uses=1]
  %14 = getelementptr inbounds %struct.InfoStruct* %11, i64 %13 ; <%struct.InfoStruct*> [#uses=1]
  %15 = load %struct.InfoStruct** %temp, align 8  ; <%struct.InfoStruct*> [#uses=1]
  %16 = icmp ugt %struct.InfoStruct* %14, %15     ; <i1> [#uses=1]
  br i1 %16, label %bb, label %bb2

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
  %2 = getelementptr inbounds %struct.InfoStruct* %obj, i32 0, i32 0 ; <i32*> [#uses=1]
  %3 = load i32* %count_addr, align 4             ; <i32> [#uses=1]
  store i32 %3, i32* %2, align 4
  %4 = getelementptr inbounds %struct.InfoStruct* %obj, i32 0, i32 2 ; <float*> [#uses=1]
  %5 = load float* %fact_addr, align 4            ; <float> [#uses=1]
  store float %5, float* %4, align 4
  %6 = getelementptr inbounds %struct.InfoStruct* %obj, i32 0, i32 1 ; <i32*> [#uses=1]
  store i32 1, i32* %6, align 4
  %7 = load %struct.InfoStruct** %ptr, align 8    ; <%struct.InfoStruct*> [#uses=1]
  %8 = load i32* %loc_addr, align 4               ; <i32> [#uses=1]
  %9 = sext i32 %8 to i64                         ; <i64> [#uses=1]
  %10 = getelementptr inbounds %struct.InfoStruct* %7, i64 %9 ; <%struct.InfoStruct*> [#uses=3]
  %11 = getelementptr inbounds %struct.InfoStruct* %10, i32 0, i32 0 ; <i32*> [#uses=1]
  %12 = getelementptr inbounds %struct.InfoStruct* %obj, i32 0, i32 0 ; <i32*> [#uses=1]
  %13 = load i32* %12, align 1                    ; <i32> [#uses=1]
  store i32 %13, i32* %11, align 1
  %14 = getelementptr inbounds %struct.InfoStruct* %10, i32 0, i32 1 ; <i32*> [#uses=1]
  %15 = getelementptr inbounds %struct.InfoStruct* %obj, i32 0, i32 1 ; <i32*> [#uses=1]
  %16 = load i32* %15, align 1                    ; <i32> [#uses=1]
  store i32 %16, i32* %14, align 1
  %17 = getelementptr inbounds %struct.InfoStruct* %10, i32 0, i32 2 ; <float*> [#uses=1]
  %18 = getelementptr inbounds %struct.InfoStruct* %obj, i32 0, i32 2 ; <float*> [#uses=1]
  %19 = load float* %18, align 1                  ; <float> [#uses=1]
  store float %19, float* %17, align 1
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

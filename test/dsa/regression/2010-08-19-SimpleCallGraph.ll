; ModuleID = 'test.c'
; RUN: dsaopt %s -calltarget-eqtd
; RUN: dsaopt %s -calltarget-eqtd -dsa-no-filter-callcc=false
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-apple-darwin10.4"

%struct.a = type { i32, i32, i32, i32, i32, i32, i32, i32 }

define i32 @func1(%struct.a* byval %bar) nounwind ssp {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = getelementptr inbounds %struct.a* %bar, i32 0, i32 6 ; <i32*> [#uses=1]
  %2 = load i32* %1, align 4                      ; <i32> [#uses=1]
  %3 = getelementptr inbounds %struct.a* %bar, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 %2, i32* %3, align 4
  %4 = getelementptr inbounds %struct.a* %bar, i32 0, i32 1 ; <i32*> [#uses=1]
  %5 = load i32* %4, align 4                      ; <i32> [#uses=1]
  %6 = mul nsw i32 %5, 2                          ; <i32> [#uses=1]
  store i32 %6, i32* %0, align 4
  %7 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %7, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

define i32 @func2(%struct.a* %bar) nounwind ssp {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = getelementptr inbounds %struct.a* %bar, i32 0, i32 6 ; <i32*> [#uses=1]
  %2 = load i32* %1, align 4                      ; <i32> [#uses=1]
  %3 = getelementptr inbounds %struct.a* %bar, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 %2, i32* %3, align 4
  %4 = getelementptr inbounds %struct.a* %bar, i32 0, i32 0 ; <i32*> [#uses=1]
  %5 = load i32* %4, align 4                      ; <i32> [#uses=1]
  %6 = getelementptr inbounds %struct.a* %bar, i32 0, i32 1 ; <i32*> [#uses=1]
  %7 = load i32* %6, align 4                      ; <i32> [#uses=1]
  %8 = add nsw i32 %5, %7                         ; <i32> [#uses=1]
  %9 = getelementptr inbounds %struct.a* %bar, i32 0, i32 2 ; <i32*> [#uses=1]
  %10 = load i32* %9, align 4                     ; <i32> [#uses=1]
  %11 = add nsw i32 %8, %10                       ; <i32> [#uses=1]
  %12 = getelementptr inbounds %struct.a* %bar, i32 0, i32 7 ; <i32*> [#uses=1]
  %13 = load i32* %12, align 4                    ; <i32> [#uses=1]
  %14 = add nsw i32 %11, %13                      ; <i32> [#uses=1]
  store i32 %14, i32* %0, align 4
  %15 = load i32* %0, align 4                     ; <i32> [#uses=1]
  store i32 %15, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

define i32 @main(i32 %argc, i8** %argv) nounwind ssp {
entry:
  %argc_addr = alloca i32                         ; <i32*> [#uses=3]
  %argv_addr = alloca i8**                        ; <i8***> [#uses=1]
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %data = alloca %struct.a                        ; <%struct.a*> [#uses=3]
  %f = alloca i32 (%struct.a*)*                   ; <i32 (%struct.a*)**> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %argc, i32* %argc_addr
  store i8** %argv, i8*** %argv_addr
  %1 = getelementptr inbounds %struct.a* %data, i32 0, i32 0 ; <i32*> [#uses=1]
  %2 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]
  store i32 %2, i32* %1, align 4
  %3 = load i32* %argc_addr, align 4              ; <i32> [#uses=1]
  %4 = icmp sle i32 %3, 1                         ; <i1> [#uses=1]
  br i1 %4, label %bb, label %bb1

bb:                                               ; preds = %entry
  store i32 (%struct.a*)* @func1, i32 (%struct.a*)** %f, align 8
  br label %bb2

bb1:                                              ; preds = %entry
  store i32 (%struct.a*)* @func2, i32 (%struct.a*)** %f, align 8
  br label %bb2

bb2:                                              ; preds = %bb1, %bb
  %5 = load i32 (%struct.a*)** %f, align 8        ; <i32 (%struct.a*)*> [#uses=1]
  %6 = call fastcc i32 %5(%struct.a* byval %data) nounwind ; <i32> [#uses=0]
  %7 = getelementptr inbounds %struct.a* %data, i32 0, i32 0 ; <i32*> [#uses=1]
  %8 = load i32* %7, align 4                      ; <i32> [#uses=1]
  store i32 %8, i32* %0, align 4
  %9 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %9, i32* %retval, align 4
  br label %return

return:                                           ; preds = %bb2
  %retval3 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval3
}

;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=lreadf,f_ungetc,lreadr
;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=lreadr,f_ungetc,f_getc

; ModuleID = 'test1.o'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct.FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct.FILE*, i32 }
%struct.gen_readio = type { i32 (i8*)*, void (i32, i8*)*, i8* }

define i32* @lreadr(%struct.gen_readio* %f) nounwind {
entry:
  %f_addr = alloca %struct.gen_readio*            ; <%struct.gen_readio**> [#uses=5]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %t = alloca i32                                 ; <i32*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.gen_readio* %f, %struct.gen_readio** %f_addr
  %1 = load %struct.gen_readio** %f_addr, align 8 ; <%struct.gen_readio*> [#uses=1]
  %2 = getelementptr inbounds %struct.gen_readio* %1, i32 0, i32 0 ; <i32 (i8*)**> [#uses=1]
  %3 = load i32 (i8*)** %2, align 8               ; <i32 (i8*)*> [#uses=1]
  %4 = load %struct.gen_readio** %f_addr, align 8 ; <%struct.gen_readio*> [#uses=1]
  %5 = getelementptr inbounds %struct.gen_readio* %4, i32 0, i32 2 ; <i8**> [#uses=1]
  %6 = load i8** %5, align 8                      ; <i8*> [#uses=1]
  %7 = call i32 %3(i8* %6) nounwind               ; <i32> [#uses=1]
  store i32 %7, i32* %t, align 4
  %8 = load %struct.gen_readio** %f_addr, align 8 ; <%struct.gen_readio*> [#uses=1]
  %9 = getelementptr inbounds %struct.gen_readio* %8, i32 0, i32 1 ; <void (i32, i8*)**> [#uses=1]
  %10 = load void (i32, i8*)** %9, align 8        ; <void (i32, i8*)*> [#uses=1]
  %11 = load %struct.gen_readio** %f_addr, align 8 ; <%struct.gen_readio*> [#uses=1]
  %12 = getelementptr inbounds %struct.gen_readio* %11, i32 0, i32 2 ; <i8**> [#uses=1]
  %13 = load i8** %12, align 8                    ; <i8*> [#uses=1]
  %14 = load i32* %t, align 4                     ; <i32> [#uses=1]
  call void %10(i32 %14, i8* %13) nounwind
  %15 = call noalias i8* @malloc(i64 4) nounwind  ; <i8*> [#uses=1]
  %16 = bitcast i8* %15 to i32*                   ; <i32*> [#uses=1]
  store i32* %16, i32** %0, align 8
  %17 = load i32** %0, align 8                    ; <i32*> [#uses=1]
  store i32* %17, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

declare noalias i8* @malloc(i64) nounwind

define void @init(%struct.gen_readio* %f, i32 (i8*)* %fcn) nounwind {
entry:
  %f_addr = alloca %struct.gen_readio*            ; <%struct.gen_readio**> [#uses=2]
  %fcn_addr = alloca i32 (i8*)*                   ; <i32 (i8*)**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.gen_readio* %f, %struct.gen_readio** %f_addr
  store i32 (i8*)* %fcn, i32 (i8*)** %fcn_addr
  %0 = load %struct.gen_readio** %f_addr, align 8 ; <%struct.gen_readio*> [#uses=1]
  %1 = getelementptr inbounds %struct.gen_readio* %0, i32 0, i32 0 ; <i32 (i8*)**> [#uses=1]
  %2 = load i32 (i8*)** %fcn_addr, align 8        ; <i32 (i8*)*> [#uses=1]
  store i32 (i8*)* %2, i32 (i8*)** %1, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

define i32* @lreadf(%struct.FILE* %f) nounwind {
entry:
  %f_addr = alloca %struct.FILE*                  ; <%struct.FILE**> [#uses=2]
  %retval = alloca i32*                           ; <i32**> [#uses=2]
  %0 = alloca i32*                                ; <i32**> [#uses=2]
  %s = alloca %struct.gen_readio                  ; <%struct.gen_readio*> [#uses=6]
  %c = alloca i32                                 ; <i32*> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.FILE* %f, %struct.FILE** %f_addr
  call void @init(%struct.gen_readio* %s, i32 (i8*)* bitcast (i32 (%struct.FILE*)* @f_getc to i32 (i8*)*)) nounwind
  %1 = getelementptr inbounds %struct.gen_readio* %s, i32 0, i32 1 ; <void (i32, i8*)**> [#uses=1]
  store void (i32, i8*)* bitcast (void (i32, %struct.FILE*)* @f_ungetc to void (i32, i8*)*), void (i32, i8*)** %1, align 8
  %2 = getelementptr inbounds %struct.gen_readio* %s, i32 0, i32 2 ; <i8**> [#uses=1]
  %3 = load %struct.FILE** %f_addr, align 8       ; <%struct.FILE*> [#uses=1]
  %4 = bitcast %struct.FILE* %3 to i8*            ; <i8*> [#uses=1]
  store i8* %4, i8** %2, align 8
  %5 = getelementptr inbounds %struct.gen_readio* %s, i32 0, i32 1 ; <void (i32, i8*)**> [#uses=1]
  %6 = load void (i32, i8*)** %5, align 8         ; <void (i32, i8*)*> [#uses=1]
  %7 = getelementptr inbounds %struct.gen_readio* %s, i32 0, i32 2 ; <i8**> [#uses=1]
  %8 = load i8** %7, align 8                      ; <i8*> [#uses=1]
  %9 = load i32* %c, align 4                      ; <i32> [#uses=1]
  call void %6(i32 %9, i8* %8) nounwind
  %10 = call i32* @lreadr(%struct.gen_readio* %s) nounwind ; <i32*> [#uses=1]
  store i32* %10, i32** %0, align 8
  %11 = load i32** %0, align 8                    ; <i32*> [#uses=1]
  store i32* %11, i32** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32** %retval                   ; <i32*> [#uses=1]
  ret i32* %retval1
}

declare i32 @f_getc(%struct.FILE*)

declare void @f_ungetc(i32, %struct.FILE*)

define void @main() nounwind {
entry:
  br label %return

return:                                           ; preds = %entry
  ret void
}

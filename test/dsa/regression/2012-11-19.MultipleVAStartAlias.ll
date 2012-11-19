; PR14147
; RUN: dsaopt %s -dsa-local -analyze -check-same-node=@sum:reg_save_area,@sum:reg_save_area16
; ModuleID = 'test-vaarg.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag = type { i32, i32, i8*, i8* }

@.str = private unnamed_addr constant [4 x i8] c"%d\0A\00", align 1

define i32 @sum(i32 %n, ...) nounwind uwtable {
entry:
  %vl = alloca [1 x %struct.__va_list_tag], align 16
  %arraydecay1 = bitcast [1 x %struct.__va_list_tag]* %vl to i8*
  call void @llvm.va_start(i8* %arraydecay1)
  %cmp32 = icmp sgt i32 %n, 0
  br i1 %cmp32, label %for.body.lr.ph, label %for.end.thread

for.end.thread:                                   ; preds = %entry
  call void @llvm.va_end(i8* %arraydecay1)
  call void @llvm.va_start(i8* %arraydecay1)
  br label %for.end26

for.body.lr.ph:                                   ; preds = %entry
  %gp_offset_p = getelementptr inbounds [1 x %struct.__va_list_tag]* %vl, i64 0, i64 0, i32 0
  %0 = getelementptr inbounds [1 x %struct.__va_list_tag]* %vl, i64 0, i64 0, i32 3
  %overflow_arg_area_p = getelementptr inbounds [1 x %struct.__va_list_tag]* %vl, i64 0, i64 0, i32 2
  %gp_offset.pre = load i32* %gp_offset_p, align 16
  br label %for.body

for.body:                                         ; preds = %vaarg.end, %for.body.lr.ph
  %gp_offset = phi i32 [ %gp_offset.pre, %for.body.lr.ph ], [ %gp_offset39, %vaarg.end ]
  %result.034 = phi i32 [ 0, %for.body.lr.ph ], [ %add, %vaarg.end ]
  %i.033 = phi i32 [ 0, %for.body.lr.ph ], [ %inc, %vaarg.end ]
  %fits_in_gp = icmp ult i32 %gp_offset, 41
  br i1 %fits_in_gp, label %vaarg.in_reg, label %vaarg.in_mem

vaarg.in_reg:                                     ; preds = %for.body
  %reg_save_area = load i8** %0, align 16
  %1 = sext i32 %gp_offset to i64
  %2 = getelementptr i8* %reg_save_area, i64 %1
  %3 = add i32 %gp_offset, 8
  store i32 %3, i32* %gp_offset_p, align 16
  br label %vaarg.end

vaarg.in_mem:                                     ; preds = %for.body
  %overflow_arg_area = load i8** %overflow_arg_area_p, align 8
  %overflow_arg_area.next = getelementptr i8* %overflow_arg_area, i64 8
  store i8* %overflow_arg_area.next, i8** %overflow_arg_area_p, align 8
  br label %vaarg.end

vaarg.end:                                        ; preds = %vaarg.in_mem, %vaarg.in_reg
  %gp_offset39 = phi i32 [ %3, %vaarg.in_reg ], [ %gp_offset, %vaarg.in_mem ]
  %vaarg.addr.in = phi i8* [ %2, %vaarg.in_reg ], [ %overflow_arg_area, %vaarg.in_mem ]
  %vaarg.addr = bitcast i8* %vaarg.addr.in to i32*
  %4 = load i32* %vaarg.addr, align 4
  %add = add nsw i32 %4, %result.034
  %inc = add nsw i32 %i.033, 1
  %exitcond36 = icmp eq i32 %inc, %n
  br i1 %exitcond36, label %for.end, label %for.body

for.end:                                          ; preds = %vaarg.end
  call void @llvm.va_end(i8* %arraydecay1)
  call void @llvm.va_start(i8* %arraydecay1)
  br i1 %cmp32, label %for.body9.lr.ph, label %for.end26

for.body9.lr.ph:                                  ; preds = %for.end
  %gp_offset_p12 = getelementptr inbounds [1 x %struct.__va_list_tag]* %vl, i64 0, i64 0, i32 0
  %5 = getelementptr inbounds [1 x %struct.__va_list_tag]* %vl, i64 0, i64 0, i32 3
  %overflow_arg_area_p18 = getelementptr inbounds [1 x %struct.__va_list_tag]* %vl, i64 0, i64 0, i32 2
  %gp_offset13.pre = load i32* %gp_offset_p12, align 16
  br label %for.body9

for.body9:                                        ; preds = %vaarg.end21, %for.body9.lr.ph
  %gp_offset13 = phi i32 [ %gp_offset13.pre, %for.body9.lr.ph ], [ %gp_offset1337, %vaarg.end21 ]
  %result.131 = phi i32 [ %add, %for.body9.lr.ph ], [ %add23, %vaarg.end21 ]
  %i.130 = phi i32 [ 0, %for.body9.lr.ph ], [ %inc25, %vaarg.end21 ]
  %fits_in_gp14 = icmp ult i32 %gp_offset13, 41
  br i1 %fits_in_gp14, label %vaarg.in_reg15, label %vaarg.in_mem17

vaarg.in_reg15:                                   ; preds = %for.body9
  %reg_save_area16 = load i8** %5, align 16
  %6 = sext i32 %gp_offset13 to i64
  %7 = getelementptr i8* %reg_save_area16, i64 %6
  %8 = add i32 %gp_offset13, 8
  store i32 %8, i32* %gp_offset_p12, align 16
  br label %vaarg.end21

vaarg.in_mem17:                                   ; preds = %for.body9
  %overflow_arg_area19 = load i8** %overflow_arg_area_p18, align 8
  %overflow_arg_area.next20 = getelementptr i8* %overflow_arg_area19, i64 8
  store i8* %overflow_arg_area.next20, i8** %overflow_arg_area_p18, align 8
  br label %vaarg.end21

vaarg.end21:                                      ; preds = %vaarg.in_mem17, %vaarg.in_reg15
  %gp_offset1337 = phi i32 [ %8, %vaarg.in_reg15 ], [ %gp_offset13, %vaarg.in_mem17 ]
  %vaarg.addr22.in = phi i8* [ %7, %vaarg.in_reg15 ], [ %overflow_arg_area19, %vaarg.in_mem17 ]
  %vaarg.addr22 = bitcast i8* %vaarg.addr22.in to i32*
  %9 = load i32* %vaarg.addr22, align 4
  %add23 = add nsw i32 %9, %result.131
  %inc25 = add nsw i32 %i.130, 1
  %exitcond = icmp eq i32 %inc25, %n
  br i1 %exitcond, label %for.end26, label %for.body9

for.end26:                                        ; preds = %vaarg.end21, %for.end, %for.end.thread
  %result.1.lcssa = phi i32 [ %add, %for.end ], [ 0, %for.end.thread ], [ %add23, %vaarg.end21 ]
  call void @llvm.va_end(i8* %arraydecay1)
  ret i32 %result.1.lcssa
}

declare void @llvm.va_start(i8*) nounwind

declare void @llvm.va_end(i8*) nounwind

define i32 @main() nounwind uwtable {
entry:
  %call = tail call i32 (i32, ...)* @sum(i32 5, i32 1, i32 2, i32 3, i32 4, i32 5)
  %call1 = tail call i32 (i8*, ...)* @printf(i8* getelementptr inbounds ([4 x i8]* @.str, i64 0, i64 0), i32 %call) nounwind
  ret i32 0
}

declare i32 @printf(i8* nocapture, ...) nounwind

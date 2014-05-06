; RUN: dsaopt %s -dsa-local -disable-output
; (Reduced testcase from submitted file demonstrating assertion failure)
; PR19175
; "Assert fails because a node collapse while handling a Vararg within structure on LocalDataStructure Analysis"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.__va_list_tag.0.2.16 = type { i32, i32, i8*, i8* }
%struct.t.1.3.17 = type { [1 x %struct.__va_list_tag.0.2.16], i8* }

; Function Attrs: nounwind
declare void @llvm.va_start(i8*) #0

; Function Attrs: nounwind uwtable
define void @test_va_bugging_func(i32 %p1, ...) #1 {
entry:
  %v = getelementptr inbounds %struct.t.1.3.17* undef, i32 0, i32 0
  %arraydecay = getelementptr inbounds [1 x %struct.__va_list_tag.0.2.16]* %v, i32 0, i32 0
  %arraydecay1 = bitcast %struct.__va_list_tag.0.2.16* %arraydecay to i8*
  call void @llvm.va_start(i8* %arraydecay1)
  unreachable
}

attributes #0 = { nounwind }
attributes #1 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.ident = !{!0}

!0 = metadata !{metadata !"clang version 3.5.0 (git://github.com/llvm-mirror/clang.git 2afa00ce8693eaf81335e4ab629ac1247a461cac) (git://github.com/llvm-mirror/llvm.git 3b4c8c2b2ab2a4af00d03b1b39b1b1eaf564ab76)"}

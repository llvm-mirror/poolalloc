;RUN: dsaopt %s -dsa-td -analyze -check-callees=decode,C2
;RUN: dsaopt %s -dsa-td -analyze -check-not-callees=decode,A1,B2,main,decode

;XFAIL: *
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.msg = type { void (i32, i32)*, i32 }

@q = common global void (i32)* null, align 8

; Function Attrs: nounwind uwtable
define i32 @main() #0 {
entry:
  %call = call noalias i8* @malloc(i64 16) #2
  %0 = bitcast i8* %call to %struct.msg*
  store void (i32)* @A1, void (i32)** @q, align 8
  %p = getelementptr inbounds %struct.msg, %struct.msg* %0, i32 0, i32 0
  store void (i32, i32)* @B2, void (i32, i32)** %p, align 8
  call void @decode(void (i32, i32)* @C2)
  ret i32 0
}

; Function Attrs: nounwind
declare noalias i8* @malloc(i64) #1

; Function Attrs: nounwind uwtable
define internal void @A1(i32 %a) #0 {
entry:
  ret void
}

; Function Attrs: nounwind uwtable
define internal void @B2(i32 %a, i32 %b) #0 {
entry:
  ret void
}

; Function Attrs: nounwind uwtable
define internal void @decode(void (i32, i32)* %decoder_f) #0 {
entry:
  call void %decoder_f(i32 1, i32 2)
  ret void
}

; Function Attrs: nounwind uwtable
define internal void @C2(i32 %a, i32 %b) #0 {
entry:
  ret void
}

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+sse,+sse2" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+sse,+sse2" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.ident = !{!0}

!0 = !{!"clang version 3.7.0 (git@github.com:llvm-mirror/clang.git c7f06de7cf01e7552da38e08a0a410c71fbc6837) (git@github.com:llvm-mirror/llvm.git 1fd101c86df4dd321a53bdd80fe5ca106f3f76e2)"}

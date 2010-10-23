;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=_ZN13DerivedClass110myFunctionEv,_ZN9BaseClass14calledFunctionEv

; ModuleID = 'tt'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%0 = type { i32, void ()* }
%struct.BaseClass = type { i32 (...)** }
%struct.DerivedClass1 = type { %struct.BaseClass }
%struct.__class_type_info_pseudo = type { %struct.__type_info_pseudo }
%struct.__si_class_type_info_pseudo = type { %struct.__type_info_pseudo, %"struct.std::type_info"* }
%struct.__type_info_pseudo = type { i8*, i8* }
%"struct.std::type_info" = type opaque

@ob = internal global %struct.BaseClass zeroinitializer ; <%struct.BaseClass*> [#uses=2]
@_ZTV9BaseClass = internal constant [4 x i32 (...)*] [i32 (...)* null, i32 (...)* bitcast (%struct.__class_type_info_pseudo* @_ZTI9BaseClass to i32 (...)*), i32 (...)* bitcast (void (%struct.BaseClass*)* @_ZN9BaseClass10myFunctionEv to i32 (...)*), i32 (...)* bitcast (void (%struct.BaseClass*)* @_ZN9BaseClass14calledFunctionEv to i32 (...)*)], align 32 ; <[4 x i32 (...)*]*> [#uses=1]
@_ZTI9BaseClass = internal constant %struct.__class_type_info_pseudo { %struct.__type_info_pseudo { i8* inttoptr (i64 add (i64 ptrtoint ([0 x i32 (...)*]* @_ZTVN10__cxxabiv117__class_type_infoE to i64), i64 16) to i8*), i8* getelementptr inbounds ([11 x i8]* @_ZTS9BaseClass, i64 0, i64 0) } }, align 16 ; <%struct.__class_type_info_pseudo*> [#uses=2]
@_ZTVN10__cxxabiv117__class_type_infoE = external constant [0 x i32 (...)*] ; <[0 x i32 (...)*]*> [#uses=1]
@_ZTS9BaseClass = internal constant [11 x i8] c"9BaseClass\00" ; <[11 x i8]*> [#uses=1]
@_ZTV13DerivedClass1 = internal constant [4 x i32 (...)*] [i32 (...)* null, i32 (...)* bitcast (%struct.__si_class_type_info_pseudo* @_ZTI13DerivedClass1 to i32 (...)*), i32 (...)* bitcast (void (%struct.DerivedClass1*)* @_ZN13DerivedClass110myFunctionEv to i32 (...)*), i32 (...)* bitcast (void (%struct.DerivedClass1*)* @_ZN13DerivedClass114calledFunctionEv to i32 (...)*)], align 32 ; <[4 x i32 (...)*]*> [#uses=1]
@_ZTI13DerivedClass1 = internal constant %struct.__si_class_type_info_pseudo { %struct.__type_info_pseudo { i8* inttoptr (i64 add (i64 ptrtoint ([0 x i32 (...)*]* @_ZTVN10__cxxabiv120__si_class_type_infoE to i64), i64 16) to i8*), i8* getelementptr inbounds ([16 x i8]* @_ZTS13DerivedClass1, i64 0, i64 0) }, %"struct.std::type_info"* bitcast (%struct.__class_type_info_pseudo* @_ZTI9BaseClass to %"struct.std::type_info"*) }, align 16 ; <%struct.__si_class_type_info_pseudo*> [#uses=1]
@_ZTVN10__cxxabiv120__si_class_type_infoE = external constant [0 x i32 (...)*] ; <[0 x i32 (...)*]*> [#uses=1]
@_ZTS13DerivedClass1 = internal constant [16 x i8] c"13DerivedClass1\00", align 16 ; <[16 x i8]*> [#uses=1]
@derivedObject1 = internal global %struct.DerivedClass1 zeroinitializer ; <%struct.DerivedClass1*> [#uses=1]
@llvm.global_ctors = appending global [1 x %0] [%0 { i32 65535, void ()* @_GLOBAL__I_ob }] ; <[1 x %0]*> [#uses=0]

define internal void @_GLOBAL__I_ob() nounwind {
entry:
  call void @_Z41__static_initialization_and_destruction_0ii(i32 1, i32 65535) nounwind
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @_ZN9BaseClass10myFunctionEv(%struct.BaseClass* %this) nounwind align 2 {
entry:
  %this_addr = alloca %struct.BaseClass*          ; <%struct.BaseClass**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.BaseClass* %this, %struct.BaseClass** %this_addr
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @_ZN9BaseClass14calledFunctionEv(%struct.BaseClass* %this) nounwind align 2 {
entry:
  %this_addr = alloca %struct.BaseClass*          ; <%struct.BaseClass**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.BaseClass* %this, %struct.BaseClass** %this_addr
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @_ZN13DerivedClass110myFunctionEv(%struct.DerivedClass1* %this) align 2 {
entry:
  %this_addr = alloca %struct.DerivedClass1*      ; <%struct.DerivedClass1**> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.DerivedClass1* %this, %struct.DerivedClass1** %this_addr
  %0 = load %struct.DerivedClass1** %this_addr, align 8 ; <%struct.DerivedClass1*> [#uses=1]
  %1 = getelementptr inbounds %struct.DerivedClass1* %0, i32 0, i32 0 ; <%struct.BaseClass*> [#uses=1]
  %2 = getelementptr inbounds %struct.BaseClass* %1, i32 0, i32 0 ; <i32 (...)***> [#uses=1]
  %3 = load i32 (...)*** %2, align 8              ; <i32 (...)**> [#uses=1]
  %4 = getelementptr inbounds i32 (...)** %3, i64 1 ; <i32 (...)**> [#uses=1]
  %5 = load i32 (...)** %4, align 1               ; <i32 (...)*> [#uses=1]
  %6 = bitcast i32 (...)* %5 to void (%struct.DerivedClass1*)* ; <void (%struct.DerivedClass1*)*> [#uses=1]
  %7 = load %struct.DerivedClass1** %this_addr, align 8 ; <%struct.DerivedClass1*> [#uses=1]
  call void %6(%struct.DerivedClass1* %7)
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @_ZN13DerivedClass114calledFunctionEv(%struct.DerivedClass1* %this) nounwind align 2 {
entry:
  %this_addr = alloca %struct.DerivedClass1*      ; <%struct.DerivedClass1**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.DerivedClass1* %this, %struct.DerivedClass1** %this_addr
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal %struct.BaseClass* @_ZL4initi(i32 %i) nounwind {
entry:
  %i_addr = alloca i32                            ; <i32*> [#uses=1]
  %retval = alloca %struct.BaseClass*             ; <%struct.BaseClass**> [#uses=2]
  %0 = alloca %struct.BaseClass*                  ; <%struct.BaseClass**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %i, i32* %i_addr
  store %struct.BaseClass* @ob, %struct.BaseClass** %0, align 8
  %1 = load %struct.BaseClass** %0, align 8       ; <%struct.BaseClass*> [#uses=1]
  store %struct.BaseClass* %1, %struct.BaseClass** %retval, align 8
  br label %return

return:                                           ; preds = %entry
  %retval1 = load %struct.BaseClass** %retval     ; <%struct.BaseClass*> [#uses=1]
  ret %struct.BaseClass* %retval1
}

define i32 @main() {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %p = alloca %struct.BaseClass*                  ; <%struct.BaseClass**> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = call %struct.BaseClass* @_ZL4initi(i32 2) nounwind ; <%struct.BaseClass*> [#uses=1]
  store %struct.BaseClass* %1, %struct.BaseClass** %p, align 8
  %2 = load %struct.BaseClass** %p, align 8       ; <%struct.BaseClass*> [#uses=1]
  %3 = getelementptr inbounds %struct.BaseClass* %2, i32 0, i32 0 ; <i32 (...)***> [#uses=1]
  %4 = load i32 (...)*** %3, align 8              ; <i32 (...)**> [#uses=1]
  %5 = getelementptr inbounds i32 (...)** %4, i64 0 ; <i32 (...)**> [#uses=1]
  %6 = load i32 (...)** %5, align 1               ; <i32 (...)*> [#uses=1]
  %7 = bitcast i32 (...)* %6 to void (%struct.BaseClass*)* ; <void (%struct.BaseClass*)*> [#uses=1]
  %8 = load %struct.BaseClass** %p, align 8       ; <%struct.BaseClass*> [#uses=1]
  call void %7(%struct.BaseClass* %8)
  store i32 0, i32* %0, align 4
  %9 = load i32* %0, align 4                      ; <i32> [#uses=1]
  store i32 %9, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

define internal void @_ZN9BaseClassC2Ev(%struct.BaseClass* %this) nounwind align 2 {
entry:
  %this_addr = alloca %struct.BaseClass*          ; <%struct.BaseClass**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.BaseClass* %this, %struct.BaseClass** %this_addr
  %0 = load %struct.BaseClass** %this_addr, align 8 ; <%struct.BaseClass*> [#uses=1]
  %1 = getelementptr inbounds %struct.BaseClass* %0, i32 0, i32 0 ; <i32 (...)***> [#uses=1]
  store i32 (...)** getelementptr inbounds ([4 x i32 (...)*]* @_ZTV9BaseClass, i64 0, i64 2), i32 (...)*** %1, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @_ZN9BaseClassC1Ev(%struct.BaseClass* %this) nounwind align 2 {
entry:
  %this_addr = alloca %struct.BaseClass*          ; <%struct.BaseClass**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.BaseClass* %this, %struct.BaseClass** %this_addr
  %0 = load %struct.BaseClass** %this_addr, align 8 ; <%struct.BaseClass*> [#uses=1]
  %1 = getelementptr inbounds %struct.BaseClass* %0, i32 0, i32 0 ; <i32 (...)***> [#uses=1]
  store i32 (...)** getelementptr inbounds ([4 x i32 (...)*]* @_ZTV9BaseClass, i64 0, i64 2), i32 (...)*** %1, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @_ZN13DerivedClass1C1Ev(%struct.DerivedClass1* %this) nounwind align 2 {
entry:
  %this_addr = alloca %struct.DerivedClass1*      ; <%struct.DerivedClass1**> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.DerivedClass1* %this, %struct.DerivedClass1** %this_addr
  %0 = load %struct.DerivedClass1** %this_addr, align 8 ; <%struct.DerivedClass1*> [#uses=1]
  %1 = getelementptr inbounds %struct.DerivedClass1* %0, i32 0, i32 0 ; <%struct.BaseClass*> [#uses=1]
  call void @_ZN9BaseClassC2Ev(%struct.BaseClass* %1) nounwind
  %2 = load %struct.DerivedClass1** %this_addr, align 8 ; <%struct.DerivedClass1*> [#uses=1]
  %3 = getelementptr inbounds %struct.DerivedClass1* %2, i32 0, i32 0 ; <%struct.BaseClass*> [#uses=1]
  %4 = getelementptr inbounds %struct.BaseClass* %3, i32 0, i32 0 ; <i32 (...)***> [#uses=1]
  store i32 (...)** getelementptr inbounds ([4 x i32 (...)*]* @_ZTV13DerivedClass1, i64 0, i64 2), i32 (...)*** %4, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @_Z41__static_initialization_and_destruction_0ii(i32 %__initialize_p, i32 %__priority) nounwind {
entry:
  %__initialize_p_addr = alloca i32               ; <i32*> [#uses=2]
  %__priority_addr = alloca i32                   ; <i32*> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %__initialize_p, i32* %__initialize_p_addr
  store i32 %__priority, i32* %__priority_addr
  %0 = load i32* %__initialize_p_addr, align 4    ; <i32> [#uses=1]
  %1 = icmp eq i32 %0, 1                          ; <i1> [#uses=1]
  br i1 %1, label %bb, label %bb2

bb:                                               ; preds = %entry
  %2 = load i32* %__priority_addr, align 4        ; <i32> [#uses=1]
  %3 = icmp eq i32 %2, 65535                      ; <i1> [#uses=1]
  br i1 %3, label %bb1, label %bb2

bb1:                                              ; preds = %bb
  call void @_ZN9BaseClassC1Ev(%struct.BaseClass* @ob) nounwind
  call void @_ZN13DerivedClass1C1Ev(%struct.DerivedClass1* @derivedObject1) nounwind
  br label %bb2

bb2:                                              ; preds = %bb1, %bb, %entry
  br label %return

return:                                           ; preds = %bb2
  ret void
}

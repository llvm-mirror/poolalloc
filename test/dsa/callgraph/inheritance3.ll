;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=main,_ZN9BaseClass10myFunctionEv,_ZN13DerivedClass110myFunctionEv,_Z4initi
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=main,_ZN9BaseClass10myFunctionEv,_ZN13DerivedClass110myFunctionEv,_Z4initi
;RUN: dsaopt %s -dsa-cbu -analyze -check-callees=_Z4funcv,_ZN9BaseClass10myFunctionEv,_ZN13DerivedClass110myFunctionEv,_Z4initi
;RUN: dsaopt %s -dsa-bu -analyze -check-callees=_Z4funcv,_ZN9BaseClass10myFunctionEv,_ZN13DerivedClass110myFunctionEv,_Z4initi

; ModuleID = 'inheritance3.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%0 = type { i32, void ()* }
%struct..0__pthread_mutex_s = type { i32, i32, i32, i32, i32, i32, %struct.__pthread_list_t }
%struct.BaseClass = type { i32 (...)** }
%struct.DerivedClass1 = type { %struct.BaseClass }
%struct.__class_type_info_pseudo = type { %struct.__type_info_pseudo }
%struct.__locale_struct = type { [13 x %struct.locale_data*], i16*, i32*, i32*, [13 x i8*] }
%struct.__pthread_list_t = type { %struct.__pthread_list_t*, %struct.__pthread_list_t* }
%struct.__si_class_type_info_pseudo = type { %struct.__type_info_pseudo, %"struct.std::type_info"* }
%struct.__type_info_pseudo = type { i8*, i8* }
%struct.locale_data = type opaque
%"struct.std::basic_ios<char,std::char_traits<char> >" = type { %"struct.std::ios_base", %"struct.std::basic_ostream<char,std::char_traits<char> >"*, i8, i8, %"struct.std::basic_streambuf<char,std::char_traits<char> >"*, %"struct.std::ctype<char>"*, %"struct.std::num_get<char,std::istreambuf_iterator<char, std::char_traits<char> > >"*, %"struct.std::num_get<char,std::istreambuf_iterator<char, std::char_traits<char> > >"* }
%"struct.std::basic_ostream<char,std::char_traits<char> >" = type { i32 (...)**, %"struct.std::basic_ios<char,std::char_traits<char> >" }
%"struct.std::basic_streambuf<char,std::char_traits<char> >" = type { i32 (...)**, i8*, i8*, i8*, i8*, i8*, i8*, %"struct.std::locale" }
%"struct.std::ctype<char>" = type { %"struct.std::locale::facet", %struct.__locale_struct*, i8, i32*, i32*, i16*, i8, [256 x i8], [256 x i8], i8 }
%"struct.std::ios_base" = type { i32 (...)**, i64, i64, i32, i32, i32, %"struct.std::ios_base::_Callback_list"*, %"struct.std::ios_base::_Words", [8 x %"struct.std::ios_base::_Words"], i32, %"struct.std::ios_base::_Words"*, %"struct.std::locale" }
%"struct.std::ios_base::Init" = type <{ i8 }>
%"struct.std::ios_base::_Callback_list" = type { %"struct.std::ios_base::_Callback_list"*, void (i32, %"struct.std::ios_base"*, i32)*, i32, i32 }
%"struct.std::ios_base::_Words" = type { i8*, i64 }
%"struct.std::locale" = type { %"struct.std::locale::_Impl"* }
%"struct.std::locale::_Impl" = type { i32, %"struct.std::locale::facet"**, i64, %"struct.std::locale::facet"**, i8** }
%"struct.std::locale::facet" = type { i32 (...)**, i32 }
%"struct.std::locale::facet.base.64" = type { i32 (...)**, i32 }
%"struct.std::num_get<char,std::istreambuf_iterator<char, std::char_traits<char> > >" = type { %"struct.std::locale::facet" }
%"struct.std::num_put<char,std::ostreambuf_iterator<char, std::char_traits<char> > >" = type { %"struct.std::locale::facet" }
%"struct.std::type_info" = type { i32 (...)**, i8* }
%union.pthread_attr_t = type { i64, [12 x i32] }
%union.pthread_mutex_t = type { %struct..0__pthread_mutex_s }
%union.pthread_mutexattr_t = type { i32 }

@ob = internal global %struct.BaseClass zeroinitializer ; <%struct.BaseClass*> [#uses=2]
@derivedObject1 = internal global %struct.DerivedClass1 zeroinitializer ; <%struct.DerivedClass1*> [#uses=2]
@_ZSt4cout = external global %"struct.std::basic_ostream<char,std::char_traits<char> >" ; <%"struct.std::basic_ostream<char,std::char_traits<char> >"*> [#uses=2]
@.str = private constant [41 x i8] c"Using BaseClass version of myFunction()\0A\00", align 8 ; <[41 x i8]*> [#uses=1]
@.str1 = private constant [47 x i8] c"Using DerivedClass1's version of myFunction()\0A\00", align 8 ; <[47 x i8]*> [#uses=1]
@_ZTV9BaseClass = internal constant [3 x i32 (...)*] [i32 (...)* null, i32 (...)* bitcast (%struct.__class_type_info_pseudo* @_ZTI9BaseClass to i32 (...)*), i32 (...)* bitcast (void (%struct.BaseClass*)* @_ZN9BaseClass10myFunctionEv to i32 (...)*)], align 16 ; <[3 x i32 (...)*]*> [#uses=1]
@_ZTI9BaseClass = internal constant %struct.__class_type_info_pseudo { %struct.__type_info_pseudo { i8* inttoptr (i64 add (i64 ptrtoint ([0 x i32 (...)*]* @_ZTVN10__cxxabiv117__class_type_infoE to i64), i64 16) to i8*), i8* getelementptr inbounds ([11 x i8]* @_ZTS9BaseClass, i64 0, i64 0) } }, align 16 ; <%struct.__class_type_info_pseudo*> [#uses=2]
@_ZTVN10__cxxabiv117__class_type_infoE = external constant [0 x i32 (...)*] ; <[0 x i32 (...)*]*> [#uses=1]
@_ZTS9BaseClass = internal constant [11 x i8] c"9BaseClass\00" ; <[11 x i8]*> [#uses=1]
@_ZTV13DerivedClass1 = internal constant [3 x i32 (...)*] [i32 (...)* null, i32 (...)* bitcast (%struct.__si_class_type_info_pseudo* @_ZTI13DerivedClass1 to i32 (...)*), i32 (...)* bitcast (void (%struct.DerivedClass1*)* @_ZN13DerivedClass110myFunctionEv to i32 (...)*)], align 16 ; <[3 x i32 (...)*]*> [#uses=1]
@_ZTI13DerivedClass1 = internal constant %struct.__si_class_type_info_pseudo { %struct.__type_info_pseudo { i8* inttoptr (i64 add (i64 ptrtoint ([0 x i32 (...)*]* @_ZTVN10__cxxabiv120__si_class_type_infoE to i64), i64 16) to i8*), i8* getelementptr inbounds ([16 x i8]* @_ZTS13DerivedClass1, i64 0, i64 0) }, %"struct.std::type_info"* bitcast (%struct.__class_type_info_pseudo* @_ZTI9BaseClass to %"struct.std::type_info"*) }, align 16 ; <%struct.__si_class_type_info_pseudo*> [#uses=1]
@_ZTVN10__cxxabiv120__si_class_type_infoE = external constant [0 x i32 (...)*] ; <[0 x i32 (...)*]*> [#uses=1]
@_ZTS13DerivedClass1 = internal constant [16 x i8] c"13DerivedClass1\00", align 16 ; <[16 x i8]*> [#uses=1]
@_ZStL8__ioinit = internal global %"struct.std::ios_base::Init" zeroinitializer ; <%"struct.std::ios_base::Init"*> [#uses=2]
@__dso_handle = external global i8*               ; <i8**> [#uses=1]
@llvm.global_ctors = appending global [1 x %0] [%0 { i32 65535, void ()* @_GLOBAL__I_ob }] ; <[1 x %0]*> [#uses=0]

@_ZL20__gthrw_pthread_oncePiPFvvE = alias weak i32 (i32*, void ()*)* @pthread_once ; <i32 (i32*, void ()*)*> [#uses=0]
@_ZL27__gthrw_pthread_getspecificj = alias weak i8* (i32)* @pthread_getspecific ; <i8* (i32)*> [#uses=0]
@_ZL27__gthrw_pthread_setspecificjPKv = alias weak i32 (i32, i8*)* @pthread_setspecific ; <i32 (i32, i8*)*> [#uses=0]
@_ZL22__gthrw_pthread_createPmPK14pthread_attr_tPFPvS3_ES3_ = alias weak i32 (i64*, %union.pthread_attr_t*, i8* (i8*)*, i8*)* @pthread_create ; <i32 (i64*, %union.pthread_attr_t*, i8* (i8*)*, i8*)*> [#uses=0]
@_ZL22__gthrw_pthread_cancelm = alias weak i32 (i64)* @pthread_cancel ; <i32 (i64)*> [#uses=0]
@_ZL26__gthrw_pthread_mutex_lockP15pthread_mutex_t = alias weak i32 (%union.pthread_mutex_t*)* @pthread_mutex_lock ; <i32 (%union.pthread_mutex_t*)*> [#uses=0]
@_ZL29__gthrw_pthread_mutex_trylockP15pthread_mutex_t = alias weak i32 (%union.pthread_mutex_t*)* @pthread_mutex_trylock ; <i32 (%union.pthread_mutex_t*)*> [#uses=0]
@_ZL28__gthrw_pthread_mutex_unlockP15pthread_mutex_t = alias weak i32 (%union.pthread_mutex_t*)* @pthread_mutex_unlock ; <i32 (%union.pthread_mutex_t*)*> [#uses=0]
@_ZL26__gthrw_pthread_mutex_initP15pthread_mutex_tPK19pthread_mutexattr_t = alias weak i32 (%union.pthread_mutex_t*, %union.pthread_mutexattr_t*)* @pthread_mutex_init ; <i32 (%union.pthread_mutex_t*, %union.pthread_mutexattr_t*)*> [#uses=0]
@_ZL26__gthrw_pthread_key_createPjPFvPvE = alias weak i32 (i32*, void (i8*)*)* @pthread_key_create ; <i32 (i32*, void (i8*)*)*> [#uses=0]
@_ZL26__gthrw_pthread_key_deletej = alias weak i32 (i32)* @pthread_key_delete ; <i32 (i32)*> [#uses=0]
@_ZL30__gthrw_pthread_mutexattr_initP19pthread_mutexattr_t = alias weak i32 (%union.pthread_mutexattr_t*)* @pthread_mutexattr_init ; <i32 (%union.pthread_mutexattr_t*)*> [#uses=0]
@_ZL33__gthrw_pthread_mutexattr_settypeP19pthread_mutexattr_ti = alias weak i32 (%union.pthread_mutexattr_t*, i32)* @pthread_mutexattr_settype ; <i32 (%union.pthread_mutexattr_t*, i32)*> [#uses=0]
@_ZL33__gthrw_pthread_mutexattr_destroyP19pthread_mutexattr_t = alias weak i32 (%union.pthread_mutexattr_t*)* @pthread_mutexattr_destroy ; <i32 (%union.pthread_mutexattr_t*)*> [#uses=0]

define internal void @_GLOBAL__I_ob() {
entry:
  call void @_Z41__static_initialization_and_destruction_0ii(i32 1, i32 65535)
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal %struct.BaseClass* @_Z4initi(i32 %i) nounwind {
entry:
  %i_addr = alloca i32                            ; <i32*> [#uses=3]
  %retval = alloca %struct.BaseClass*             ; <%struct.BaseClass**> [#uses=2]
  %0 = alloca %struct.BaseClass*                  ; <%struct.BaseClass**> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i32 %i, i32* %i_addr
  %1 = load i32* %i_addr, align 4                 ; <i32> [#uses=1]
  %2 = icmp eq i32 %1, 1                          ; <i1> [#uses=1]
  br i1 %2, label %bb, label %bb1

bb:                                               ; preds = %entry
  store %struct.BaseClass* @ob, %struct.BaseClass** %0, align 8
  br label %bb4

bb1:                                              ; preds = %entry
  %3 = load i32* %i_addr, align 4                 ; <i32> [#uses=1]
  %4 = icmp eq i32 %3, 2                          ; <i1> [#uses=1]
  br i1 %4, label %bb2, label %bb3

bb2:                                              ; preds = %bb1
  store %struct.BaseClass* getelementptr inbounds (%struct.DerivedClass1* @derivedObject1, i64 0, i32 0), %struct.BaseClass** %0, align 8
  br label %bb4

bb3:                                              ; preds = %bb1
  br label %return

bb4:                                              ; preds = %bb2, %bb
  %5 = load %struct.BaseClass** %0, align 8       ; <%struct.BaseClass*> [#uses=1]
  store %struct.BaseClass* %5, %struct.BaseClass** %retval, align 8
  br label %return

return:                                           ; preds = %bb4, %bb3
  %retval5 = load %struct.BaseClass** %retval     ; <%struct.BaseClass*> [#uses=1]
  ret %struct.BaseClass* %retval5
}

define internal void @_Z4funcv() {
entry:
  %p = alloca %struct.BaseClass*                  ; <%struct.BaseClass**> [#uses=3]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %0 = call %struct.BaseClass* @_Z4initi(i32 2) nounwind ; <%struct.BaseClass*> [#uses=1]
  store %struct.BaseClass* %0, %struct.BaseClass** %p, align 8
  %1 = load %struct.BaseClass** %p, align 8       ; <%struct.BaseClass*> [#uses=1]
  %2 = getelementptr inbounds %struct.BaseClass* %1, i32 0, i32 0 ; <i32 (...)***> [#uses=1]
  %3 = load i32 (...)*** %2, align 8              ; <i32 (...)**> [#uses=1]
  %4 = getelementptr inbounds i32 (...)** %3, i64 0 ; <i32 (...)**> [#uses=1]
  %5 = load i32 (...)** %4, align 1               ; <i32 (...)*> [#uses=1]
  %6 = bitcast i32 (...)* %5 to void (%struct.BaseClass*)* ; <void (%struct.BaseClass*)*> [#uses=1]
  %7 = load %struct.BaseClass** %p, align 8       ; <%struct.BaseClass*> [#uses=1]
  call void %6(%struct.BaseClass* %7)
  br label %return

return:                                           ; preds = %entry
  ret void
}

define i32 @main() {
entry:
  %retval = alloca i32                            ; <i32*> [#uses=2]
  %0 = alloca i32                                 ; <i32*> [#uses=2]
  %p = alloca %struct.BaseClass*                  ; <%struct.BaseClass**> [#uses=9]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  %1 = call %struct.BaseClass* @_Z4initi(i32 2) nounwind ; <%struct.BaseClass*> [#uses=1]
  store %struct.BaseClass* %1, %struct.BaseClass** %p, align 8
  %2 = load %struct.BaseClass** %p, align 8       ; <%struct.BaseClass*> [#uses=1]
  %3 = getelementptr inbounds %struct.BaseClass* %2, i32 0, i32 0 ; <i32 (...)***> [#uses=1]
  %4 = load i32 (...)*** %3, align 8              ; <i32 (...)**> [#uses=1]
  %5 = getelementptr inbounds i32 (...)** %4, i64 0 ; <i32 (...)**> [#uses=1]
  %6 = load i32 (...)** %5, align 1               ; <i32 (...)*> [#uses=1]
  %7 = bitcast i32 (...)* %6 to void (%struct.BaseClass*)* ; <void (%struct.BaseClass*)*> [#uses=1]
  %8 = load %struct.BaseClass** %p, align 8       ; <%struct.BaseClass*> [#uses=1]
  call void %7(%struct.BaseClass* %8)
  %9 = call %struct.BaseClass* @_Z4initi(i32 3) nounwind ; <%struct.BaseClass*> [#uses=1]
  store %struct.BaseClass* %9, %struct.BaseClass** %p, align 8
  %10 = load %struct.BaseClass** %p, align 8      ; <%struct.BaseClass*> [#uses=1]
  %11 = getelementptr inbounds %struct.BaseClass* %10, i32 0, i32 0 ; <i32 (...)***> [#uses=1]
  %12 = load i32 (...)*** %11, align 8            ; <i32 (...)**> [#uses=1]
  %13 = getelementptr inbounds i32 (...)** %12, i64 0 ; <i32 (...)**> [#uses=1]
  %14 = load i32 (...)** %13, align 1             ; <i32 (...)*> [#uses=1]
  %15 = bitcast i32 (...)* %14 to void (%struct.BaseClass*)* ; <void (%struct.BaseClass*)*> [#uses=1]
  %16 = load %struct.BaseClass** %p, align 8      ; <%struct.BaseClass*> [#uses=1]
  call void %15(%struct.BaseClass* %16)
  %17 = call %struct.BaseClass* @_Z4initi(i32 4) nounwind ; <%struct.BaseClass*> [#uses=1]
  store %struct.BaseClass* %17, %struct.BaseClass** %p, align 8
  %18 = load %struct.BaseClass** %p, align 8      ; <%struct.BaseClass*> [#uses=1]
  %19 = getelementptr inbounds %struct.BaseClass* %18, i32 0, i32 0 ; <i32 (...)***> [#uses=1]
  %20 = load i32 (...)*** %19, align 8            ; <i32 (...)**> [#uses=1]
  %21 = getelementptr inbounds i32 (...)** %20, i64 0 ; <i32 (...)**> [#uses=1]
  %22 = load i32 (...)** %21, align 1             ; <i32 (...)*> [#uses=1]
  %23 = bitcast i32 (...)* %22 to void (%struct.BaseClass*)* ; <void (%struct.BaseClass*)*> [#uses=1]
  %24 = load %struct.BaseClass** %p, align 8      ; <%struct.BaseClass*> [#uses=1]
  call void %23(%struct.BaseClass* %24)
  call void @_Z4funcv()
  store i32 0, i32* %0, align 4
  %25 = load i32* %0, align 4                     ; <i32> [#uses=1]
  store i32 %25, i32* %retval, align 4
  br label %return

return:                                           ; preds = %entry
  %retval1 = load i32* %retval                    ; <i32> [#uses=1]
  ret i32 %retval1
}

define internal void @_ZN9BaseClass10myFunctionEv(%struct.BaseClass* %this) align 2 {
entry:
  %this_addr = alloca %struct.BaseClass*          ; <%struct.BaseClass**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.BaseClass* %this, %struct.BaseClass** %this_addr
  %0 = call %"struct.std::basic_ostream<char,std::char_traits<char> >"* @_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc(%"struct.std::basic_ostream<char,std::char_traits<char> >"* @_ZSt4cout, i8* getelementptr inbounds ([41 x i8]* @.str, i64 0, i64 0)) ; <%"struct.std::basic_ostream<char,std::char_traits<char> >"*> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  ret void
}

declare %"struct.std::basic_ostream<char,std::char_traits<char> >"* @_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc(%"struct.std::basic_ostream<char,std::char_traits<char> >"*, i8*)

define internal void @_ZN13DerivedClass110myFunctionEv(%struct.DerivedClass1* %this) align 2 {
entry:
  %this_addr = alloca %struct.DerivedClass1*      ; <%struct.DerivedClass1**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.DerivedClass1* %this, %struct.DerivedClass1** %this_addr
  %0 = call %"struct.std::basic_ostream<char,std::char_traits<char> >"* @_ZStlsISt11char_traitsIcEERSt13basic_ostreamIcT_ES5_PKc(%"struct.std::basic_ostream<char,std::char_traits<char> >"* @_ZSt4cout, i8* getelementptr inbounds ([47 x i8]* @.str1, i64 0, i64 0)) ; <%"struct.std::basic_ostream<char,std::char_traits<char> >"*> [#uses=0]
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @_ZN9BaseClassC2Ev(%struct.BaseClass* %this) nounwind align 2 {
entry:
  %this_addr = alloca %struct.BaseClass*          ; <%struct.BaseClass**> [#uses=2]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store %struct.BaseClass* %this, %struct.BaseClass** %this_addr
  %0 = load %struct.BaseClass** %this_addr, align 8 ; <%struct.BaseClass*> [#uses=1]
  %1 = getelementptr inbounds %struct.BaseClass* %0, i32 0, i32 0 ; <i32 (...)***> [#uses=1]
  store i32 (...)** getelementptr inbounds ([3 x i32 (...)*]* @_ZTV9BaseClass, i64 0, i64 2), i32 (...)*** %1, align 8
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
  store i32 (...)** getelementptr inbounds ([3 x i32 (...)*]* @_ZTV9BaseClass, i64 0, i64 2), i32 (...)*** %1, align 8
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
  store i32 (...)** getelementptr inbounds ([3 x i32 (...)*]* @_ZTV13DerivedClass1, i64 0, i64 2), i32 (...)*** %4, align 8
  br label %return

return:                                           ; preds = %entry
  ret void
}

define internal void @_Z41__static_initialization_and_destruction_0ii(i32 %__initialize_p, i32 %__priority) {
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
  call void @_ZNSt8ios_base4InitC1Ev(%"struct.std::ios_base::Init"* @_ZStL8__ioinit)
  %4 = call i32 @__cxa_atexit(void (i8*)* @__tcf_0, i8* null, i8* bitcast (i8** @__dso_handle to i8*)) nounwind ; <i32> [#uses=0]
  call void @_ZN9BaseClassC1Ev(%struct.BaseClass* @ob) nounwind
  call void @_ZN13DerivedClass1C1Ev(%struct.DerivedClass1* @derivedObject1) nounwind
  br label %bb2

bb2:                                              ; preds = %bb1, %bb, %entry
  br label %return

return:                                           ; preds = %bb2
  ret void
}

declare void @_ZNSt8ios_base4InitC1Ev(%"struct.std::ios_base::Init"*)

declare i32 @__cxa_atexit(void (i8*)*, i8*, i8*) nounwind

define internal void @__tcf_0(i8* %unnamed_arg) {
entry:
  %unnamed_arg_addr = alloca i8*                  ; <i8**> [#uses=1]
  %"alloca point" = bitcast i32 0 to i32          ; <i32> [#uses=0]
  store i8* %unnamed_arg, i8** %unnamed_arg_addr
  call void @_ZNSt8ios_base4InitD1Ev(%"struct.std::ios_base::Init"* @_ZStL8__ioinit)
  br label %return

return:                                           ; preds = %entry
  ret void
}

declare void @_ZNSt8ios_base4InitD1Ev(%"struct.std::ios_base::Init"*)

declare extern_weak i32 @pthread_once(i32*, void ()*)

declare extern_weak i8* @pthread_getspecific(i32)

declare extern_weak i32 @pthread_setspecific(i32, i8*)

declare extern_weak i32 @pthread_create(i64*, %union.pthread_attr_t*, i8* (i8*)*, i8*)

declare extern_weak i32 @pthread_cancel(i64)

declare extern_weak i32 @pthread_mutex_lock(%union.pthread_mutex_t*)

declare extern_weak i32 @pthread_mutex_trylock(%union.pthread_mutex_t*)

declare extern_weak i32 @pthread_mutex_unlock(%union.pthread_mutex_t*)

declare extern_weak i32 @pthread_mutex_init(%union.pthread_mutex_t*, %union.pthread_mutexattr_t*)

declare extern_weak i32 @pthread_key_create(i32*, void (i8*)*)

declare extern_weak i32 @pthread_key_delete(i32)

declare extern_weak i32 @pthread_mutexattr_init(%union.pthread_mutexattr_t*)

declare extern_weak i32 @pthread_mutexattr_settype(%union.pthread_mutexattr_t*, i32)

declare extern_weak i32 @pthread_mutexattr_destroy(%union.pthread_mutexattr_t*)

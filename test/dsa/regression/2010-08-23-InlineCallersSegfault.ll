; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"
;This presently causes td to segfault
;RUN: dsaopt %s -dsa-bu -disable-output
;RUN: dsaopt %s -dsa-td -disable-output
;RUN: dsaopt %s -dsa-eqtd -disable-output

%struct..0__pthread_mutex_s = type { i32, i32, i32, i32, i32, i32, %struct.__pthread_list_t }
%struct.__locale_struct = type { [13 x %struct.locale_data*], i16*, i32*, i32*, [13 x i8*] }
%struct.__pthread_list_t = type { %struct.__pthread_list_t*, %struct.__pthread_list_t* }
%struct.__si_class_type_info_pseudo = type { %struct.__type_info_pseudo, %"struct.std::type_info"* }
%struct.__type_info_pseudo = type { i8*, i8* }
%struct.ggBRDF = type { i32 (...)** }
%"struct.ggBST<ggMaterial>" = type { %"struct.ggBSTNode<ggMaterial>"*, i32 }
%"struct.ggBST<ggRasterSurfaceTexture>" = type { %"struct.ggBSTNode<ggRasterSurfaceTexture>"*, i32 }
%"struct.ggBST<ggSolidTexture>" = type { %"struct.ggBSTNode<ggSolidTexture>"*, i32 }
%"struct.ggBST<ggSpectrum>" = type { %"struct.ggBSTNode<ggSpectrum>"*, i32 }
%"struct.ggBST<mrObjectRecord>" = type { %"struct.ggBSTNode<mrObjectRecord>"*, i32 }
%"struct.ggBSTNode<ggMaterial>" = type { %"struct.ggBSTNode<ggMaterial>"*, %"struct.ggBSTNode<ggMaterial>"*, %struct.ggString, %struct.ggMaterial* }
%"struct.ggBSTNode<ggRasterSurfaceTexture>" = type { %"struct.ggBSTNode<ggRasterSurfaceTexture>"*, %"struct.ggBSTNode<ggRasterSurfaceTexture>"*, %struct.ggString, %struct.ggRasterSurfaceTexture* }
%"struct.ggBSTNode<ggSolidTexture>" = type { %"struct.ggBSTNode<ggSolidTexture>"*, %"struct.ggBSTNode<ggSolidTexture>"*, %struct.ggString, %struct.ggBRDF* }
%"struct.ggBSTNode<ggSpectrum>" = type { %"struct.ggBSTNode<ggSpectrum>"*, %"struct.ggBSTNode<ggSpectrum>"*, %struct.ggString, %struct.ggSpectrum* }
%"struct.ggBSTNode<mrObjectRecord>" = type { %"struct.ggBSTNode<mrObjectRecord>"*, %"struct.ggBSTNode<mrObjectRecord>"*, %struct.ggString, %struct.mrObjectRecord* }
%struct.ggBox3 = type { %struct.ggPoint3, %struct.ggPoint3 }
%"struct.ggDictionary<ggMaterial>" = type { %"struct.ggBST<ggMaterial>" }
%"struct.ggDictionary<ggRasterSurfaceTexture>" = type { %"struct.ggBST<ggRasterSurfaceTexture>" }
%"struct.ggDictionary<ggSolidTexture>" = type { %"struct.ggBST<ggSolidTexture>" }
%"struct.ggDictionary<ggSpectrum>" = type { %"struct.ggBST<ggSpectrum>" }
%"struct.ggDictionary<mrObjectRecord>" = type { %"struct.ggBST<mrObjectRecord>" }
%struct.ggHAffineMatrix3 = type { %struct.ggHMatrix3 }
%struct.ggHBoxMatrix3 = type { %struct.ggHAffineMatrix3 }
%struct.ggHMatrix3 = type { [4 x [4 x double]] }
%struct.ggMaterial = type { i32 (...)**, %struct.ggBRDF* }
%struct.ggMaterialRecord = type { %struct.ggPoint2, %struct.ggBox3, %struct.ggBox3, %struct.ggSpectrum, %struct.ggSpectrum, %struct.ggSpectrum, %struct.ggBRDF*, i32, i32, i32, i32 }
%struct.ggONB3 = type { %struct.ggPoint3, %struct.ggPoint3, %struct.ggPoint3 }
%struct.ggPoint2 = type { [2 x double] }
%struct.ggPoint3 = type { [3 x double] }
%"struct.ggRGBPixel<char>" = type { [3 x i8], i8 }
%"struct.ggRaster<ggRGBPixel<unsigned char> >" = type { i32, i32, %"struct.ggRGBPixel<char>"* }
%struct.ggRasterSurfaceTexture = type { %"struct.ggRaster<ggRGBPixel<unsigned char> >"* }
%struct.ggSpectrum = type { [8 x float] }
%struct.ggString = type { %"struct.ggString::StringRep"* }
%"struct.ggString::StringRep" = type { i32, i32, [1 x i8] }
%"struct.ggTrain<mrPixelRenderer*>" = type { %struct.ggBRDF**, i32, i32 }
%struct.locale_data = type opaque
%struct.mrObjectRecord = type { %struct.ggHBoxMatrix3, %struct.ggHBoxMatrix3, %struct.mrSurfaceList, %struct.ggMaterial*, i32, %struct.ggRasterSurfaceTexture*, %struct.ggBRDF*, i32, i32 }
%struct.mrScene = type { %struct.ggSpectrum, %struct.ggSpectrum, %struct.ggBRDF*, %struct.ggBRDF*, %struct.ggBRDF*, i32, double, %"struct.ggDictionary<mrObjectRecord>", %"struct.ggDictionary<ggRasterSurfaceTexture>", %"struct.ggDictionary<ggSolidTexture>", %"struct.ggDictionary<ggSpectrum>", %"struct.ggDictionary<ggMaterial>" }
%struct.mrSurfaceList = type { %struct.ggBRDF, %"struct.ggTrain<mrPixelRenderer*>" }
%struct.mrSurfaceTexture = type { %struct.ggBRDF, %struct.ggBRDF*, %struct.ggSpectrum, %struct.ggSpectrum, %struct.ggSpectrum, %struct.ggRasterSurfaceTexture*, double, double }
%struct.mrViewingHitRecord = type { double, %struct.ggPoint3, %struct.ggONB3, %struct.ggPoint2, double, %struct.ggSpectrum, %struct.ggSpectrum, i32, i32, i32, i32 }
%"struct.std::__codecvt_abstract_base<char,char,__mbstate_t>.base.64" = type { %"struct.std::locale::facet" }
%"struct.std::basic_ios<char,std::char_traits<char> >" = type { %"struct.std::ios_base", %"struct.std::basic_ostream<char,std::char_traits<char> >"*, i8, i8, %"struct.std::basic_streambuf<char,std::char_traits<char> >"*, %"struct.std::ctype<char>"*, %"struct.std::__codecvt_abstract_base<char,char,__mbstate_t>.base.64"*, %"struct.std::__codecvt_abstract_base<char,char,__mbstate_t>.base.64"* }
%"struct.std::basic_istream<char,std::char_traits<char> >" = type { i32 (...)**, i64, %"struct.std::basic_ios<char,std::char_traits<char> >" }
%"struct.std::basic_ostream<char,std::char_traits<char> >" = type { i32 (...)**, %"struct.std::basic_ios<char,std::char_traits<char> >" }
%"struct.std::basic_streambuf<char,std::char_traits<char> >" = type { i32 (...)**, i8*, i8*, i8*, i8*, i8*, i8*, %"struct.std::locale" }
%"struct.std::ctype<char>" = type { %"struct.std::locale::facet", %struct.__locale_struct*, i8, i32*, i32*, i16*, i8, [256 x i8], [256 x i8], i8 }
%"struct.std::ios_base" = type { i32 (...)**, i64, i64, i32, i32, i32, %"struct.std::ios_base::_Callback_list"*, %"struct.std::ios_base::_Words", [8 x %"struct.std::ios_base::_Words"], i32, %"struct.std::ios_base::_Words"*, %"struct.std::locale" }
%"struct.std::ios_base::_Callback_list" = type { %"struct.std::ios_base::_Callback_list"*, void (i32, %"struct.std::ios_base"*, i32)*, i32, i32 }
%"struct.std::ios_base::_Words" = type { i8*, i64 }
%"struct.std::locale" = type { %"struct.std::locale::_Impl"* }
%"struct.std::locale::_Impl" = type { i32, %"struct.std::locale::facet"**, i64, %"struct.std::locale::facet"**, i8** }
%"struct.std::locale::facet" = type { i32 (...)**, i32 }
%"struct.std::type_info" = type { i32 (...)**, i8* }
%"union.__mbstate_t::._2" = type { i32 }
%union.pthread_attr_t = type { i64, [12 x i32] }
%union.pthread_mutex_t = type { %struct..0__pthread_mutex_s }

@_ZTV16mrSurfaceTexture = internal constant [9 x i32 (...)*] [i32 (...)* null, i32 (...)* bitcast (%struct.__si_class_type_info_pseudo* @_ZTI16mrSurfaceTexture to i32 (...)*), i32 (...)* bitcast (i32 (%struct.mrSurfaceTexture*, %"struct.std::basic_ostream<char,std::char_traits<char> >"*)* @_ZNK16mrSurfaceTexture5printERSo to i32 (...)*), i32 (...)* bitcast (i32 (%struct.mrSurfaceTexture*, %struct.ggBox3*, double, double, double, double*, %struct.ggPoint3*, i32*, %struct.ggSpectrum*)* @_ZNK16mrSurfaceTexture9shadowHitERK6ggRay3dddRdR9ggVector3RiR10ggSpectrum to i32 (...)*), i32 (...)* bitcast (i32 (%struct.mrSurfaceTexture*, %struct.ggBox3*, double, double, double, %struct.mrViewingHitRecord*, %struct.ggMaterialRecord*)* @_ZNK16mrSurfaceTexture10viewingHitERK6ggRay3dddR18mrViewingHitRecordR16ggMaterialRecord to i32 (...)*), i32 (...)* bitcast (i32 (%struct.mrSurfaceTexture*, double, double, %struct.ggBox3*)* @_ZNK16mrSurfaceTexture11boundingBoxEddR6ggBox3 to i32 (...)*), i32 (...)* bitcast (i32 (%struct.ggBRDF*, double, double, %struct.ggBox3*)* @_ZNK9mrSurface11overlapsBoxEddRK6ggBox3 to i32 (...)*), i32 (...)* bitcast (i32 (%struct.mrSurfaceTexture*, %struct.ggPoint3*, %struct.ggPoint3*, %struct.ggPoint2*, double, %struct.ggPoint3*, double*)* @_ZNK16mrSurfaceTexture18selectVisiblePointERK8ggPoint3RK9ggVector3RK8ggPoint2dRS0_Rd to i32 (...)*), i32 (...)* bitcast (i32 (%struct.ggBRDF*, %struct.ggPoint3*, %struct.ggPoint3*, double, %struct.ggSpectrum*)* @_ZNK9mrSurface25approximateDirectRadianceERK8ggPoint3RK9ggVector3dR10ggSpectrum to i32 (...)*)], align 32 ; <[9 x i32 (...)*]*> [#uses=1]
@_ZTI16mrSurfaceTexture = external constant %struct.__si_class_type_info_pseudo, align 16 ; <%struct.__si_class_type_info_pseudo*> [#uses=1]

@_ZL20__gthrw_pthread_oncePiPFvvE = alias weak i32 (i32*, void ()*)* @pthread_once ; <i32 (i32*, void ()*)*> [#uses=0]
@_ZL27__gthrw_pthread_getspecificj = alias weak i8* (i32)* @pthread_getspecific ; <i8* (i32)*> [#uses=0]
@_ZL27__gthrw_pthread_setspecificjPKv = alias weak i32 (i32, i8*)* @pthread_setspecific ; <i32 (i32, i8*)*> [#uses=0]
@_ZL22__gthrw_pthread_createPmPK14pthread_attr_tPFPvS3_ES3_ = alias weak i32 (i64*, %union.pthread_attr_t*, i8* (i8*)*, i8*)* @pthread_create ; <i32 (i64*, %union.pthread_attr_t*, i8* (i8*)*, i8*)*> [#uses=0]
@_ZL22__gthrw_pthread_cancelm = alias weak i32 (i64)* @pthread_cancel ; <i32 (i64)*> [#uses=0]
@_ZL26__gthrw_pthread_mutex_lockP15pthread_mutex_t = alias weak i32 (%union.pthread_mutex_t*)* @pthread_mutex_lock ; <i32 (%union.pthread_mutex_t*)*> [#uses=0]
@_ZL29__gthrw_pthread_mutex_trylockP15pthread_mutex_t = alias weak i32 (%union.pthread_mutex_t*)* @pthread_mutex_trylock ; <i32 (%union.pthread_mutex_t*)*> [#uses=0]
@_ZL28__gthrw_pthread_mutex_unlockP15pthread_mutex_t = alias weak i32 (%union.pthread_mutex_t*)* @pthread_mutex_unlock ; <i32 (%union.pthread_mutex_t*)*> [#uses=0]
@_ZL26__gthrw_pthread_mutex_initP15pthread_mutex_tPK19pthread_mutexattr_t = alias weak i32 (%union.pthread_mutex_t*, %"union.__mbstate_t::._2"*)* @pthread_mutex_init ; <i32 (%union.pthread_mutex_t*, %"union.__mbstate_t::._2"*)*> [#uses=0]
@_ZL26__gthrw_pthread_key_createPjPFvPvE = alias weak i32 (i32*, void (i8*)*)* @pthread_key_create ; <i32 (i32*, void (i8*)*)*> [#uses=0]
@_ZL26__gthrw_pthread_key_deletej = alias weak i32 (i32)* @pthread_key_delete ; <i32 (i32)*> [#uses=0]
@_ZL30__gthrw_pthread_mutexattr_initP19pthread_mutexattr_t = alias weak i32 (%"union.__mbstate_t::._2"*)* @pthread_mutexattr_init ; <i32 (%"union.__mbstate_t::._2"*)*> [#uses=0]
@_ZL33__gthrw_pthread_mutexattr_settypeP19pthread_mutexattr_ti = alias weak i32 (%"union.__mbstate_t::._2"*, i32)* @pthread_mutexattr_settype ; <i32 (%"union.__mbstate_t::._2"*, i32)*> [#uses=0]
@_ZL33__gthrw_pthread_mutexattr_destroyP19pthread_mutexattr_t = alias weak i32 (%"union.__mbstate_t::._2"*)* @pthread_mutexattr_destroy ; <i32 (%"union.__mbstate_t::._2"*)*> [#uses=0]

declare i8* @_Znwm(i64)

define fastcc void @_ZN7mrScene4ReadERSi(%struct.mrScene* %this, %"struct.std::basic_istream<char,std::char_traits<char> >"* %surfaces) nounwind align 2 {
entry:
  br i1 undef, label %_ZrsRSiR8ggString.exit794, label %bb.i792

bb.i792:                                          ; preds = %entry
  br label %_ZrsRSiR8ggString.exit794

_ZrsRSiR8ggString.exit794:                        ; preds = %bb.i792, %entry
  br i1 undef, label %bb445.preheader, label %bb

bb445.preheader:                                  ; preds = %_ZrsRSiR8ggString.exit794
  br i1 undef, label %_ZrsRSiR8ggString.exit, label %bb.i61

bb:                                               ; preds = %_ZrsRSiR8ggString.exit794
  unreachable

bb1:                                              ; preds = %_ZrsRSiR8ggString.exit
  unreachable

bb.i61:                                           ; preds = %bb445.preheader
  unreachable

_ZrsRSiR8ggString.exit:                           ; preds = %bb445.preheader
  br i1 undef, label %bb447, label %bb1

bb447:                                            ; preds = %_ZrsRSiR8ggString.exit
  br i1 undef, label %_ZNK12ggDictionaryI14mrObjectRecordE9fillTrainER7ggTrainIPS0_E.exit.i, label %bb.i.i.i

bb.i.i.i:                                         ; preds = %bb447
  unreachable

_ZNK12ggDictionaryI14mrObjectRecordE9fillTrainER7ggTrainIPS0_E.exit.i: ; preds = %bb447
  br i1 undef, label %_ZN8ggStringD1Ev.exit50.i, label %_ZN8ggString9StringRepD1Ev.exit.i48.i

_ZN8ggString9StringRepD1Ev.exit.i48.i:            ; preds = %_ZNK12ggDictionaryI14mrObjectRecordE9fillTrainER7ggTrainIPS0_E.exit.i
  unreachable

_ZN8ggStringD1Ev.exit50.i:                        ; preds = %_ZNK12ggDictionaryI14mrObjectRecordE9fillTrainER7ggTrainIPS0_E.exit.i
  br i1 undef, label %_ZN8ggStringD1Ev.exit44.i, label %_ZN8ggString9StringRepD1Ev.exit.i42.i

_ZN8ggString9StringRepD1Ev.exit.i42.i:            ; preds = %_ZN8ggStringD1Ev.exit50.i
  unreachable

_ZN8ggStringD1Ev.exit44.i:                        ; preds = %_ZN8ggStringD1Ev.exit50.i
  br i1 undef, label %_ZN8ggStringD1Ev.exit.i, label %_ZN8ggString9StringRepD1Ev.exit.i.i

_ZN8ggString9StringRepD1Ev.exit.i.i:              ; preds = %_ZN8ggStringD1Ev.exit44.i
  br label %_ZN8ggStringD1Ev.exit.i

_ZN8ggStringD1Ev.exit.i:                          ; preds = %_ZN8ggString9StringRepD1Ev.exit.i.i, %_ZN8ggStringD1Ev.exit44.i
  br i1 undef, label %bb.nph65.i, label %bb30.i

bb.nph65.i:                                       ; preds = %_ZN8ggStringD1Ev.exit.i
  br i1 undef, label %bb12.i, label %bb10.loopexit.i

bb10.loopexit.i:                                  ; preds = %bb.nph65.i
  unreachable

bb12.i:                                           ; preds = %bb.nph65.i
  %0 = call i8* @_Znwm(i64 320) nounwind          ; <i8*> [#uses=2]
  %1 = getelementptr inbounds i8* %0, i64 264     ; <i8*> [#uses=1]
  %2 = bitcast i8* %1 to %struct.ggBRDF**         ; <%struct.ggBRDF**> [#uses=1]
  %3 = load %struct.ggBRDF** %2, align 8          ; <%struct.ggBRDF*> [#uses=2]
  %4 = getelementptr inbounds %struct.ggBRDF* %3, i64 0, i32 0 ; <i32 (...)***> [#uses=1]
  %5 = load i32 (...)*** %4, align 8              ; <i32 (...)**> [#uses=1]
  %6 = getelementptr inbounds i32 (...)** %5, i64 3 ; <i32 (...)**> [#uses=1]
  %7 = load i32 (...)** %6, align 8               ; <i32 (...)*> [#uses=1]
  %8 = bitcast i32 (...)* %7 to i32 (%struct.ggBRDF*, double, double, %struct.ggBox3*)* ; <i32 (%struct.ggBRDF*, double, double, %struct.ggBox3*)*> [#uses=1]
  %9 = call i32 %8(%struct.ggBRDF* %3, double 0.000000e+00, double 0.000000e+00, %struct.ggBox3* undef) nounwind ; <i32> [#uses=0]
  br i1 undef, label %_ZN10mrInstance9SetMatrixERK19ggHRigidBodyMatrix3S2_.exit.i, label %bb.nph89.bb.nph89.split_crit_edge.i.i

bb.nph89.bb.nph89.split_crit_edge.i.i:            ; preds = %bb12.i
  unreachable

_ZN10mrInstance9SetMatrixERK19ggHRigidBodyMatrix3S2_.exit.i: ; preds = %bb12.i
  %10 = bitcast i8* %0 to %struct.ggBRDF*         ; <%struct.ggBRDF*> [#uses=2]
  br i1 undef, label %bb19.i, label %bb16.i

bb16.i:                                           ; preds = %_ZN10mrInstance9SetMatrixERK19ggHRigidBodyMatrix3S2_.exit.i
  %11 = call i8* @_Znwm(i64 136) nounwind         ; <i8*> [#uses=3]
  %12 = bitcast i8* %11 to i32 (...)***           ; <i32 (...)***> [#uses=1]
  store i32 (...)** getelementptr inbounds ([9 x i32 (...)*]* @_ZTV16mrSurfaceTexture, i64 0, i64 2), i32 (...)*** %12, align 8
  %13 = getelementptr inbounds i8* %11, i64 8     ; <i8*> [#uses=1]
  %14 = bitcast i8* %13 to %struct.ggBRDF**       ; <%struct.ggBRDF**> [#uses=1]
  store %struct.ggBRDF* %10, %struct.ggBRDF** %14, align 8
  %15 = bitcast i8* %11 to %struct.ggBRDF*        ; <%struct.ggBRDF*> [#uses=1]
  br label %bb19.i

bb19.i:                                           ; preds = %bb16.i, %_ZN10mrInstance9SetMatrixERK19ggHRigidBodyMatrix3S2_.exit.i
  %surfPtr15.0.i = phi %struct.ggBRDF* [ %10, %_ZN10mrInstance9SetMatrixERK19ggHRigidBodyMatrix3S2_.exit.i ], [ %15, %bb16.i ] ; <%struct.ggBRDF*> [#uses=0]
  unreachable

bb30.i:                                           ; preds = %_ZN8ggStringD1Ev.exit.i
  br label %bb2.i2.i.i.i.i

bb2.i2.i.i.i.i:                                   ; preds = %bb2.i2.i.i.i.i, %bb30.i
  br label %bb2.i2.i.i.i.i
}

declare i32 @_ZNK9mrSurface11overlapsBoxEddRK6ggBox3(%struct.ggBRDF*, double, double, %struct.ggBox3* nocapture) nounwind align 2

declare i32 @_ZNK9mrSurface25approximateDirectRadianceERK8ggPoint3RK9ggVector3dR10ggSpectrum(%struct.ggBRDF* nocapture, %struct.ggPoint3* nocapture, %struct.ggPoint3* nocapture, double, %struct.ggSpectrum* nocapture) nounwind align 2

declare i32 @_ZNK16mrSurfaceTexture5printERSo(%struct.mrSurfaceTexture*, %"struct.std::basic_ostream<char,std::char_traits<char> >"*) nounwind align 2

declare i32 @_ZNK16mrSurfaceTexture9shadowHitERK6ggRay3dddRdR9ggVector3RiR10ggSpectrum(%struct.mrSurfaceTexture* nocapture, %struct.ggBox3*, double, double, double, double*, %struct.ggPoint3*, i32*, %struct.ggSpectrum*) nounwind align 2

declare i32 @_ZNK16mrSurfaceTexture11boundingBoxEddR6ggBox3(%struct.mrSurfaceTexture* nocapture, double, double, %struct.ggBox3*) nounwind align 2

declare i32 @_ZNK16mrSurfaceTexture18selectVisiblePointERK8ggPoint3RK9ggVector3RK8ggPoint2dRS0_Rd(%struct.mrSurfaceTexture* nocapture, %struct.ggPoint3*, %struct.ggPoint3*, %struct.ggPoint2*, double, %struct.ggPoint3*, double*) nounwind align 2

declare i32 @_ZNK16mrSurfaceTexture10viewingHitERK6ggRay3dddR18mrViewingHitRecordR16ggMaterialRecord(%struct.mrSurfaceTexture*, %struct.ggBox3*, double, double, double, %struct.mrViewingHitRecord*, %struct.ggMaterialRecord*) nounwind align 2

declare i32 @pthread_once(i32*, void ()*)

declare i8* @pthread_getspecific(i32)

declare i32 @pthread_setspecific(i32, i8*)

declare i32 @pthread_create(i64*, %union.pthread_attr_t*, i8* (i8*)*, i8*)

declare i32 @pthread_cancel(i64)

declare i32 @pthread_mutex_lock(%union.pthread_mutex_t*)

declare i32 @pthread_mutex_trylock(%union.pthread_mutex_t*)

declare i32 @pthread_mutex_unlock(%union.pthread_mutex_t*)

declare i32 @pthread_mutex_init(%union.pthread_mutex_t*, %"union.__mbstate_t::._2"*)

declare i32 @pthread_key_create(i32*, void (i8*)*)

declare i32 @pthread_key_delete(i32)

declare i32 @pthread_mutexattr_init(%"union.__mbstate_t::._2"*)

declare i32 @pthread_mutexattr_settype(%"union.__mbstate_t::._2"*, i32)

declare i32 @pthread_mutexattr_destroy(%"union.__mbstate_t::._2"*)

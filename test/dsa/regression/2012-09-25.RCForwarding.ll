;RUN: dsaopt %s -dsa-eqtd -disable-output
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.anon = type { i32, %struct._icmBase* (%struct._icc*)* }
%struct._icmBase = type { i32, %struct._icc*, i32, i32, i32 (%struct._icmBase*)*, i32 (%struct._icmBase*, i64, i64)*, i32 (%struct._icmBase*, i64)*, void (%struct._icmBase*)*, void (%struct._icmBase*, %struct._IO_FILE*, i32)*, i32 (%struct._icmBase*)* }
%struct._icc = type { i32 (%struct._icc*)*, i32 (%struct._icc*, %struct._icmFile*, i64)*, i32 (%struct._icc*, %struct._icmFile*, i64)*, void (%struct._icc*, %struct._IO_FILE*, i32)*, void (%struct._icc*)*, i32 (%struct._icc*, i32)*, %struct._icmBase* (%struct._icc*, i32)*, %struct._icmBase* (%struct._icc*, i32, i32)*, i32 (%struct._icc*, i32, i32)*, %struct._icmBase* (%struct._icc*, i32, i32)*, i32 (%struct._icc*, i32)*, i32 (%struct._icc*)*, i32 (%struct._icc*, i32)*, %struct._icmLuBase* (%struct._icc*, i32, i32, i32, i32)*, %struct._icmHeader*, [512 x i8], i32, %struct._icmAlloc*, i32, %struct._icmFile*, i64, i32, %struct.icmTag* }
%struct._icmFile = type { i32 (%struct._icmFile*, i64)*, i64 (%struct._icmFile*, i8*, i64, i64)*, i64 (%struct._icmFile*, i8*, i64, i64)*, i32 (%struct._icmFile*)*, i32 (%struct._icmFile*)* }
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
%struct._icmLuBase = type { i32, %struct._icc*, i32, i32, %struct.icmXYZNumber, %struct.icmXYZNumber, %struct.icmXYZNumber, [3 x [3 x double]], [3 x [3 x double]], i32, i32, i32, i32, i32, i32, void (%struct._icmLuBase*)*, void (%struct._icmLuBase*, i32*, i32*, i32*, i32*)*, void (%struct._icmLuBase*, i32*, i32*, i32*, i32*, i32*, i32*, i32*, i32*)*, void (%struct._icmLuBase*, double*, double*, double*, double*)*, void (%struct._icmLuBase*, %struct.icmXYZNumber*, %struct.icmXYZNumber*)*, i32 (%struct._icmLuBase*, double*, double*)* }
%struct.icmXYZNumber = type { double, double, double }
%struct._icmHeader = type { i32 (%struct._icmHeader*)*, i32 (%struct._icmHeader*, i64, i64)*, i32 (%struct._icmHeader*, i64)*, void (%struct._icmHeader*)*, %struct._icc*, i32, void (%struct._icmHeader*, %struct._IO_FILE*, i32)*, i32, i32, i32, i32, i32, i32, %struct.icmUint64, i32, i32, i32, i32, i32, i32, %struct._icmDateTimeNumber, i32, %struct.icmXYZNumber }
%struct.icmUint64 = type { i64, i64 }
%struct._icmDateTimeNumber = type { i32, %struct._icc*, i32, i32, i32 (%struct._icmBase*)*, i32 (%struct._icmBase*, i64, i64)*, i32 (%struct._icmBase*, i64)*, void (%struct._icmBase*)*, void (%struct._icmBase*, %struct._IO_FILE*, i32)*, i32 (%struct._icmBase*)*, i32, i32, i32, i32, i32, i32 }
%struct._icmAlloc = type { i8* (%struct._icmAlloc*, i64)*, i8* (%struct._icmAlloc*, i64, i64)*, i8* (%struct._icmAlloc*, i8*, i64)*, void (%struct._icmAlloc*, i8*)*, void (%struct._icmAlloc*)* }
%struct.icmTag = type { i32, i32, i32, i32, %struct._icmBase* }

@typetable = internal unnamed_addr constant [25 x %struct.anon] [%struct.anon { i32 1668441193, %struct._icmBase* (%struct._icc*)* @new_icmCrdInfo }, %struct.anon { i32 1668641398, %struct._icmBase* (%struct._icc*)* @new_icmCurve }, %struct.anon { i32 1684108385, %struct._icmBase* (%struct._icc*)* @new_icmData }, %struct.anon { i32 1685350765, %struct._icmBase* (%struct._icc*)* @new_icmDateTimeNumber }, %struct.anon { i32 1835430962, %struct._icmBase* (%struct._icc*)* @new_icmLut }, %struct.anon { i32 1835430961, %struct._icmBase* (%struct._icc*)* @new_icmLut }, %struct.anon { i32 1835360627, %struct._icmBase* (%struct._icc*)* @new_icmMeasurement }, %struct.anon { i32 1852010348, %struct._icmBase* (%struct._icc*)* @new_icmNamedColor }, %struct.anon { i32 1852009522, %struct._icmBase* (%struct._icc*)* @new_icmNamedColor }, %struct.anon { i32 1886610801, %struct._icmBase* (%struct._icc*)* @new_icmProfileSequenceDesc }, %struct.anon { i32 1936077618, %struct._icmBase* (%struct._icc*)* @new_icmS15Fixed16Array }, %struct.anon { i32 1935897198, %struct._icmBase* (%struct._icc*)* @new_icmScreening }, %struct.anon { i32 1936287520, %struct._icmBase* (%struct._icc*)* @new_icmSignature }, %struct.anon { i32 1684370275, %struct._icmBase* (%struct._icc*)* @new_icmTextDescription }, %struct.anon { i32 1952807028, %struct._icmBase* (%struct._icc*)* @new_icmText }, %struct.anon { i32 1969632050, %struct._icmBase* (%struct._icc*)* @new_icmU16Fixed16Array }, %struct.anon { i32 1650877472, %struct._icmBase* (%struct._icc*)* @new_icmUcrBg }, %struct.anon { i32 1986226036, %struct._icmBase* (%struct._icc*)* @new_icmVideoCardGamma }, %struct.anon { i32 1969828150, %struct._icmBase* (%struct._icc*)* @new_icmUInt16Array }, %struct.anon { i32 1969828658, %struct._icmBase* (%struct._icc*)* @new_icmUInt32Array }, %struct.anon { i32 1969829428, %struct._icmBase* (%struct._icc*)* @new_icmUInt64Array }, %struct.anon { i32 1969827896, %struct._icmBase* (%struct._icc*)* @new_icmUInt8Array }, %struct.anon { i32 1986618743, %struct._icmBase* (%struct._icc*)* @new_icmViewingConditions }, %struct.anon { i32 1482250784, %struct._icmBase* (%struct._icc*)* @new_icmXYZArray }, %struct.anon { i32 -1, %struct._icmBase* (%struct._icc*)* null }], align 16
@llvm.compiler.used = appending global [0 x i8*] zeroinitializer, section "llvm.metadata"

declare i32 @test() nounwind readnone

define void @new_icc_a() noreturn nounwind uwtable {
entry:
  store %struct._icmBase* (%struct._icc*, i32, i32)* @icc_add_tag, %struct._icmBase* (%struct._icc*, i32, i32)** inttoptr (i64 56 to %struct._icmBase* (%struct._icc*, i32, i32)**), align 8, !tbaa !0
  tail call void @llvm.trap()
  unreachable
}

define void @icc_dump() noreturn nounwind uwtable readnone {
entry:
  unreachable
}

define noalias %struct._icmBase* @icc_add_tag(%struct._icc* nocapture %p, i32 %sig, i32 %ttype) noreturn nounwind uwtable {
entry:
  %0 = tail call i32 @test() nounwind readnone
  %cmp = icmp eq i32 %0, 44
  br i1 %cmp, label %for.body30, label %for.body
for.body:                                         ; preds = %entry, %for.body
  br label %for.body

for.body30:                                       ; preds = %entry, %for.body30
  %idxprom26144 = phi i64 [ 44, %for.body30 ], [ 0, %entry ]
  %ttype28 = getelementptr inbounds [25 x %struct.anon]* @typetable, i64 0, i64 %idxprom26144, i32 0
  %1 = load i32* %ttype28, align 16, !tbaa !1
  %cmp34 = icmp eq i32 %1, %ttype
  br i1 %cmp34, label %for.cond49.preheader, label %for.body30

for.cond49.preheader:                             ; preds = %for.body30
  tail call void @llvm.trap()
  unreachable
}

define void @icc_link_tag() noreturn nounwind uwtable readnone {
entry:
  unreachable
}
declare %struct._icmBase* @new_icmCrdInfo(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmCurve(%struct._icc*) nounwind uwtable

define noalias %struct._icmBase* @new_icmData(%struct._icc* %icp) nounwind uwtable {
entry:
  %al = getelementptr inbounds %struct._icc* %icp, i64 0, i32 17
  %0 = load %struct._icmAlloc** %al, align 8, !tbaa !0
  %calloc = getelementptr inbounds %struct._icmAlloc* %0, i64 0, i32 1
  %1 = load i8* (%struct._icmAlloc*, i64, i64)** %calloc, align 8, !tbaa !0
  %call = tail call i8* %1(%struct._icmAlloc* %0, i64 1, i64 96) nounwind
  %get_size = getelementptr inbounds i8* %call, i64 24
  %2 = bitcast i8* %get_size to i32 (%struct._icmBase*)**
  store i32 (%struct._icmBase*)* @icmData_get_size, i32 (%struct._icmBase*)** %2, align 8, !tbaa !0
  %icp2 = getelementptr inbounds i8* %call, i64 8
  %3 = bitcast i8* %icp2 to %struct._icc**
  store %struct._icc* %icp, %struct._icc** %3, align 8, !tbaa !0
  ret %struct._icmBase* null
}

declare %struct._icmBase* @new_icmDateTimeNumber(%struct._icc*) nounwind uwtable
declare %struct._icmBase* @new_icmLut(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmMeasurement(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmNamedColor(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmProfileSequenceDesc(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmS15Fixed16Array(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmScreening(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmSignature(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmTextDescription(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmText(%struct._icc*) nounwind uwtable
declare %struct._icmBase* @new_icmU16Fixed16Array(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmUcrBg(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmVideoCardGamma(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmUInt16Array(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmUInt32Array(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmUInt64Array(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmUInt8Array(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmViewingConditions(%struct._icc*) nounwind uwtable

declare %struct._icmBase* @new_icmXYZArray(%struct._icc*) nounwind uwtable

define void @icmScreening_delete() noreturn nounwind uwtable readnone {
entry:
  unreachable
}
declare i32 @icmData_get_size(%struct._icmBase* nocapture) nounwind uwtable readonly

declare void @llvm.trap() noreturn nounwind

!0 = metadata !{metadata !"any pointer", metadata !1}
!1 = metadata !{metadata !"omnipotent char", metadata !2}
!2 = metadata !{metadata !"Simple C/C++ TBAA"}


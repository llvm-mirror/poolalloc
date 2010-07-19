;RUN: dsaopt %s -dsa-eq -disable-output
; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%0 = type { %struct._IO_FILE*, %struct._IO_FILE*, [8192 x i8] }
%struct.TypCollectors = type { i8*, void (%struct.TypHeader*, i64)*, i32 (i64*, %struct.TypHeader*)* }
%struct.TypHeader = type { i64, %struct.TypHeader**, [3 x i8], i8 }
%struct.TypInputFile = type { i64, [64 x i8], [256 x i8], i8*, i64 }
%struct.TypOutputFile = type { i64, [256 x i8], i64, i64, i64, i64 }
%struct._IO_FILE = type { i32, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, i8*, %struct._IO_marker*, %struct._IO_FILE*, i32, i32, i64, i16, i8, [1 x i8], i8*, i64, i8*, i8*, i8*, i8*, i64, i32, [20 x i8] }
%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, i32 }
%struct.__jmp_buf_tag = type { [8 x i64], i32, %struct.__sigset_t }
%struct.__sigset_t = type { [16 x i64] }
%struct.anon = type { i64, i64, [4 x i8] }
%struct.termio = type { i16, i16, i16, i16, i8, [8 x i8] }
%struct.tms = type { i64, i64, i64, i64 }

@Collectors = external constant [6 x %struct.TypCollectors], align 32 ; <[6 x %struct.TypCollectors]*> [#uses=0]
@CSeries = external global i64*                   ; <i64**> [#uses=0]
@Class = external global i16                      ; <i16*> [#uses=0]
@CWeights = external global i64*                  ; <i64**> [#uses=0]
@ug = external global i16                         ; <i16*> [#uses=0]
@cg = external global i16                         ; <i16*> [#uses=0]
@g = external global i64*                         ; <i64**> [#uses=0]
@ce = external global i64                         ; <i64*> [#uses=0]
@Commutators = external global %struct.TypHeader** ; <%struct.TypHeader***> [#uses=0]
@GenStk = external global i16*                    ; <i16**> [#uses=0]
@ExpStk = external global i64*                    ; <i64**> [#uses=0]
@StrStk = external global i16**                   ; <i16***> [#uses=0]
@Sp = external global i64                         ; <i64*> [#uses=0]
@StkDim = external global i64                     ; <i64*> [#uses=0]
@ue = external global i64                         ; <i64*> [#uses=0]
@LastClass = external global i16                  ; <i16*> [#uses=0]
@Powers = external global %struct.TypHeader**     ; <%struct.TypHeader***> [#uses=0]
@NrGens = external global i64                     ; <i64*> [#uses=0]
@Prime = external global i64                      ; <i64*> [#uses=0]
@HdRnAvec = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@.str = external constant [7 x i8], align 1       ; <[7 x i8]*> [#uses=0]
@.str1 = external constant [11 x i8], align 1     ; <[11 x i8]*> [#uses=0]
@.str2 = external constant [9 x i8], align 1      ; <[9 x i8]*> [#uses=0]
@.str3 = external constant [6 x i8], align 1      ; <[6 x i8]*> [#uses=0]
@.str4 = external constant [7 x i8], align 1      ; <[7 x i8]*> [#uses=0]
@.str5 = external constant [8 x i8], align 1      ; <[8 x i8]*> [#uses=0]
@.str6 = external constant [12 x i8], align 1     ; <[12 x i8]*> [#uses=0]
@.str7 = external constant [10 x i8], align 1     ; <[10 x i8]*> [#uses=0]
@.str8 = external constant [17 x i8], align 1     ; <[17 x i8]*> [#uses=0]
@.str9 = external constant [14 x i8], align 1     ; <[14 x i8]*> [#uses=0]
@.str10 = external constant [17 x i8], align 1    ; <[17 x i8]*> [#uses=0]
@.str11 = external constant [62 x i8], align 8    ; <[62 x i8]*> [#uses=0]
@.str12 = external constant [7 x i8], align 1     ; <[7 x i8]*> [#uses=0]
@HdCallOop2 = external global %struct.TypHeader*  ; <%struct.TypHeader**> [#uses=0]
@HdCallOop1 = external global %struct.TypHeader*  ; <%struct.TypHeader**> [#uses=0]
@.str13 = external constant [43 x i8], align 8    ; <[43 x i8]*> [#uses=0]
@.str14 = external constant [40 x i8], align 8    ; <[40 x i8]*> [#uses=0]
@.str15 = external constant [43 x i8], align 8    ; <[43 x i8]*> [#uses=0]
@.str16 = external constant [42 x i8], align 8    ; <[42 x i8]*> [#uses=0]
@.str17 = external constant [40 x i8], align 8    ; <[40 x i8]*> [#uses=0]
@.str18 = external constant [41 x i8], align 8    ; <[41 x i8]*> [#uses=0]
@.str19 = external constant [41 x i8], align 8    ; <[41 x i8]*> [#uses=0]
@.str20 = external constant [35 x i8], align 8    ; <[35 x i8]*> [#uses=0]
@.str21 = external constant [35 x i8], align 8    ; <[35 x i8]*> [#uses=0]
@.str22 = external constant [28 x i8], align 1    ; <[28 x i8]*> [#uses=0]
@.str23 = external constant [21 x i8], align 1    ; <[21 x i8]*> [#uses=0]
@.str24 = external constant [22 x i8], align 1    ; <[22 x i8]*> [#uses=0]
@.str25 = external constant [26 x i8], align 1    ; <[26 x i8]*> [#uses=0]
@.str26 = external constant [18 x i8], align 1    ; <[18 x i8]*> [#uses=0]
@.str27 = external constant [11 x i8], align 1    ; <[11 x i8]*> [#uses=0]
@.str28 = external constant [31 x i8], align 8    ; <[31 x i8]*> [#uses=0]
@.str29 = external constant [35 x i8], align 8    ; <[35 x i8]*> [#uses=0]
@.str30 = external constant [29 x i8], align 1    ; <[29 x i8]*> [#uses=0]
@.str31 = external constant [27 x i8], align 1    ; <[27 x i8]*> [#uses=0]
@.str32 = external constant [14 x i8], align 1    ; <[14 x i8]*> [#uses=0]
@.str33 = external constant [15 x i8], align 1    ; <[15 x i8]*> [#uses=0]
@.str34 = external constant [43 x i8], align 8    ; <[43 x i8]*> [#uses=0]
@.str35 = external constant [40 x i8], align 8    ; <[40 x i8]*> [#uses=0]
@.str36 = external constant [10 x i8], align 1    ; <[10 x i8]*> [#uses=0]
@.str37 = external constant [49 x i8], align 8    ; <[49 x i8]*> [#uses=0]
@.str38 = external constant [50 x i8], align 8    ; <[50 x i8]*> [#uses=0]
@.str39 = external constant [42 x i8], align 8    ; <[42 x i8]*> [#uses=0]
@.str40 = external constant [45 x i8], align 8    ; <[45 x i8]*> [#uses=0]
@.str41 = external constant [32 x i8], align 8    ; <[32 x i8]*> [#uses=0]
@.str42 = external constant [48 x i8], align 8    ; <[48 x i8]*> [#uses=0]
@.str43 = external constant [11 x i8], align 1    ; <[11 x i8]*> [#uses=0]
@.str44 = external constant [47 x i8], align 8    ; <[47 x i8]*> [#uses=0]
@.str45 = external constant [49 x i8], align 8    ; <[49 x i8]*> [#uses=0]
@.str46 = external constant [53 x i8], align 8    ; <[53 x i8]*> [#uses=0]
@.str47 = external constant [9 x i8], align 1     ; <[9 x i8]*> [#uses=0]
@.str48 = external constant [10 x i8], align 1    ; <[10 x i8]*> [#uses=0]
@.str49 = external constant [33 x i8], align 8    ; <[33 x i8]*> [#uses=0]
@.str50 = external constant [38 x i8], align 8    ; <[38 x i8]*> [#uses=0]
@.str51 = external constant [36 x i8], align 8    ; <[36 x i8]*> [#uses=0]
@.str52 = external constant [7 x i8], align 1     ; <[7 x i8]*> [#uses=0]
@.str53 = external constant [7 x i8], align 1     ; <[7 x i8]*> [#uses=0]
@.str54 = external constant [10 x i8], align 1    ; <[10 x i8]*> [#uses=0]
@.str55 = external constant [11 x i8], align 1    ; <[11 x i8]*> [#uses=0]
@.str56 = external constant [14 x i8], align 1    ; <[14 x i8]*> [#uses=0]
@.str57 = external constant [51 x i8], align 8    ; <[51 x i8]*> [#uses=0]
@.str58 = external constant [60 x i8], align 8    ; <[60 x i8]*> [#uses=0]
@.str59 = external constant [47 x i8], align 8    ; <[47 x i8]*> [#uses=0]
@.str60 = external constant [11 x i8], align 1    ; <[11 x i8]*> [#uses=0]
@.str61 = external constant [11 x i8], align 1    ; <[11 x i8]*> [#uses=0]
@.str62 = external constant [48 x i8], align 8    ; <[48 x i8]*> [#uses=0]
@.str63 = external constant [57 x i8], align 8    ; <[57 x i8]*> [#uses=0]
@.str64 = external constant [8 x i8], align 1     ; <[8 x i8]*> [#uses=0]
@.str65 = external constant [10 x i8], align 1    ; <[10 x i8]*> [#uses=0]
@.str166 = external constant [10 x i8], align 1   ; <[10 x i8]*> [#uses=0]
@HdRnSumAgWord = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str267 = external constant [17 x i8], align 1   ; <[17 x i8]*> [#uses=0]
@HdRnDifferenceAgWord = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str368 = external constant [6 x i8], align 1    ; <[6 x i8]*> [#uses=0]
@HdRnDepth = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@.str469 = external constant [10 x i8], align 1   ; <[10 x i8]*> [#uses=0]
@HdRnTailDepth = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str570 = external constant [14 x i8], align 1   ; <[14 x i8]*> [#uses=0]
@HdRnCentralWeight = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str671 = external constant [16 x i8], align 1   ; <[16 x i8]*> [#uses=0]
@HdRnLeadingExponent = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str772 = external constant [14 x i8], align 1   ; <[14 x i8]*> [#uses=0]
@HdRnReducedAgWord = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str873 = external constant [14 x i8], align 1   ; <[14 x i8]*> [#uses=0]
@HdRnRelativeOrder = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str974 = external constant [15 x i8], align 1   ; <[15 x i8]*> [#uses=0]
@HdRnExponentAgWord = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str1075 = external constant [16 x i8], align 1  ; <[16 x i8]*> [#uses=0]
@HdRnExponentsAgWord = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str1176 = external constant [18 x i8], align 1  ; <[18 x i8]*> [#uses=0]
@HdRnInformationAgWord = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str1277 = external constant [19 x i8], align 1  ; <[19 x i8]*> [#uses=0]
@HdRnIsCompatibleAgWord = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str1378 = external constant [13 x i8], align 1  ; <[13 x i8]*> [#uses=0]
@HdRnNormalizeIgs = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str1479 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@HdRnIsAgWord = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str1580 = external constant [5 x i8], align 1   ; <[5 x i8]*> [#uses=0]
@.str1681 = external constant [12 x i8], align 1  ; <[12 x i8]*> [#uses=0]
@.str1782 = external constant [16 x i8], align 1  ; <[16 x i8]*> [#uses=0]
@.str1883 = external constant [20 x i8], align 1  ; <[20 x i8]*> [#uses=0]
@.str1984 = external constant [22 x i8], align 1  ; <[22 x i8]*> [#uses=0]
@.str2085 = external constant [20 x i8], align 1  ; <[20 x i8]*> [#uses=0]
@.str2186 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str2287 = external constant [13 x i8], align 1  ; <[13 x i8]*> [#uses=0]
@.str2388 = external constant [14 x i8], align 1  ; <[14 x i8]*> [#uses=0]
@.str2489 = external constant [14 x i8], align 1  ; <[14 x i8]*> [#uses=0]
@.str2590 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str2691 = external constant [17 x i8], align 1  ; <[17 x i8]*> [#uses=0]
@HdCPL = external global %struct.TypHeader*       ; <%struct.TypHeader**> [#uses=0]
@HdCPC = external global %struct.TypHeader*       ; <%struct.TypHeader**> [#uses=0]
@HdCPS = external global %struct.TypHeader*       ; <%struct.TypHeader**> [#uses=0]
@.str2794 = external constant [19 x i8], align 1  ; <[19 x i8]*> [#uses=0]
@.str2895 = external constant [3 x i8], align 1   ; <[3 x i8]*> [#uses=0]
@.str2996 = external constant [14 x i8], align 1  ; <[14 x i8]*> [#uses=0]
@.str3097 = external constant [13 x i8], align 1  ; <[13 x i8]*> [#uses=0]
@.str3198 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@.str3299 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@.str33100 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str34101 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str35102 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str36103 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str37104 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str38105 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str39106 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str40107 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str41108 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@RepTimes = external global i64                   ; <i64*> [#uses=0]
@.str42109 = external constant [45 x i8], align 8 ; <[45 x i8]*> [#uses=0]
@.str43110 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str44111 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str45112 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@CallsProdAg = external global i64                ; <i64*> [#uses=0]
@TimeProdAg = external global i64                 ; <i64*> [#uses=0]
@.str46113 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@.str47114 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@CallsQuoAg = external global i64                 ; <i64*> [#uses=0]
@TimeQuoAg = external global i64                  ; <i64*> [#uses=0]
@.str48115 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@CallsPowAgI = external global i64                ; <i64*> [#uses=0]
@TimePowAgI = external global i64                 ; <i64*> [#uses=0]
@.str49116 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@CallsPowAgAg = external global i64               ; <i64*> [#uses=0]
@TimePowAgAg = external global i64                ; <i64*> [#uses=0]
@.str50117 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@CallsModAg = external global i64                 ; <i64*> [#uses=0]
@TimeModAg = external global i64                  ; <i64*> [#uses=0]
@.str51118 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@CallsCommAg = external global i64                ; <i64*> [#uses=0]
@TimeCommAg = external global i64                 ; <i64*> [#uses=0]
@.str52119 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@CallsLtAg = external global i64                  ; <i64*> [#uses=0]
@TimeLtAg = external global i64                   ; <i64*> [#uses=0]
@.str53120 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@CallsEqAg = external global i64                  ; <i64*> [#uses=0]
@TimeEqAg = external global i64                   ; <i64*> [#uses=0]
@.str54121 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@CallsSumAg = external global i64                 ; <i64*> [#uses=0]
@TimeSumAg = external global i64                  ; <i64*> [#uses=0]
@.str55122 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@CallsDiffAg = external global i64                ; <i64*> [#uses=0]
@TimeDiffAg = external global i64                 ; <i64*> [#uses=0]
@.str56123 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@.str57124 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str58125 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str59126 = external constant [25 x i8], align 1 ; <[25 x i8]*> [#uses=0]
@.str60127 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str61128 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str62129 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str63130 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str64131 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str65132 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str66 = external constant [31 x i8], align 8    ; <[31 x i8]*> [#uses=0]
@.str67 = external constant [37 x i8], align 8    ; <[37 x i8]*> [#uses=0]
@.str68 = external constant [37 x i8], align 8    ; <[37 x i8]*> [#uses=0]
@.str69 = external constant [12 x i8], align 1    ; <[12 x i8]*> [#uses=0]
@.str70 = external constant [12 x i8], align 1    ; <[12 x i8]*> [#uses=0]
@CPP.b = external global i1                       ; <i1*> [#uses=0]
@.str71 = external constant [1 x i8], align 1     ; <[1 x i8]*> [#uses=0]
@.str72 = external constant [3 x i8], align 1     ; <[3 x i8]*> [#uses=0]
@CPN = external global i64                        ; <i64*> [#uses=0]
@.str73 = external constant [39 x i8], align 8    ; <[39 x i8]*> [#uses=0]
@.str74 = external constant [24 x i8], align 1    ; <[24 x i8]*> [#uses=0]
@.str75 = external constant [23 x i8], align 1    ; <[23 x i8]*> [#uses=0]
@.str76 = external constant [39 x i8], align 8    ; <[39 x i8]*> [#uses=0]
@.str77 = external constant [30 x i8], align 1    ; <[30 x i8]*> [#uses=0]
@.str78 = external constant [35 x i8], align 8    ; <[35 x i8]*> [#uses=0]
@.str79 = external constant [40 x i8], align 8    ; <[40 x i8]*> [#uses=0]
@.str80 = external constant [33 x i8], align 8    ; <[33 x i8]*> [#uses=0]
@.str81 = external constant [47 x i8], align 8    ; <[47 x i8]*> [#uses=0]
@.str82 = external constant [38 x i8], align 8    ; <[38 x i8]*> [#uses=0]
@.str83 = external constant [45 x i8], align 8    ; <[45 x i8]*> [#uses=0]
@.str84 = external constant [36 x i8], align 8    ; <[36 x i8]*> [#uses=0]
@.str87 = external constant [32 x i8], align 8    ; <[32 x i8]*> [#uses=0]
@.str89 = external constant [6 x i8], align 1     ; <[6 x i8]*> [#uses=0]
@.str91 = external constant [36 x i8], align 8    ; <[36 x i8]*> [#uses=0]
@.str93 = external constant [34 x i8], align 8    ; <[34 x i8]*> [#uses=0]
@.str94 = external constant [28 x i8], align 1    ; <[28 x i8]*> [#uses=0]
@.str95 = external constant [36 x i8], align 8    ; <[36 x i8]*> [#uses=0]
@.str96 = external constant [30 x i8], align 1    ; <[30 x i8]*> [#uses=0]
@.str97 = external constant [34 x i8], align 8    ; <[34 x i8]*> [#uses=0]
@.str98 = external constant [46 x i8], align 8    ; <[46 x i8]*> [#uses=0]
@.str99 = external constant [30 x i8], align 1    ; <[30 x i8]*> [#uses=0]
@.str100 = external constant [26 x i8], align 1   ; <[26 x i8]*> [#uses=0]
@.str101 = external constant [45 x i8], align 8   ; <[45 x i8]*> [#uses=0]
@.str102 = external constant [50 x i8], align 8   ; <[50 x i8]*> [#uses=0]
@.str103 = external constant [46 x i8], align 8   ; <[46 x i8]*> [#uses=0]
@.str104 = external constant [29 x i8], align 1   ; <[29 x i8]*> [#uses=0]
@.str105 = external constant [33 x i8], align 8   ; <[33 x i8]*> [#uses=0]
@.str106 = external constant [30 x i8], align 1   ; <[30 x i8]*> [#uses=0]
@.str107 = external constant [36 x i8], align 8   ; <[36 x i8]*> [#uses=0]
@.str108 = external constant [44 x i8], align 8   ; <[44 x i8]*> [#uses=0]
@.str109 = external constant [29 x i8], align 1   ; <[29 x i8]*> [#uses=0]
@.str110 = external constant [34 x i8], align 8   ; <[34 x i8]*> [#uses=0]
@.str111 = external constant [55 x i8], align 8   ; <[55 x i8]*> [#uses=0]
@.str113 = external constant [33 x i8], align 8   ; <[33 x i8]*> [#uses=0]
@.str114 = external constant [43 x i8], align 8   ; <[43 x i8]*> [#uses=0]
@.str115 = external constant [41 x i8], align 8   ; <[41 x i8]*> [#uses=0]
@.str116 = external constant [23 x i8], align 1   ; <[23 x i8]*> [#uses=0]
@.str117 = external constant [12 x i8], align 1   ; <[12 x i8]*> [#uses=0]
@.str118 = external constant [12 x i8], align 1   ; <[12 x i8]*> [#uses=0]
@.str119 = external constant [22 x i8], align 1   ; <[22 x i8]*> [#uses=0]
@.str120 = external constant [29 x i8], align 1   ; <[29 x i8]*> [#uses=0]
@.str121 = external constant [35 x i8], align 8   ; <[35 x i8]*> [#uses=0]
@.str122 = external constant [40 x i8], align 8   ; <[40 x i8]*> [#uses=0]
@.str123 = external constant [29 x i8], align 1   ; <[29 x i8]*> [#uses=0]
@.str124 = external constant [33 x i8], align 8   ; <[33 x i8]*> [#uses=0]
@.str125 = external constant [41 x i8], align 8   ; <[41 x i8]*> [#uses=0]
@.str137 = external constant [8 x i8], align 1    ; <[8 x i8]*> [#uses=0]
@.str1138 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str2139 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str3140 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str4141 = external constant [14 x i8], align 1  ; <[14 x i8]*> [#uses=0]
@.str5142 = external constant [15 x i8], align 1  ; <[15 x i8]*> [#uses=0]
@.str6143 = external constant [11 x i8], align 1  ; <[11 x i8]*> [#uses=0]
@.str7144 = external constant [14 x i8], align 1  ; <[14 x i8]*> [#uses=0]
@.str8145 = external constant [14 x i8], align 1  ; <[14 x i8]*> [#uses=0]
@.str9146 = external constant [43 x i8], align 8  ; <[43 x i8]*> [#uses=0]
@.str10147 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str11148 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str12149 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str13150 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str14151 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str15152 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str16153 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str17154 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str18155 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str19156 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str20157 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str21158 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str22159 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str23160 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str24161 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str25162 = external constant [45 x i8], align 8 ; <[45 x i8]*> [#uses=0]
@.str26163 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str27164 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str28165 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str29166 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str30167 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str31168 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str32169 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str33170 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str34171 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str35172 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str36173 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str37174 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str38175 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str39176 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str40177 = external constant [24 x i8], align 1 ; <[24 x i8]*> [#uses=0]
@.str41178 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str179 = external constant [15 x i8], align 1   ; <[15 x i8]*> [#uses=0]
@.str1180 = external constant [35 x i8], align 8  ; <[35 x i8]*> [#uses=0]
@.str2181 = external constant [34 x i8], align 8  ; <[34 x i8]*> [#uses=0]
@.str3182 = external constant [39 x i8], align 8  ; <[39 x i8]*> [#uses=0]
@.str4183 = external constant [19 x i8], align 1  ; <[19 x i8]*> [#uses=0]
@.str5184 = external constant [37 x i8], align 8  ; <[37 x i8]*> [#uses=0]
@.str6185 = external constant [20 x i8], align 1  ; <[20 x i8]*> [#uses=0]
@.str7186 = external constant [37 x i8], align 8  ; <[37 x i8]*> [#uses=0]
@.str8187 = external constant [44 x i8], align 8  ; <[44 x i8]*> [#uses=0]
@.str9188 = external constant [45 x i8], align 8  ; <[45 x i8]*> [#uses=0]
@.str10189 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str11190 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str12191 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@.str13192 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str14193 = external constant [25 x i8], align 1 ; <[25 x i8]*> [#uses=0]
@.str15194 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str16195 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str17196 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str18197 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str19198 = external constant [51 x i8], align 8 ; <[51 x i8]*> [#uses=0]
@.str20199 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str21200 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str22201 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str23202 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str24203 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str25204 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str26205 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str27206 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str28207 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@hdTable = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@treeType = external global i64                   ; <i64*> [#uses=0]
@hdWordValue = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@hdTree2 = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@treeWordLength = external global i64             ; <i64*> [#uses=0]
@wordList = external global [1024 x i64], align 32 ; <[1024 x i64]*> [#uses=0]
@hdTabl2 = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@.str208 = external constant [9 x i8], align 1    ; <[9 x i8]*> [#uses=0]
@.str1209 = external constant [17 x i8], align 1  ; <[17 x i8]*> [#uses=0]
@.str2210 = external constant [17 x i8], align 1  ; <[17 x i8]*> [#uses=0]
@.str3211 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str4212 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=0]
@.str5213 = external constant [14 x i8], align 1  ; <[14 x i8]*> [#uses=0]
@.str6214 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str7215 = external constant [18 x i8], align 1  ; <[18 x i8]*> [#uses=0]
@.str8216 = external constant [18 x i8], align 1  ; <[18 x i8]*> [#uses=0]
@.str9217 = external constant [18 x i8], align 1  ; <[18 x i8]*> [#uses=0]
@.str10218 = external constant [45 x i8], align 8 ; <[45 x i8]*> [#uses=0]
@.str11219 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@.str12220 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str13221 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str14222 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str15223 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str16224 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@hdExponent = external global %struct.TypHeader*  ; <%struct.TypHeader**> [#uses=0]
@hdTree1 = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@hdTree = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@.str17225 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str18226 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str19227 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str20228 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str21229 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str22230 = external constant [27 x i8], align 1 ; <[27 x i8]*> [#uses=0]
@.str23231 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str24232 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str25233 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str26234 = external constant [18 x i8], align 1 ; <[18 x i8]*> [#uses=0]
@.str27235 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str28236 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str29237 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@hdRel = external global %struct.TypHeader*       ; <%struct.TypHeader**> [#uses=0]
@.str30238 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@hdNums = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@.str31239 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str32240 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str33241 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str34242 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@dedlst = external global i64                     ; <i64*> [#uses=0]
@.str35243 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@dedfst = external global i64                     ; <i64*> [#uses=0]
@dedgen = external global [40960 x i64], align 32 ; <[40960 x i64]*> [#uses=0]
@dedcos = external global [40960 x i64], align 32 ; <[40960 x i64]*> [#uses=0]
@dedprint.b = external global i1                  ; <i1*> [#uses=0]
@.str36244 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@hdNext = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@hdPrev = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@lastDef = external global i64                    ; <i64*> [#uses=0]
@firstDef = external global i64                   ; <i64*> [#uses=0]
@hdFact = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@firstFree = external global i64                  ; <i64*> [#uses=0]
@lastFree = external global i64                   ; <i64*> [#uses=0]
@nrdel = external global i64                      ; <i64*> [#uses=0]
@.str37245 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str38246 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@lastN.4480 = external global i64                 ; <i64*> [#uses=0]
@phi.4481 = external global i64                   ; <i64*> [#uses=0]
@isSqfree.4482.b = external global i1             ; <i1*> [#uses=0]
@nrp.4483 = external global i64                   ; <i64*> [#uses=0]
@HdResult = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@CycLastE = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@.str247 = external constant [2 x i8], align 1    ; <[2 x i8]*> [#uses=0]
@.str1248 = external constant [6 x i8], align 1   ; <[6 x i8]*> [#uses=0]
@.str2249 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@.str3250 = external constant [7 x i8], align 1   ; <[7 x i8]*> [#uses=0]
@.str4251 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=1]
@.str5252 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str6253 = external constant [33 x i8], align 8  ; <[33 x i8]*> [#uses=0]
@.str7254 = external constant [36 x i8], align 8  ; <[36 x i8]*> [#uses=0]
@.str8255 = external constant [38 x i8], align 8  ; <[38 x i8]*> [#uses=0]
@.str9256 = external constant [26 x i8], align 1  ; <[26 x i8]*> [#uses=0]
@.str10257 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str11258 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str12259 = external constant [45 x i8], align 8 ; <[45 x i8]*> [#uses=0]
@.str13260 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str14261 = external constant [25 x i8], align 1 ; <[25 x i8]*> [#uses=0]
@.str15262 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str16263 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@.str17264 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str18265 = external constant [14 x i8], align 1 ; <[14 x i8]*> [#uses=0]
@.str19266 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@CycLastN = external global i64                   ; <i64*> [#uses=0]
@.str20267 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str21268 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str22269 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str23270 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str24271 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str25272 = external constant [17 x i8], align 1 ; <[17 x i8]*> [#uses=0]
@.str26273 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str27274 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str28275 = external constant [19 x i8], align 1 ; <[19 x i8]*> [#uses=0]
@.str29276 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str30277 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str31278 = external constant [17 x i8], align 1 ; <[17 x i8]*> [#uses=0]
@EvTab = external global [81 x %struct.TypHeader* (%struct.TypHeader*)*], align 32 ; <[81 x %struct.TypHeader* (%struct.TypHeader*)*]*> [#uses=0]
@TabSum = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@TabDiff = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@TabProd = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@TabQuo = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@TabMod = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@TabPow = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@HdTrue = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@HdFalse = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@TabEq = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@TabLt = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@PrTab = external global [81 x void (%struct.TypHeader*)*], align 32 ; <[81 x void (%struct.TypHeader*)*]*> [#uses=0]
@.str292 = external constant [45 x i8], align 8   ; <[45 x i8]*> [#uses=0]
@TabComm = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@.str1294 = external constant [5 x i8], align 1   ; <[5 x i8]*> [#uses=0]
@.str2295 = external constant [13 x i8], align 1  ; <[13 x i8]*> [#uses=0]
@HdVoid = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@.str3297 = external constant [5 x i8], align 1   ; <[5 x i8]*> [#uses=0]
@.str4298 = external constant [6 x i8], align 1   ; <[6 x i8]*> [#uses=0]
@.str5299 = external constant [7 x i8], align 1   ; <[7 x i8]*> [#uses=0]
@.str6300 = external constant [12 x i8], align 1  ; <[12 x i8]*> [#uses=0]
@.str7301 = external constant [5 x i8], align 1   ; <[5 x i8]*> [#uses=0]
@.str8302 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=1]
@.str9303 = external constant [7 x i8], align 1   ; <[7 x i8]*> [#uses=0]
@.str10304 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@HdTildePr = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@.str11305 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str12306 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str13307 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str14308 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str15309 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str16310 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str17311 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str18312 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str19313 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str20314 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str21315 = external constant [51 x i8], align 8 ; <[51 x i8]*> [#uses=0]
@.str22316 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str23317 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str24318 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str25319 = external constant [49 x i8], align 8 ; <[49 x i8]*> [#uses=0]
@.str26320 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str27321 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str28322 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str29323 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str30324 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str31325 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str32326 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str33327 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str34328 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str35329 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str36330 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str37331 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str38332 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str40334 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str41335 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str42336 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str43337 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str44338 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str45339 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str46340 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str47341 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str48342 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str49343 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str50344 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str51345 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str52346 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str53347 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str54348 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str55349 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str56350 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str57351 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str58352 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str59353 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str60354 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@prPrec = external global i64                     ; <i64*> [#uses=0]
@.str61355 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str62356 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str63357 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str64358 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str65359 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str66360 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str67361 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str68362 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str69363 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str70364 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str71365 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str72366 = external constant [17 x i8], align 1 ; <[17 x i8]*> [#uses=0]
@.str73367 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str74368 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str75369 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str76370 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str77371 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str78372 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str79373 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str81375 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str82376 = external constant [45 x i8], align 8 ; <[45 x i8]*> [#uses=0]
@.str83377 = external constant [24 x i8], align 1 ; <[24 x i8]*> [#uses=0]
@.str84378 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str85379 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str86380 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str87381 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str88382 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str89383 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str90384 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str91385 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str92386 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str93387 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str94388 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@HdReturn = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@Pols = external constant [186 x i64], align 32   ; <[186 x i64]*> [#uses=0]
@HdFields = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@HdIntFFEs = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@.str396 = external constant [6 x i8], align 1    ; <[6 x i8]*> [#uses=0]
@.str1397 = external constant [7 x i8], align 1   ; <[7 x i8]*> [#uses=0]
@.str2398 = external constant [7 x i8], align 1   ; <[7 x i8]*> [#uses=0]
@.str3399 = external constant [2 x i8], align 1   ; <[2 x i8]*> [#uses=0]
@.str4400 = external constant [22 x i8], align 1  ; <[22 x i8]*> [#uses=0]
@.str5401 = external constant [36 x i8], align 8  ; <[36 x i8]*> [#uses=0]
@.str6402 = external constant [24 x i8], align 1  ; <[24 x i8]*> [#uses=0]
@.str7403 = external constant [15 x i8], align 1  ; <[15 x i8]*> [#uses=0]
@.str8404 = external constant [11 x i8], align 1  ; <[11 x i8]*> [#uses=0]
@.str9405 = external constant [4 x i8], align 1   ; <[4 x i8]*> [#uses=0]
@.str10406 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str11407 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str12408 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str13409 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@HdLastIntFFE = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str14410 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@.str15411 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str16412 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str17413 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str18414 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str19415 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str20416 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str21417 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str22418 = external constant [51 x i8], align 8 ; <[51 x i8]*> [#uses=0]
@.str23419 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str24420 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str25421 = external constant [59 x i8], align 8 ; <[59 x i8]*> [#uses=0]
@.str26422 = external constant [52 x i8], align 8 ; <[52 x i8]*> [#uses=0]
@.str27423 = external constant [59 x i8], align 8 ; <[59 x i8]*> [#uses=0]
@.str28424 = external constant [52 x i8], align 8 ; <[52 x i8]*> [#uses=0]
@.str29425 = external constant [59 x i8], align 8 ; <[59 x i8]*> [#uses=0]
@.str30426 = external constant [52 x i8], align 8 ; <[52 x i8]*> [#uses=0]
@.str31427 = external constant [59 x i8], align 8 ; <[59 x i8]*> [#uses=0]
@.str32428 = external constant [52 x i8], align 8 ; <[52 x i8]*> [#uses=0]
@HdExec = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@.str431 = external constant [7 x i8], align 1    ; <[7 x i8]*> [#uses=0]
@.str1432 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str2433 = external constant [12 x i8], align 1  ; <[12 x i8]*> [#uses=0]
@.str3434 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@HdTimes = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@.str4435 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=0]
@.str5436 = external constant [47 x i8], align 8  ; <[47 x i8]*> [#uses=0]
@.str8439 = external constant [15 x i8], align 1  ; <[15 x i8]*> [#uses=0]
@.str11442 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str12443 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str13444 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str14445 = external constant [19 x i8], align 1 ; <[19 x i8]*> [#uses=0]
@.str15446 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@prFull.b = external global i1                    ; <i1*> [#uses=0]
@.str16447 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str18449 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str19450 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str20451 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str22453 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str23454 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str24455 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str25456 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str26457 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str27458 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str28459 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str29460 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@IsProfiling = external global i64                ; <i64*> [#uses=0]
@.str30461 = external constant [49 x i8], align 8 ; <[49 x i8]*> [#uses=0]
@.str31462 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str32463 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str33464 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@Timesum = external global i64                    ; <i64*> [#uses=0]
@.str34465 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@.str35466 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str36467 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str37468 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str39470 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str40471 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str42473 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str43474 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str477 = external constant [9 x i8], align 1    ; <[9 x i8]*> [#uses=0]
@.str1478 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@.str2479 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=0]
@.str3480 = external constant [7 x i8], align 1   ; <[7 x i8]*> [#uses=0]
@.str4481 = external constant [5 x i8], align 1   ; <[5 x i8]*> [#uses=0]
@.str5482 = external constant [5 x i8], align 1   ; <[5 x i8]*> [#uses=0]
@.str7484 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=0]
@.str8485 = external constant [6 x i8], align 1   ; <[6 x i8]*> [#uses=0]
@HdLast = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@ErrRet = external global [1 x %struct.__jmp_buf_tag], align 32 ; <[1 x %struct.__jmp_buf_tag]*> [#uses=0]
@.str9486 = external constant [5 x i8], align 1   ; <[5 x i8]*> [#uses=0]
@.str10487 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@HdLast2 = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@.str11488 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@HdLast3 = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@.str12489 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@HdTime = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@.str13490 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str14491 = external constant [18 x i8], align 1 ; <[18 x i8]*> [#uses=0]
@.str15492 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str16493 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str17494 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str18495 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str19496 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str20497 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str21498 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str22499 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str23500 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str24501 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str25502 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str26503 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str27504 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str28505 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str29506 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str30507 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str31508 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str32509 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str33510 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str34511 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str35512 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str36513 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str37514 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str38515 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str39516 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str40517 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str41518 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str42519 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str43520 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str44521 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str45522 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str46523 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str47524 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@.str48525 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str49526 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str50527 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str51528 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str52529 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str53530 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str54531 = external constant [52 x i8], align 8 ; <[52 x i8]*> [#uses=0]
@.str55532 = external constant [55 x i8], align 8 ; <[55 x i8]*> [#uses=0]
@.str56533 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str57534 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str58535 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str59536 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str60537 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str61538 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str62539 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str63540 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str64541 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@.str65542 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@.str66543 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str67544 = external constant [19 x i8], align 1 ; <[19 x i8]*> [#uses=0]
@.str68545 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str69546 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str70547 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str71548 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str72549 = external constant [17 x i8], align 1 ; <[17 x i8]*> [#uses=0]
@.str73550 = external constant [45 x i8], align 8 ; <[45 x i8]*> [#uses=0]
@.str74551 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str75552 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str76553 = external constant [17 x i8], align 1 ; <[17 x i8]*> [#uses=0]
@.str77554 = external constant [25 x i8], align 1 ; <[25 x i8]*> [#uses=0]
@.str78555 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str79556 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str80557 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str81558 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str82559 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str83560 = external constant [54 x i8], align 8 ; <[54 x i8]*> [#uses=0]
@.str84561 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str85562 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str86563 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str87564 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str88565 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str89566 = external constant [25 x i8], align 1 ; <[25 x i8]*> [#uses=0]
@.str90567 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str91568 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str93570 = external constant [55 x i8], align 8 ; <[55 x i8]*> [#uses=0]
@.str94571 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str95572 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str96573 = external constant [54 x i8], align 8 ; <[54 x i8]*> [#uses=0]
@.str97574 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str98575 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str99576 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str100577 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str101578 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str102579 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str103580 = external constant [27 x i8], align 1 ; <[27 x i8]*> [#uses=0]
@.str104581 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str105582 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str106583 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str107584 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str108585 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str109586 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str110587 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str111588 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str112589 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str113590 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str114591 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str115592 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str116593 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str117594 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str118595 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str119596 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str120597 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str121598 = external constant [14 x i8], align 1 ; <[14 x i8]*> [#uses=0]
@.str122599 = external constant [7 x i8], align 1 ; <[7 x i8]*> [#uses=0]
@.str124601 = external constant [19 x i8], align 1 ; <[19 x i8]*> [#uses=0]
@.str125602 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str126 = external constant [5 x i8], align 1    ; <[5 x i8]*> [#uses=0]
@.str127 = external constant [6 x i8], align 1    ; <[6 x i8]*> [#uses=0]
@.str128 = external constant [39 x i8], align 8   ; <[39 x i8]*> [#uses=0]
@NameType = external constant [81 x i8*], align 32 ; <[81 x i8*]*> [#uses=0]
@Size = external global [81 x %struct.anon], align 32 ; <[81 x %struct.anon]*> [#uses=0]
@HdNewHandles = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@NrNewHandles = external global i64               ; <i64*> [#uses=0]
@s.3831 = external global [7 x i8]                ; <[7 x i8]*> [#uses=0]
@HdFree = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@.str605 = external constant [55 x i8], align 8   ; <[55 x i8]*> [#uses=0]
@FreeHandle = external global %struct.TypHeader*  ; <%struct.TypHeader**> [#uses=0]
@NrFreeHandles = external global i64              ; <i64*> [#uses=0]
@FirstBag = external global %struct.TypHeader**   ; <%struct.TypHeader***> [#uses=0]
@HdResize = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@IsResizeCall.b = external global i1              ; <i1*> [#uses=0]
@.str1606 = external constant [22 x i8], align 1  ; <[22 x i8]*> [#uses=0]
@.str2607 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=0]
@.str3608 = external constant [49 x i8], align 8  ; <[49 x i8]*> [#uses=0]
@.str4609 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=0]
@.str5610 = external constant [11 x i8], align 1  ; <[11 x i8]*> [#uses=0]
@.str6611 = external constant [11 x i8], align 1  ; <[11 x i8]*> [#uses=0]
@.str7612 = external constant [41 x i8], align 8  ; <[41 x i8]*> [#uses=0]
@.str8613 = external constant [11 x i8], align 1  ; <[11 x i8]*> [#uses=0]
@.str9614 = external constant [3 x i8], align 1   ; <[3 x i8]*> [#uses=0]
@.str10615 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str11616 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@lastType = external global i64                   ; <i64*> [#uses=0]
@lastSize = external global i64                   ; <i64*> [#uses=0]
@.str12617 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str13618 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str14619 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str15620 = external constant [17 x i8], align 1 ; <[17 x i8]*> [#uses=0]
@.str16621 = external constant [18 x i8], align 1 ; <[18 x i8]*> [#uses=0]
@.str17622 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str18623 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str19624 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str20625 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@.str21626 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str22627 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str23628 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str24629 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str25630 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str26631 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str28633 = external constant [18 x i8], align 1 ; <[18 x i8]*> [#uses=0]
@.str29634 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str30635 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str31636 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str32637 = external constant [20 x i8], align 1 ; <[20 x i8]*> [#uses=0]
@.str33638 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str34639 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str35640 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str36641 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str37642 = external constant [18 x i8], align 1 ; <[18 x i8]*> [#uses=0]
@.str38643 = external constant [18 x i8], align 1 ; <[18 x i8]*> [#uses=0]
@.str39644 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str40645 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str41646 = external constant [18 x i8], align 1 ; <[18 x i8]*> [#uses=0]
@.str42647 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str43648 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str44649 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str45650 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str46651 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str47652 = external constant [18 x i8], align 1 ; <[18 x i8]*> [#uses=0]
@.str54659 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str65670 = external constant [14 x i8], align 1 ; <[14 x i8]*> [#uses=0]
@.str66671 = external constant [19 x i8], align 1 ; <[19 x i8]*> [#uses=0]
@.str67672 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str68673 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str69674 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str70675 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str71676 = external constant [17 x i8], align 1 ; <[17 x i8]*> [#uses=0]
@.str72677 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str73678 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str74679 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str75680 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str76681 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str77682 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str78683 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str79684 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str80685 = external constant [19 x i8], align 1 ; <[19 x i8]*> [#uses=0]
@.str81686 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str82687 = external constant [24 x i8], align 1 ; <[24 x i8]*> [#uses=0]
@.str83688 = external constant [19 x i8], align 1 ; <[19 x i8]*> [#uses=0]
@.str84689 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@.str85690 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str86691 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str87692 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@GasmanStatAlive = external constant [81 x i64], align 32 ; <[81 x i64]*> [#uses=0]
@GasmanStatTotal = external global [81 x i64], align 32 ; <[81 x i64]*> [#uses=0]
@GasmanStatTSize = external global [81 x i64], align 32 ; <[81 x i64]*> [#uses=0]
@HdStack = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@TopStack = external global i64                   ; <i64*> [#uses=0]
@HdIdenttab = external global %struct.TypHeader*  ; <%struct.TypHeader**> [#uses=1]
@HdRectab = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@NrRectab = external global i64                   ; <i64*> [#uses=0]
@IsUndefinedGlobal.b = external global i1         ; <i1*> [#uses=0]
@NrIdenttab = external global i64                 ; <i64*> [#uses=0]
@.str710 = external constant [6 x i8], align 1    ; <[6 x i8]*> [#uses=0]
@.str1711 = external constant [7 x i8], align 1   ; <[7 x i8]*> [#uses=0]
@.str2712 = external constant [7 x i8], align 1   ; <[7 x i8]*> [#uses=0]
@.str3713 = external constant [7 x i8], align 1   ; <[7 x i8]*> [#uses=0]
@.str4714 = external constant [22 x i8], align 1  ; <[22 x i8]*> [#uses=0]
@.str5715 = external constant [36 x i8], align 8  ; <[36 x i8]*> [#uses=0]
@.str6716 = external constant [30 x i8], align 1  ; <[30 x i8]*> [#uses=0]
@.str7717 = external constant [46 x i8], align 8  ; <[46 x i8]*> [#uses=0]
@.str8718 = external constant [30 x i8], align 1  ; <[30 x i8]*> [#uses=0]
@.str9719 = external constant [30 x i8], align 1  ; <[30 x i8]*> [#uses=0]
@.str10720 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str11721 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str12722 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@PrIntC = external global [1000 x i16], align 32  ; <[1000 x i16]*> [#uses=0]
@PrIntD = external global [1205 x i16], align 32  ; <[1205 x i16]*> [#uses=0]
@.str15725 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str16726 = external constant [5 x i8], align 1  ; <[5 x i8]*> [#uses=0]
@.str18728 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@TabIsList = external global [28 x i64], align 32 ; <[28 x i64]*> [#uses=0]
@TabPlainList = external global [28 x void (%struct.TypHeader*)*], align 32 ; <[28 x void (%struct.TypHeader*)*]*> [#uses=0]
@TabIsXTypeList = external global [28 x i64 (%struct.TypHeader*)*], align 32 ; <[28 x i64 (%struct.TypHeader*)*]*> [#uses=0]
@TabLenList = external global [28 x i64 (%struct.TypHeader*)*], align 32 ; <[28 x i64 (%struct.TypHeader*)*]*> [#uses=0]
@TabElmlList = external global [28 x %struct.TypHeader* (%struct.TypHeader*, i64)*], align 32 ; <[28 x %struct.TypHeader* (%struct.TypHeader*, i64)*]*> [#uses=0]
@TabElmrList = external global [28 x %struct.TypHeader* (%struct.TypHeader*, i64)*], align 32 ; <[28 x %struct.TypHeader* (%struct.TypHeader*, i64)*]*> [#uses=0]
@TabElmList = external global [28 x %struct.TypHeader* (%struct.TypHeader*, i64)*], align 32 ; <[28 x %struct.TypHeader* (%struct.TypHeader*, i64)*]*> [#uses=0]
@TabElmfList = external global [28 x %struct.TypHeader* (%struct.TypHeader*, i64)*], align 32 ; <[28 x %struct.TypHeader* (%struct.TypHeader*, i64)*]*> [#uses=0]
@TabElmsList = external global [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*], align 32 ; <[28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]*> [#uses=0]
@TabAssList = external global [28 x %struct.TypHeader* (%struct.TypHeader*, i64, %struct.TypHeader*)*], align 32 ; <[28 x %struct.TypHeader* (%struct.TypHeader*, i64, %struct.TypHeader*)*]*> [#uses=0]
@TabAsssList = external global [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*)*], align 32 ; <[28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*)*]*> [#uses=0]
@TabPosList = external global [28 x i64 (%struct.TypHeader*, %struct.TypHeader*, i64)*], align 32 ; <[28 x i64 (%struct.TypHeader*, %struct.TypHeader*, i64)*]*> [#uses=0]
@TabIsDenseList = external global [28 x i64 (%struct.TypHeader*)*], align 32 ; <[28 x i64 (%struct.TypHeader*)*]*> [#uses=0]
@TabIsPossList = external global [28 x i64 (%struct.TypHeader*)*], align 32 ; <[28 x i64 (%struct.TypHeader*)*]*> [#uses=0]
@TabDepthVector = external global [28 x %struct.TypHeader* (%struct.TypHeader*)*], align 32 ; <[28 x %struct.TypHeader* (%struct.TypHeader*)*]*> [#uses=0]
@.str748 = external constant [7 x i8], align 1    ; <[7 x i8]*> [#uses=0]
@.str1749 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@.str2750 = external constant [6 x i8], align 1   ; <[6 x i8]*> [#uses=0]
@.str3751 = external constant [7 x i8], align 1   ; <[7 x i8]*> [#uses=0]
@.str4752 = external constant [4 x i8], align 1   ; <[4 x i8]*> [#uses=0]
@.str5753 = external constant [7 x i8], align 1   ; <[7 x i8]*> [#uses=0]
@.str6754 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@.str7755 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@.str8756 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=0]
@.str9757 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@.str10758 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str11759 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str12760 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str13761 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str14762 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str15763 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str16764 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str17765 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str18766 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str19767 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str20768 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str21769 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str22770 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str23771 = external constant [24 x i8], align 1 ; <[24 x i8]*> [#uses=0]
@.str24772 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@.str25773 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str26774 = external constant [25 x i8], align 1 ; <[25 x i8]*> [#uses=0]
@.str27775 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str28776 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str29777 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str30778 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str31779 = external constant [66 x i8], align 8 ; <[66 x i8]*> [#uses=0]
@.str32780 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str33781 = external constant [62 x i8], align 8 ; <[62 x i8]*> [#uses=0]
@.str34782 = external constant [72 x i8], align 8 ; <[72 x i8]*> [#uses=0]
@.str35783 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str36784 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str37785 = external constant [63 x i8], align 8 ; <[63 x i8]*> [#uses=0]
@.str38786 = external constant [65 x i8], align 8 ; <[65 x i8]*> [#uses=0]
@.str39787 = external constant [45 x i8], align 8 ; <[45 x i8]*> [#uses=0]
@.str40788 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str41789 = external constant [61 x i8], align 8 ; <[61 x i8]*> [#uses=0]
@.str42790 = external constant [55 x i8], align 8 ; <[55 x i8]*> [#uses=0]
@.str43791 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str44792 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str45793 = external constant [69 x i8], align 8 ; <[69 x i8]*> [#uses=0]
@.str46794 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str47795 = external constant [52 x i8], align 8 ; <[52 x i8]*> [#uses=0]
@.str49797 = external constant [45 x i8], align 8 ; <[45 x i8]*> [#uses=0]
@.str50798 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str51799 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str52800 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str53801 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str54802 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str55803 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str56804 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str57805 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str58806 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str59807 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str60808 = external constant [27 x i8], align 1 ; <[27 x i8]*> [#uses=0]
@.str61809 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str62810 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str63811 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str64812 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str68816 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str69817 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str70818 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str71819 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str72820 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str73821 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str74822 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str75823 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str76824 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str77825 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str78826 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str832 = external constant [14 x i8], align 1   ; <[14 x i8]*> [#uses=0]
@.str1833 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@.str2834 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=0]
@.str3835 = external constant [13 x i8], align 1  ; <[13 x i8]*> [#uses=0]
@.str4836 = external constant [11 x i8], align 1  ; <[11 x i8]*> [#uses=0]
@.str5837 = external constant [12 x i8], align 1  ; <[12 x i8]*> [#uses=0]
@.str6838 = external constant [16 x i8], align 1  ; <[16 x i8]*> [#uses=0]
@.str7839 = external constant [7 x i8], align 1   ; <[7 x i8]*> [#uses=0]
@.str8840 = external constant [14 x i8], align 1  ; <[14 x i8]*> [#uses=0]
@.str9841 = external constant [12 x i8], align 1  ; <[12 x i8]*> [#uses=0]
@.str10842 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str11843 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str12844 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str13845 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str14846 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str15847 = external constant [14 x i8], align 1 ; <[14 x i8]*> [#uses=0]
@.str16848 = external constant [18 x i8], align 1 ; <[18 x i8]*> [#uses=0]
@.str17849 = external constant [24 x i8], align 1 ; <[24 x i8]*> [#uses=0]
@.str18850 = external constant [14 x i8], align 1 ; <[14 x i8]*> [#uses=0]
@.str19851 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str20852 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str21853 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str22854 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str23855 = external constant [17 x i8], align 1 ; <[17 x i8]*> [#uses=0]
@.str24856 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str25857 = external constant [14 x i8], align 1 ; <[14 x i8]*> [#uses=0]
@.str26858 = external constant [17 x i8], align 1 ; <[17 x i8]*> [#uses=0]
@.str27859 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str28860 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str29861 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str30862 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str31863 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str32864 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str33865 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str34866 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str35867 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str36868 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str37869 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str38870 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str39871 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str40872 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str41873 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str42874 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str43875 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str44876 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str45877 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str46878 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str47879 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str48880 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str49881 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str50882 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str51883 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str52884 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str53885 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str54886 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str55887 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str56888 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str57889 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str58890 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str59891 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str60892 = external constant [20 x i8], align 1 ; <[20 x i8]*> [#uses=0]
@.str61893 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str62894 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str63895 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str64896 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str65897 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str66898 = external constant [27 x i8], align 1 ; <[27 x i8]*> [#uses=0]
@.str67899 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str68900 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str69901 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str70902 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str71903 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str72904 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str73905 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str74906 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str75907 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str76908 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str77909 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str78910 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str79911 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str80912 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str81913 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str82914 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str83915 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@.str84916 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str85917 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str86918 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str87919 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str88920 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str89921 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str92924 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str93925 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str94926 = external constant [20 x i8], align 1 ; <[20 x i8]*> [#uses=0]
@.str95927 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str96928 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str97929 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str98930 = external constant [25 x i8], align 1 ; <[25 x i8]*> [#uses=0]
@.str99931 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str100932 = external constant [19 x i8], align 1 ; <[19 x i8]*> [#uses=0]
@.str934 = external constant [7 x i8], align 1    ; <[7 x i8]*> [#uses=0]
@.str1935 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@.str2936 = external constant [22 x i8], align 1  ; <[22 x i8]*> [#uses=0]
@.str3937 = external constant [19 x i8], align 1  ; <[19 x i8]*> [#uses=0]
@.str4938 = external constant [13 x i8], align 1  ; <[13 x i8]*> [#uses=0]
@.str5939 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str6940 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@.str7941 = external constant [22 x i8], align 1  ; <[22 x i8]*> [#uses=0]
@HdPerm = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@.str8942 = external constant [37 x i8], align 8  ; <[37 x i8]*> [#uses=0]
@.str9943 = external constant [39 x i8], align 8  ; <[39 x i8]*> [#uses=0]
@.str10944 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str11945 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str12946 = external constant [45 x i8], align 8 ; <[45 x i8]*> [#uses=0]
@.str13947 = external constant [49 x i8], align 8 ; <[49 x i8]*> [#uses=0]
@.str14948 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str15949 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str16950 = external constant [55 x i8], align 8 ; <[55 x i8]*> [#uses=0]
@.str17951 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str18952 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str19953 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str20954 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str21955 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str22956 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str23957 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str24958 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str25959 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str26960 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str27961 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str28962 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str29963 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str30964 = external constant [52 x i8], align 8 ; <[52 x i8]*> [#uses=0]
@.str31965 = external constant [27 x i8], align 1 ; <[27 x i8]*> [#uses=0]
@.str32966 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str33967 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str34968 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str35969 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str36970 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str37971 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str38972 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str39973 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str40974 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str41975 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str42976 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str44978 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str45979 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str48982 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str6992 = external constant [35 x i8], align 8  ; <[35 x i8]*> [#uses=0]
@TabNormalizeCoeffs = external global [28 x %struct.TypHeader* (%struct.TypHeader*)*], align 32 ; <[28 x %struct.TypHeader* (%struct.TypHeader*)*]*> [#uses=0]
@TabShrinkCoeffs = external global [28 x void (%struct.TypHeader*)*], align 32 ; <[28 x void (%struct.TypHeader*)*]*> [#uses=0]
@TabShiftedCoeffs = external global [28 x %struct.TypHeader* (%struct.TypHeader*, i64)*], align 32 ; <[28 x %struct.TypHeader* (%struct.TypHeader*, i64)*]*> [#uses=0]
@TabAddCoeffs = external global [28 x [28 x void (%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x void (%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@TabMultiplyCoeffs = external global [28 x [28 x i64 (%struct.TypHeader*, %struct.TypHeader*, i64, %struct.TypHeader*, i64)*]], align 32 ; <[28 x [28 x i64 (%struct.TypHeader*, %struct.TypHeader*, i64, %struct.TypHeader*, i64)*]]*> [#uses=0]
@TabProductCoeffs = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@TabProductCoeffsMod = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@TabReduceCoeffs = external global [28 x [28 x i64 (%struct.TypHeader*, i64, %struct.TypHeader*, i64)*]], align 32 ; <[28 x [28 x i64 (%struct.TypHeader*, i64, %struct.TypHeader*, i64)*]]*> [#uses=0]
@TabReduceCoeffsMod = external global [28 x [28 x i64 (%struct.TypHeader*, i64, %struct.TypHeader*, i64, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x i64 (%struct.TypHeader*, i64, %struct.TypHeader*, i64, %struct.TypHeader*)*]]*> [#uses=0]
@TabPowerModCoeffsInt = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@TabPowerModCoeffsLInt = external global [28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*)*]], align 32 ; <[28 x [28 x %struct.TypHeader* (%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*)*]]*> [#uses=0]
@.str995 = external constant [14 x i8], align 1   ; <[14 x i8]*> [#uses=0]
@.str1996 = external constant [16 x i8], align 1  ; <[16 x i8]*> [#uses=0]
@.str2997 = external constant [13 x i8], align 1  ; <[13 x i8]*> [#uses=0]
@.str3998 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str4999 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str51000 = external constant [14 x i8], align 1 ; <[14 x i8]*> [#uses=0]
@.str61001 = external constant [17 x i8], align 1 ; <[17 x i8]*> [#uses=0]
@.str71002 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str81003 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str91004 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str101005 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str111006 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str121007 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str131008 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@.str141009 = external constant [24 x i8], align 1 ; <[24 x i8]*> [#uses=0]
@.str151010 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str161011 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str171012 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str181013 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str191014 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str201015 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str211016 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str221017 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str231018 = external constant [27 x i8], align 1 ; <[27 x i8]*> [#uses=0]
@.str241019 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str251020 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str261021 = external constant [24 x i8], align 1 ; <[24 x i8]*> [#uses=0]
@.str271022 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str281023 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str1025 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=0]
@.str21027 = external constant [24 x i8], align 1 ; <[24 x i8]*> [#uses=0]
@.str31028 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str41029 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str51030 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str61031 = external constant [18 x i8], align 1 ; <[18 x i8]*> [#uses=0]
@.str81033 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str111036 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str121037 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str131038 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str141039 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str151040 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str1044 = external constant [6 x i8], align 1   ; <[6 x i8]*> [#uses=0]
@.str11045 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str21046 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str31047 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str41048 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str51049 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str61050 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str71051 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@.str81052 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str101054 = external constant [6 x i8], align 1 ; <[6 x i8]*> [#uses=0]
@.str121056 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str131057 = external constant [25 x i8], align 1 ; <[25 x i8]*> [#uses=0]
@.str11060 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@HdCurLHS = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@.str21061 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str31062 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str41063 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str61065 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str71066 = external constant [4 x i8], align 1  ; <[4 x i8]*> [#uses=0]
@.str81067 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str91068 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str101069 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str131072 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str141073 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str151074 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str161075 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str171076 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str181077 = external constant [2 x i8], align 1 ; <[2 x i8]*> [#uses=0]
@.str241083 = external constant [6 x i8], align 1 ; <[6 x i8]*> [#uses=0]
@.str251084 = external constant [4 x i8], align 1 ; <[4 x i8]*> [#uses=0]
@.str271086 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str281087 = external constant [17 x i8], align 1 ; <[17 x i8]*> [#uses=0]
@HdRnOp = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@HdRnEq = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@HdCallEq = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@HdStrEq = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@HdTilde = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@.str11093 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@HdRnSum = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@HdCallSum = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@.str31096 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@HdStrSum = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@HdRnDiff = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@HdCallDiff = external global %struct.TypHeader*  ; <%struct.TypHeader**> [#uses=0]
@.str51099 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@HdStrDiff = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@HdRnProd = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@HdCallProd = external global %struct.TypHeader*  ; <%struct.TypHeader**> [#uses=0]
@.str71102 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@HdStrProd = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@HdRnQuo = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@HdCallQuo = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@.str91105 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@HdStrQuo = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@HdRnMod = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@HdCallMod = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@.str111108 = external constant [18 x i8], align 1 ; <[18 x i8]*> [#uses=0]
@HdStrMod = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@HdRnPow = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@HdCallPow = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@.str131111 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@HdStrPow = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@HdRnComm = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@HdCallComm = external global %struct.TypHeader*  ; <%struct.TypHeader**> [#uses=0]
@.str151114 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@HdStrComm = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@.str171116 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@HdRnLt = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@HdCallLt = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@.str191119 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@HdStrLt = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@HdRnIn = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@HdCallIn = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@.str211122 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@HdStrIn = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@HdRnPrint = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@HdCallPrint = external global %struct.TypHeader* ; <%struct.TypHeader**> [#uses=0]
@.str231125 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@HdStrPrint = external global %struct.TypHeader*  ; <%struct.TypHeader**> [#uses=0]
@.str241126 = external constant [6 x i8], align 1 ; <[6 x i8]*> [#uses=0]
@.str251127 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str261128 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@.str271129 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str281130 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str291131 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str301132 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str311133 = external constant [49 x i8], align 8 ; <[49 x i8]*> [#uses=0]
@.str321134 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str331135 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str341136 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str351137 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str361138 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str371139 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str421144 = external constant [6 x i8], align 1 ; <[6 x i8]*> [#uses=0]
@.str661168 = external constant [7 x i8], align 1 ; <[7 x i8]*> [#uses=0]
@.str671169 = external constant [4 x i8], align 1 ; <[4 x i8]*> [#uses=0]
@.str681170 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str691171 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str711173 = external constant [9 x i8], align 1 ; <[9 x i8]*> [#uses=0]
@.str721174 = external constant [9 x i8], align 1 ; <[9 x i8]*> [#uses=0]
@.str731175 = external constant [6 x i8], align 1 ; <[6 x i8]*> [#uses=0]
@.str741176 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str761178 = external constant [49 x i8], align 8 ; <[49 x i8]*> [#uses=0]
@.str771179 = external constant [49 x i8], align 8 ; <[49 x i8]*> [#uses=0]
@.str781180 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str791181 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@Logfile = external global i64                    ; <i64*> [#uses=0]
@Input = external global %struct.TypInputFile*    ; <%struct.TypInputFile**> [#uses=0]
@TestInput = external global i64                  ; <i64*> [#uses=0]
@In = external global i8*                         ; <i8**> [#uses=0]
@Symbol = external global i64                     ; <i64*> [#uses=0]
@TestOutput = external global i64                 ; <i64*> [#uses=0]
@TestLine = external global [256 x i8], align 32  ; <[256 x i8]*> [#uses=0]
@InputLogfile = external global i64               ; <i64*> [#uses=0]
@InputFiles = external global [16 x %struct.TypInputFile], align 32 ; <[16 x %struct.TypInputFile]*> [#uses=0]
@.str1187 = external constant [2 x i8], align 1   ; <[2 x i8]*> [#uses=0]
@Output = external global %struct.TypOutputFile*  ; <%struct.TypOutputFile**> [#uses=0]
@OutputFiles = external global [16 x %struct.TypOutputFile], align 32 ; <[16 x %struct.TypOutputFile]*> [#uses=0]
@.str21189 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str41191 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str51192 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str61193 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str71194 = external constant [14 x i8], align 1 ; <[14 x i8]*> [#uses=0]
@NrError = external global i64                    ; <i64*> [#uses=0]
@NrErrLine = external global i64                  ; <i64*> [#uses=0]
@.str91198 = external constant [17 x i8], align 1 ; <[17 x i8]*> [#uses=0]
@.str101199 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str131202 = external constant [2 x i8], align 1 ; <[2 x i8]*> [#uses=0]
@.str141203 = external constant [2 x i8], align 1 ; <[2 x i8]*> [#uses=0]
@.str151204 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@Prompt = external global i8*                     ; <i8**> [#uses=0]
@.str161206 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@Value = external global [1024 x i8], align 32    ; <[1024 x i8]*> [#uses=0]
@.str171208 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str181209 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str191210 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str201211 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str211212 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str431234 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str1250 = external constant [4 x i8], align 1   ; <[4 x i8]*> [#uses=0]
@.str11251 = external constant [6 x i8], align 1  ; <[6 x i8]*> [#uses=0]
@.str21252 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str31253 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str41254 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str51255 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str61256 = external constant [9 x i8], align 1  ; <[9 x i8]*> [#uses=0]
@.str71257 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str81258 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@HdUnion = external global %struct.TypHeader*     ; <%struct.TypHeader**> [#uses=0]
@.str111261 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str121262 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str131263 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str141264 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str151265 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str161266 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str171267 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@.str181268 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str191269 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str201270 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str211271 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str221272 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str231273 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str241274 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str251275 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str261276 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str271277 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str281278 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str291279 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str301280 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str311281 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str321282 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str331283 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str341284 = external constant [20 x i8], align 1 ; <[20 x i8]*> [#uses=0]
@.str351285 = external constant [27 x i8], align 1 ; <[27 x i8]*> [#uses=0]
@StrStat = external global i8*                    ; <i8**> [#uses=0]
@HdStat = external global %struct.TypHeader*      ; <%struct.TypHeader**> [#uses=0]
@.str11292 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str21293 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str41295 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str51296 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str61297 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str71298 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str81299 = external constant [12 x i8], align 1 ; <[12 x i8]*> [#uses=0]
@.str91300 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str101301 = external constant [9 x i8], align 1 ; <[9 x i8]*> [#uses=0]
@.str111302 = external constant [14 x i8], align 1 ; <[14 x i8]*> [#uses=0]
@.str131304 = external constant [9 x i8], align 1 ; <[9 x i8]*> [#uses=0]
@.str151306 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str161307 = external constant [4 x i8], align 1 ; <[4 x i8]*> [#uses=0]
@.str171308 = external constant [6 x i8], align 1 ; <[6 x i8]*> [#uses=0]
@.str181309 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str191310 = external constant [7 x i8], align 1 ; <[7 x i8]*> [#uses=0]
@.str201311 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str211312 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str221313 = external constant [7 x i8], align 1 ; <[7 x i8]*> [#uses=0]
@.str231314 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str241315 = external constant [49 x i8], align 8 ; <[49 x i8]*> [#uses=0]
@.str251316 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str281319 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@.str291320 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@HdChars = external global [256 x %struct.TypHeader*], align 32 ; <[256 x %struct.TypHeader*]*> [#uses=0]
@.str1322 = external constant [9 x i8], align 1   ; <[9 x i8]*> [#uses=0]
@.str31326 = external constant [25 x i8], align 1 ; <[25 x i8]*> [#uses=0]
@.str41327 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str61329 = external constant [2 x i8], align 1  ; <[2 x i8]*> [#uses=0]
@.str71330 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str81331 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str91332 = external constant [3 x i8], align 1  ; <[3 x i8]*> [#uses=0]
@.str101333 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str111334 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str121335 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str131336 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str151338 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@.str161339 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@.str171340 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@.str181341 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@.str191342 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@.str201343 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@.str211344 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@.str221345 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@SyFlags = external global [13 x i8]              ; <[13 x i8]*> [#uses=0]
@syLastIntr = external global i64                 ; <i64*> [#uses=0]
@syWorkspace = external global i8*                ; <i8**> [#uses=0]
@stderr = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=0]
@.str1350 = external constant [38 x i8], align 8  ; <[38 x i8]*> [#uses=0]
@syStartTime = external global i64                ; <i64*> [#uses=0]
@.str11351 = external constant [52 x i8], align 8 ; <[52 x i8]*> [#uses=0]
@syBuf = external global [16 x %0], align 32      ; <[16 x %0]*> [#uses=0]
@syWindow.b = external global i1                  ; <i1*> [#uses=0]
@syOld = external global %struct.termio, align 16 ; <%struct.termio*> [#uses=0]
@.str21352 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@syFid = external global i64                      ; <i64*> [#uses=0]
@.str31353 = external constant [51 x i8], align 8 ; <[51 x i8]*> [#uses=0]
@.str41354 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str51355 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@stdin = external global %struct._IO_FILE*        ; <%struct._IO_FILE**> [#uses=0]
@stdout = external global %struct._IO_FILE*       ; <%struct._IO_FILE**> [#uses=0]
@.str121362 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str131363 = external constant [4 x i8], align 1 ; <[4 x i8]*> [#uses=0]
@SyBanner = external global i64                   ; <i64*> [#uses=0]
@SyGasman = external global i64                   ; <i64*> [#uses=0]
@.str141366 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@SyLibname = external global [256 x i8], align 32 ; <[256 x i8]*> [#uses=0]
@.str161369 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@SyHelpname = external global [256 x i8], align 32 ; <[256 x i8]*> [#uses=0]
@.str171370 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@SyMemory = external global i64                   ; <i64*> [#uses=0]
@.str181372 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@syLineEdit = external global i64                 ; <i64*> [#uses=0]
@SyQuiet = external global i64                    ; <i64*> [#uses=0]
@.str191374 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@SyNrCols = external global i64                   ; <i64*> [#uses=0]
@.str201376 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@SyNrRows = external global i64                   ; <i64*> [#uses=0]
@syCTRD = external global i64                     ; <i64*> [#uses=0]
@.str211378 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str231380 = external constant [7 x i8], align 1 ; <[7 x i8]*> [#uses=0]
@.str241381 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@SyInitfiles = external global [16 x [256 x i8]], align 32 ; <[16 x [256 x i8]]*> [#uses=0]
@.str251383 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str261384 = external constant [7 x i8], align 1 ; <[7 x i8]*> [#uses=0]
@.str271385 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str281386 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str291387 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@.str301388 = external constant [8 x i8], align 1 ; <[8 x i8]*> [#uses=0]
@.str311389 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str321390 = external constant [56 x i8], align 8 ; <[56 x i8]*> [#uses=0]
@.str331391 = external constant [53 x i8], align 8 ; <[53 x i8]*> [#uses=0]
@.str341392 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@.str351393 = external constant [54 x i8], align 8 ; <[54 x i8]*> [#uses=0]
@.str361394 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str371395 = external constant [7 x i8], align 1 ; <[7 x i8]*> [#uses=0]
@.str381396 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str391397 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@WinCmdBuffer = external global [8000 x i8], align 32 ; <[8000 x i8]*> [#uses=0]
@.str401398 = external constant [27 x i8], align 1 ; <[27 x i8]*> [#uses=0]
@syNrchar = external global i64                   ; <i64*> [#uses=0]
@syPrompt = external global [256 x i8], align 32  ; <[256 x i8]*> [#uses=0]
@.str411399 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str421400 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str431401 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str441402 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str451403 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@syNew = external global %struct.termio, align 16 ; <%struct.termio*> [#uses=0]
@syStopTime = external global i64                 ; <i64*> [#uses=0]
@syHistory = external global [8192 x i8], align 32 ; <[8192 x i8]*> [#uses=0]
@syCTRO = external global i32                     ; <i32*> [#uses=0]
@yank.3948 = external global [512 x i8], align 32 ; <[512 x i8]*> [#uses=0]
@syHi = external global i8*                       ; <i8**> [#uses=0]
@.str461404 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str471405 = external constant [6 x i8], align 1 ; <[6 x i8]*> [#uses=0]
@.str491407 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str501408 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str511409 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str521410 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@syLastIndex = external global i16                ; <i16*> [#uses=0]
@syLastTopics = external global [16 x [64 x i8]], align 32 ; <[16 x [64 x i8]]*> [#uses=0]
@.str551413 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str561414 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str571415 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str581416 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str591417 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str601418 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str611419 = external constant [21 x i8], align 1 ; <[21 x i8]*> [#uses=0]
@.str621420 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str631421 = external constant [4 x i8], align 1 ; <[4 x i8]*> [#uses=0]
@.str641422 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str651423 = external constant [18 x i8], align 1 ; <[18 x i8]*> [#uses=0]
@.str661424 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str671425 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str681426 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str691427 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str701428 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str711429 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str721430 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str731431 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str741432 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str751433 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str761434 = external constant [9 x i8], align 1 ; <[9 x i8]*> [#uses=0]
@.str771435 = external constant [9 x i8], align 1 ; <[9 x i8]*> [#uses=0]
@.str781436 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str791437 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str801438 = external constant [3 x i8], align 1 ; <[3 x i8]*> [#uses=0]
@.str811439 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str821440 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str831441 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str841442 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str851443 = external constant [27 x i8], align 1 ; <[27 x i8]*> [#uses=0]
@.str861444 = external constant [27 x i8], align 1 ; <[27 x i8]*> [#uses=0]
@.str871445 = external constant [27 x i8], align 1 ; <[27 x i8]*> [#uses=0]
@.str881446 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@.str891447 = external constant [9 x i8], align 1 ; <[9 x i8]*> [#uses=0]
@.str901448 = external constant [9 x i8], align 1 ; <[9 x i8]*> [#uses=0]
@.str911449 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str921450 = external constant [9 x i8], align 1 ; <[9 x i8]*> [#uses=0]
@.str931451 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str941452 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str951453 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str961454 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str971455 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str981456 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str991457 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str1001458 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str1011459 = external constant [8 x i8], align 1 ; <[8 x i8]*> [#uses=0]
@.str1021460 = external constant [24 x i8], align 1 ; <[24 x i8]*> [#uses=0]
@.str1031461 = external constant [32 x i8], align 8 ; <[32 x i8]*> [#uses=0]
@.str1041462 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str1051463 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str1061464 = external constant [67 x i8], align 8 ; <[67 x i8]*> [#uses=0]
@.str1071465 = external constant [6 x i8], align 1 ; <[6 x i8]*> [#uses=0]
@.str1081466 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str1111469 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str1121470 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str1131471 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str1141472 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str1151473 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@syChapnames = external global [128 x [16 x i8]], align 32 ; <[128 x [16 x i8]]*> [#uses=0]
@.str1161474 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str1171475 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str1181476 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str1191477 = external constant [5 x i8], align 1 ; <[5 x i8]*> [#uses=0]
@.str1201478 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str1211479 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str1221480 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str1231481 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str1241482 = external constant [20 x i8], align 1 ; <[20 x i8]*> [#uses=0]
@.str1251483 = external constant [71 x i8], align 8 ; <[71 x i8]*> [#uses=0]
@.str1261484 = external constant [8 x i8], align 1 ; <[8 x i8]*> [#uses=0]
@.str1271485 = external constant [9 x i8], align 1 ; <[9 x i8]*> [#uses=0]
@.str1281486 = external constant [8 x i8], align 1 ; <[8 x i8]*> [#uses=0]
@.str129 = external constant [6 x i8], align 1    ; <[6 x i8]*> [#uses=0]
@.str130 = external constant [17 x i8], align 1   ; <[17 x i8]*> [#uses=0]
@.str131 = external constant [2 x i8], align 1    ; <[2 x i8]*> [#uses=0]
@.str132 = external constant [9 x i8], align 1    ; <[9 x i8]*> [#uses=0]
@.str1504 = external constant [10 x i8], align 1  ; <[10 x i8]*> [#uses=0]
@.str11505 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str21506 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str31507 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@.str41508 = external constant [14 x i8], align 1 ; <[14 x i8]*> [#uses=0]
@.str51509 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str61510 = external constant [14 x i8], align 1 ; <[14 x i8]*> [#uses=0]
@.str71511 = external constant [19 x i8], align 1 ; <[19 x i8]*> [#uses=0]
@.str81512 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str91513 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str101514 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str111515 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str121516 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str131517 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str141518 = external constant [62 x i8], align 8 ; <[62 x i8]*> [#uses=0]
@.str151519 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str161520 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str171521 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str181522 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str191523 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str201524 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str211525 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str221526 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str231527 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str241528 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str251529 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str261530 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str271531 = external constant [63 x i8], align 8 ; <[63 x i8]*> [#uses=0]
@.str281532 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str291533 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str301534 = external constant [57 x i8], align 8 ; <[57 x i8]*> [#uses=0]
@.str311535 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str321536 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str331537 = external constant [54 x i8], align 8 ; <[54 x i8]*> [#uses=0]
@.str341538 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str351539 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str361540 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str371541 = external constant [63 x i8], align 8 ; <[63 x i8]*> [#uses=0]
@.str381542 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str391543 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str401544 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str411545 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str421546 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str431547 = external constant [25 x i8], align 1 ; <[25 x i8]*> [#uses=0]
@.str441548 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str451549 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str461550 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str471551 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str1553 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=0]
@.str11554 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str21555 = external constant [26 x i8], align 1 ; <[26 x i8]*> [#uses=0]
@.str31556 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str41557 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str51558 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@LargestUnknown = external global i64             ; <i64*> [#uses=0]
@.str61559 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str81561 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str91562 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@HdVecFFEL = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@HdVecFFER = external global %struct.TypHeader*   ; <%struct.TypHeader**> [#uses=0]
@TabIntVecFFE = external global [28 x %struct.TypHeader* (%struct.TypHeader*, i64)*], align 32 ; <[28 x %struct.TypHeader* (%struct.TypHeader*, i64)*]*> [#uses=0]
@.str1564 = external constant [8 x i8], align 1   ; <[8 x i8]*> [#uses=0]
@.str11565 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str21566 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str31567 = external constant [10 x i8], align 1 ; <[10 x i8]*> [#uses=0]
@.str41568 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str51569 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str61570 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str91573 = external constant [50 x i8], align 8 ; <[50 x i8]*> [#uses=0]
@.str101574 = external constant [48 x i8], align 8 ; <[48 x i8]*> [#uses=0]
@.str111575 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str121576 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str131577 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str141578 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str151579 = external constant [25 x i8], align 1 ; <[25 x i8]*> [#uses=0]
@.str161580 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str171581 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str181582 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str191583 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str201584 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str211585 = external constant [33 x i8], align 8 ; <[33 x i8]*> [#uses=0]
@.str241588 = external constant [43 x i8], align 8 ; <[43 x i8]*> [#uses=0]
@.str261590 = external constant [9 x i8], align 1 ; <[9 x i8]*> [#uses=0]
@.str281592 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str291593 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str301594 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str311595 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str321596 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str331597 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str341598 = external constant [44 x i8], align 8 ; <[44 x i8]*> [#uses=0]
@.str351599 = external constant [46 x i8], align 8 ; <[46 x i8]*> [#uses=0]
@.str361600 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str371601 = external constant [24 x i8], align 1 ; <[24 x i8]*> [#uses=0]
@.str381602 = external constant [65 x i8], align 8 ; <[65 x i8]*> [#uses=0]
@.str391603 = external constant [22 x i8], align 1 ; <[22 x i8]*> [#uses=0]
@.str401604 = external constant [63 x i8], align 8 ; <[63 x i8]*> [#uses=0]
@.str1619 = external constant [18 x i8], align 1  ; <[18 x i8]*> [#uses=0]
@.str11620 = external constant [19 x i8], align 1 ; <[19 x i8]*> [#uses=0]
@.str21621 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str31622 = external constant [8 x i8], align 1  ; <[8 x i8]*> [#uses=0]
@.str41623 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str51624 = external constant [13 x i8], align 1 ; <[13 x i8]*> [#uses=0]
@.str61625 = external constant [7 x i8], align 1  ; <[7 x i8]*> [#uses=0]
@.str71626 = external constant [16 x i8], align 1 ; <[16 x i8]*> [#uses=0]
@.str81627 = external constant [11 x i8], align 1 ; <[11 x i8]*> [#uses=0]
@.str91628 = external constant [15 x i8], align 1 ; <[15 x i8]*> [#uses=0]
@HdIdWord = external global %struct.TypHeader*    ; <%struct.TypHeader**> [#uses=0]
@.str101630 = external constant [7 x i8], align 1 ; <[7 x i8]*> [#uses=0]
@.str111631 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str121632 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str131633 = external constant [23 x i8], align 1 ; <[23 x i8]*> [#uses=0]
@.str141634 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str151635 = external constant [47 x i8], align 8 ; <[47 x i8]*> [#uses=0]
@.str161636 = external constant [35 x i8], align 8 ; <[35 x i8]*> [#uses=0]
@.str171637 = external constant [39 x i8], align 8 ; <[39 x i8]*> [#uses=0]
@.str181638 = external constant [30 x i8], align 1 ; <[30 x i8]*> [#uses=0]
@.str191639 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str201640 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str211641 = external constant [24 x i8], align 1 ; <[24 x i8]*> [#uses=0]
@.str221642 = external constant [42 x i8], align 8 ; <[42 x i8]*> [#uses=0]
@.str231643 = external constant [28 x i8], align 1 ; <[28 x i8]*> [#uses=0]
@.str241644 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str251645 = external constant [24 x i8], align 1 ; <[24 x i8]*> [#uses=0]
@.str261646 = external constant [45 x i8], align 8 ; <[45 x i8]*> [#uses=0]
@.str271647 = external constant [53 x i8], align 8 ; <[53 x i8]*> [#uses=0]
@.str281648 = external constant [38 x i8], align 8 ; <[38 x i8]*> [#uses=0]
@.str291649 = external constant [36 x i8], align 8 ; <[36 x i8]*> [#uses=0]
@.str301650 = external constant [29 x i8], align 1 ; <[29 x i8]*> [#uses=0]
@.str311651 = external constant [31 x i8], align 8 ; <[31 x i8]*> [#uses=0]
@.str341654 = external constant [40 x i8], align 8 ; <[40 x i8]*> [#uses=0]
@.str351655 = external constant [37 x i8], align 8 ; <[37 x i8]*> [#uses=0]
@.str361656 = external constant [41 x i8], align 8 ; <[41 x i8]*> [#uses=0]
@.str371657 = external constant [34 x i8], align 8 ; <[34 x i8]*> [#uses=0]
@.str401660 = external constant [6 x i8], align 1 ; <[6 x i8]*> [#uses=0]
@.str421662 = external constant [7 x i8], align 1 ; <[7 x i8]*> [#uses=0]
@.str431663 = external constant [4 x i8], align 1 ; <[4 x i8]*> [#uses=0]

declare fastcc i32 @OrdinaryCollect() nounwind

declare fastcc void @AddString2(i16* nocapture) nounwind

declare i32 @AgCombinatorial2(i64*, %struct.TypHeader* nocapture) nounwind

declare i32 @AgSingle(i64* nocapture, %struct.TypHeader* nocapture) nounwind

declare i32 @AgTriple(i64* nocapture, %struct.TypHeader* nocapture) nounwind

declare i32 @AgQuadruple(i64* nocapture, %struct.TypHeader* nocapture) nounwind

declare fastcc void @SetAvecAgGroup(%struct.TypHeader* nocapture, i64, i64) nounwind

declare fastcc void @SetGeneratorsAgGroup(%struct.TypHeader*) nounwind

declare fastcc %struct.TypHeader* @AgWordAgExp(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare fastcc %struct.TypHeader* @SaveAndClearCollector(%struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @BlankAgGroup() nounwind

declare fastcc void @SetStacksAgGroup(%struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @EvalOopN(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader* nocapture, i8* nocapture) nounwind

declare fastcc %struct.TypHeader* @EvalOop2(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*, i8* nocapture) nounwind

declare fastcc %struct.TypHeader* @EvalOop(%struct.TypHeader*, %struct.TypHeader*, i8* nocapture) nounwind

declare fastcc void @AddString(i16* nocapture, i64) nounwind

declare fastcc void @AddGen() nounwind

declare i32 @AgCombinatorial(i64*, %struct.TypHeader* nocapture) nounwind

declare fastcc i32 @SetCWeightsAgGroup(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare void @InitCombinatorial(%struct.TypHeader* nocapture, i64) nounwind

declare void @InitSingle(%struct.TypHeader* nocapture, i64) nounwind

declare fastcc void @Collect(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare void @InitTriple(%struct.TypHeader* nocapture, i64) nounwind

declare void @InitQuadr(%struct.TypHeader* nocapture, i64) nounwind

declare fastcc %struct.TypHeader* @AgSolution2(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture, %struct.TypHeader** nocapture, %struct.TypHeader*) nounwind

declare fastcc %struct.TypHeader* @AgSolution(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EqAg(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @LtAg(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @EvAg(%struct.TypHeader*) nounwind readnone

declare fastcc %struct.TypHeader* @IntExponentsAgWord(%struct.TypHeader* nocapture, i64, i64) nounwind

declare %struct.TypHeader* @ProdAg(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @QuoAg(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ModAg(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowAgI(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowAgAg(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @CommAg(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunSumAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDifferenceAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDepthAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunCentralWeightAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunExponentsAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunFactorAgGroup(%struct.TypHeader* nocapture) nounwind

declare void @PrAgen(%struct.TypHeader* nocapture) nounwind

declare void @PrAgList(%struct.TypHeader* nocapture) nounwind

declare void @PrAgExp(%struct.TypHeader* nocapture) nounwind

declare void @PrAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunAgProfile(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @TEqAg(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @TLtAg(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @TProdAg(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @TQuoAg(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @TModAg(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @TPowAgI(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @TPowAgAg(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @TCommAg(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunAgGroupRecord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDUMPLONG(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunCollectorProfile(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsCompatibleAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunExponentAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunReducedAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunInformationAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunRelativeOrderAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunLeadingExponentAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTailDepthAgWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunNormalizeIgs(%struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @DifferenceAgWord(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @SumAgWord(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunFactorAgWord(%struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @FactorAgGroup(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @FunSetCollectorAgWord(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunAgFpGroup(%struct.TypHeader* nocapture) nounwind

declare i64 @LenBlist(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @ElmfBlist(%struct.TypHeader* nocapture, i64) nounwind readonly

declare i64 @PosBlist(%struct.TypHeader* nocapture, %struct.TypHeader*, i64) nounwind readonly

declare i64 @IsDenseBlist(%struct.TypHeader* nocapture) nounwind readnone

declare i64 @IsPossBlist(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @EqBlist(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @ElmBlist(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @ElmsBlist(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AssBlist(%struct.TypHeader*, i64, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AsssBlist(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare void @PlainBlist(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunIsBlist(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunBlistList(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunListBlist(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSizeBlist(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsSubsetBlist(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIntersectBlist(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunUniteBlist(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSubtractBlist(%struct.TypHeader* nocapture) nounwind

declare fastcc i64 @IsBlist(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunDistanceBlist(%struct.TypHeader* nocapture) nounwind

declare fastcc void @CVCM2V2(%struct.TypHeader*, %struct.TypHeader*, i64, i64, %struct.TypHeader*, i64, i64*, %struct.TypHeader*) nounwind

declare fastcc void @CVCMFVF(%struct.TypHeader*, i64, %struct.TypHeader*, i64, i64, %struct.TypHeader*, i64, i64, i64*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunDistanceVecFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDistancesDistributionVecFFEsVecFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDistancesDistributionMatFFEVecFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunCosetLeadersMatFFE(%struct.TypHeader* nocapture) nounwind

declare fastcc i64 @CLMF(%struct.TypHeader*, i64, %struct.TypHeader*, i64, %struct.TypHeader*, %struct.TypHeader*, i64, i64, i64) nounwind

declare fastcc i64 @CLM2(%struct.TypHeader*, i64, i64, %struct.TypHeader*, %struct.TypHeader*, i64, i64) nounwind

declare fastcc %struct.TypHeader* @BlistsMatFF2(%struct.TypHeader*) nounwind

declare fastcc void @DDMFVF(%struct.TypHeader*, i64, %struct.TypHeader*, %struct.TypHeader*, i64, i64, %struct.TypHeader*) nounwind

declare fastcc void @DDM2V2(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*, i64, %struct.TypHeader*) nounwind

declare fastcc i64 @ConvVecFFE(%struct.TypHeader*, i64) nounwind

declare fastcc i64 @ConvMatFFE(%struct.TypHeader*, i64) nounwind

declare %struct.TypHeader* @FunAClosestVectorCombinationsMatFFEVecFFE(%struct.TypHeader* nocapture) nounwind

declare fastcc void @CompressDeductionList() nounwind

declare %struct.TypHeader* @FunStandardizeTable(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunStandardizeTable2(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunApplyRel(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunMakeConsequences(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunApplyRel2(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunCopyRel(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunMakeCanonical(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTreeEntry(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunMakeConsequences2(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunAddAbelianRelator(%struct.TypHeader* nocapture) nounwind

declare fastcc i64 @TreeEntryC() nounwind

declare fastcc void @AddCosetFactor2(i64) nounwind

declare fastcc void @HandleCoinc2(i64, i64, %struct.TypHeader*) nounwind

declare fastcc void @HandleCoinc(i64, i64) nounwind

declare fastcc void @ConvertToBase(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @EvCyc(%struct.TypHeader*) nounwind readnone

declare %struct.TypHeader* @EqCyc(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @LtCyc(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdCycI(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare fastcc %struct.TypHeader* @Cyclotomic(%struct.TypHeader* nocapture, i64, i64) nounwind

declare void @PrCyc(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @SumCyc(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffCyc(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdCyc(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @QuoCyc(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowCyc(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsCyc(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsCycInt(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunNofCyc(%struct.TypHeader* nocapture) nounwind

define internal %struct.TypHeader* @FunCoeffsCyc(%struct.TypHeader* nocapture %hdCall) nounwind {
entry:
  ret %struct.TypHeader* null
}

define internal %struct.TypHeader* @FunGaloisCyc(%struct.TypHeader* nocapture %hdCall) nounwind {
entry:
  unreachable
}

declare %struct.TypHeader* @CantEval(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @Sum(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @Diff(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @Prod(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @Quo(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @Mod(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @Pow(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @Eq(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @Lt(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @Ne(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @Le(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @Gt(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @Ge(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @IsTrue(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @IsFalse(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @EvBool(%struct.TypHeader*) nounwind readnone

declare %struct.TypHeader* @EqBool(%struct.TypHeader*, %struct.TypHeader*) nounwind readonly

declare %struct.TypHeader* @LtBool(%struct.TypHeader*, %struct.TypHeader*) nounwind readonly

define internal fastcc void @InitEval() nounwind {
bb.nph49:
  br label %bb

bb:                                               ; preds = %bb, %bb.nph49
  br i1 undef, label %bb5.preheader, label %bb

bb4:                                              ; preds = %bb5.preheader, %bb4
  br i1 undef, label %bb6, label %bb4

bb6:                                              ; preds = %bb4
  br i1 undef, label %bb11.preheader, label %bb5.preheader

bb5.preheader:                                    ; preds = %bb6, %bb
  br label %bb4

bb10:                                             ; preds = %bb11.preheader, %bb10
  br i1 undef, label %bb15, label %bb10

bb15:                                             ; preds = %bb10
  br i1 undef, label %bb17, label %bb11.preheader

bb11.preheader:                                   ; preds = %bb15, %bb6
  br label %bb10

bb17:                                             ; preds = %bb15
  br i1 undef, label %InstIntFunc.exit, label %bb.i

bb.i:                                             ; preds = %bb17
  unreachable

InstIntFunc.exit:                                 ; preds = %bb17
  br i1 undef, label %InstIntFunc.exit8, label %bb.i7

bb.i7:                                            ; preds = %InstIntFunc.exit
  unreachable

InstIntFunc.exit8:                                ; preds = %InstIntFunc.exit
  br i1 undef, label %InstVar.exit28, label %bb.i27

bb.i27:                                           ; preds = %InstIntFunc.exit8
  unreachable

InstVar.exit28:                                   ; preds = %InstIntFunc.exit8
  br i1 undef, label %InstVar.exit, label %bb.i25

bb.i25:                                           ; preds = %InstVar.exit28
  unreachable

InstVar.exit:                                     ; preds = %InstVar.exit28
  br i1 undef, label %InstIntFunc.exit23, label %bb.i22

bb.i22:                                           ; preds = %InstVar.exit
  unreachable

InstIntFunc.exit23:                               ; preds = %InstVar.exit
  br i1 undef, label %InstIntFunc.exit20, label %bb.i19

bb.i19:                                           ; preds = %InstIntFunc.exit23
  unreachable

InstIntFunc.exit20:                               ; preds = %InstIntFunc.exit23
  br i1 undef, label %InstIntFunc.exit17, label %bb.i16

bb.i16:                                           ; preds = %InstIntFunc.exit20
  br label %InstIntFunc.exit17

InstIntFunc.exit17:                               ; preds = %bb.i16, %InstIntFunc.exit20
  %tmp79 = tail call fastcc %struct.TypHeader* @NewBag(i32 16, i64 8) nounwind ; <%struct.TypHeader*> [#uses=2]
  %tmp80 = getelementptr inbounds %struct.TypHeader* %tmp79, i64 0, i32 1 ; <%struct.TypHeader***> [#uses=1]
  %tmp81 = load %struct.TypHeader*** %tmp80, align 8 ; <%struct.TypHeader**> [#uses=1]
  store %struct.TypHeader* bitcast (%struct.TypHeader* (%struct.TypHeader*)* @FunIsBound to %struct.TypHeader*), %struct.TypHeader** %tmp81
  %tmp82 = tail call fastcc %struct.TypHeader* @FindIdent(i8* getelementptr inbounds ([8 x i8]* @.str8302, i64 0, i64 0)) nounwind ; <%struct.TypHeader*> [#uses=1]
  %tmp83 = getelementptr inbounds %struct.TypHeader* %tmp82, i64 0, i32 1 ; <%struct.TypHeader***> [#uses=1]
  %tmp84 = load %struct.TypHeader*** %tmp83, align 8 ; <%struct.TypHeader**> [#uses=1]
  br i1 undef, label %InstIntFunc.exit14, label %bb.i13

bb.i13:                                           ; preds = %InstIntFunc.exit17
  unreachable

InstIntFunc.exit14:                               ; preds = %InstIntFunc.exit17
  store %struct.TypHeader* %tmp79, %struct.TypHeader** %tmp84, align 8
  br i1 undef, label %InstIntFunc.exit11, label %bb.i10

bb.i10:                                           ; preds = %InstIntFunc.exit14
  unreachable

InstIntFunc.exit11:                               ; preds = %InstIntFunc.exit14
  br i1 undef, label %InstIntFunc.exit9.i, label %bb.i8.i

bb.i8.i:                                          ; preds = %InstIntFunc.exit11
  unreachable

InstIntFunc.exit9.i:                              ; preds = %InstIntFunc.exit11
  br i1 undef, label %InstIntFunc.exit6.i, label %bb.i5.i

bb.i5.i:                                          ; preds = %InstIntFunc.exit9.i
  unreachable

InstIntFunc.exit6.i:                              ; preds = %InstIntFunc.exit9.i
  br i1 undef, label %InstIntFunc.exit3.i, label %bb.i2.i

bb.i2.i:                                          ; preds = %InstIntFunc.exit6.i
  unreachable

InstIntFunc.exit3.i:                              ; preds = %InstIntFunc.exit6.i
  br i1 undef, label %InitInt.exit, label %bb.i.i

bb.i.i:                                           ; preds = %InstIntFunc.exit3.i
  br label %InitInt.exit

InitInt.exit:                                     ; preds = %bb.i.i, %InstIntFunc.exit3.i
  br i1 undef, label %InstIntFunc.exit6.i419, label %bb.i5.i418

bb.i5.i418:                                       ; preds = %InitInt.exit
  unreachable

InstIntFunc.exit6.i419:                           ; preds = %InitInt.exit
  br i1 undef, label %InstIntFunc.exit3.i422, label %bb.i2.i421

bb.i2.i421:                                       ; preds = %InstIntFunc.exit6.i419
  unreachable

InstIntFunc.exit3.i422:                           ; preds = %InstIntFunc.exit6.i419
  br i1 undef, label %InitRat.exit, label %bb.i.i424

bb.i.i424:                                        ; preds = %InstIntFunc.exit3.i422
  unreachable

InitRat.exit:                                     ; preds = %InstIntFunc.exit3.i422
  br label %bb.i396

bb.i396:                                          ; preds = %bb.i396, %InitRat.exit
  br i1 undef, label %bb2.i398, label %bb.i396

bb2.i398:                                         ; preds = %bb.i396
  br i1 undef, label %InstIntFunc.exit15.i401, label %bb.i14.i400

bb.i14.i400:                                      ; preds = %bb2.i398
  br label %InstIntFunc.exit15.i401

InstIntFunc.exit15.i401:                          ; preds = %bb.i14.i400, %bb2.i398
  br i1 undef, label %InstIntFunc.exit12.i404, label %bb.i11.i403

bb.i11.i403:                                      ; preds = %InstIntFunc.exit15.i401
  unreachable

InstIntFunc.exit12.i404:                          ; preds = %InstIntFunc.exit15.i401
  br i1 undef, label %InstIntFunc.exit9.i407, label %bb.i8.i406

bb.i8.i406:                                       ; preds = %InstIntFunc.exit12.i404
  unreachable

InstIntFunc.exit9.i407:                           ; preds = %InstIntFunc.exit12.i404
  br i1 undef, label %InstIntFunc.exit6.i410, label %bb.i5.i409

bb.i5.i409:                                       ; preds = %InstIntFunc.exit9.i407
  br label %InstIntFunc.exit6.i410

InstIntFunc.exit6.i410:                           ; preds = %bb.i5.i409, %InstIntFunc.exit9.i407
  %tmp215 = tail call fastcc %struct.TypHeader* @NewBag(i32 16, i64 8) nounwind ; <%struct.TypHeader*> [#uses=2]
  %tmp216 = getelementptr inbounds %struct.TypHeader* %tmp215, i64 0, i32 1 ; <%struct.TypHeader***> [#uses=1]
  %tmp217 = load %struct.TypHeader*** %tmp216, align 8 ; <%struct.TypHeader**> [#uses=1]
  store %struct.TypHeader* bitcast (%struct.TypHeader* (%struct.TypHeader*)* @FunCoeffsCyc to %struct.TypHeader*), %struct.TypHeader** %tmp217
  %tmp218 = tail call fastcc %struct.TypHeader* @FindIdent(i8* getelementptr inbounds ([10 x i8]* @.str4251, i64 0, i64 0)) nounwind ; <%struct.TypHeader*> [#uses=1]
  %tmp219 = getelementptr inbounds %struct.TypHeader* %tmp218, i64 0, i32 1 ; <%struct.TypHeader***> [#uses=1]
  %tmp220 = load %struct.TypHeader*** %tmp219, align 8 ; <%struct.TypHeader**> [#uses=1]
  br i1 undef, label %InstIntFunc.exit3.i413, label %bb.i2.i412

bb.i2.i412:                                       ; preds = %InstIntFunc.exit6.i410
  unreachable

InstIntFunc.exit3.i413:                           ; preds = %InstIntFunc.exit6.i410
  store %struct.TypHeader* %tmp215, %struct.TypHeader** %tmp220, align 8
  unreachable
}

declare void @CantPrint(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @CantSum(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @CantDiff(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @CantProd(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @CantQuo(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @CantMod(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @CantPow(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @IntComm(%struct.TypHeader* nocapture) nounwind

declare void @PrBinop(%struct.TypHeader*) nounwind

declare void @PrComm(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvVar(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvVarAuto(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvVarAss(%struct.TypHeader* nocapture) nounwind

declare void @PrVar(%struct.TypHeader* nocapture) nounwind

declare void @PrVarAss(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvNot(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvAnd(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvOr(%struct.TypHeader* nocapture) nounwind

declare void @PrBool(%struct.TypHeader*) nounwind

declare void @PrNot(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsBool(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunShallowCopy(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunCopy(%struct.TypHeader* nocapture) nounwind

define internal %struct.TypHeader* @FunIsBound(%struct.TypHeader* nocapture %hdCall) nounwind {
entry:
  br i1 undef, label %bb1, label %bb

bb:                                               ; preds = %entry
  ret %struct.TypHeader* undef

bb1:                                              ; preds = %entry
  br i1 undef, label %bb3, label %bb24

bb3:                                              ; preds = %bb1
  br i1 undef, label %bb25, label %bb24

bb24:                                             ; preds = %bb3, %bb1
  ret %struct.TypHeader* undef

bb25:                                             ; preds = %bb3
  br i1 undef, label %bb28, label %bb86

bb28:                                             ; preds = %bb25
  br i1 undef, label %bb133, label %bb86

bb86:                                             ; preds = %bb28, %bb25
  br i1 undef, label %bb93, label %bb94

bb93:                                             ; preds = %bb86
  unreachable

bb94:                                             ; preds = %bb86
  br i1 undef, label %bb97, label %bb108

bb97:                                             ; preds = %bb94
  ret %struct.TypHeader* undef

bb108:                                            ; preds = %bb94
  br i1 undef, label %bb110, label %bb109

bb109:                                            ; preds = %bb108
  %tmp91 = bitcast %struct.TypHeader** undef to i8* ; <i8*> [#uses=1]
  %tmp92 = call fastcc %struct.TypHeader* @FindRecname(i8* %tmp91) nounwind ; <%struct.TypHeader*> [#uses=0]
  unreachable

bb110:                                            ; preds = %bb108
  ret %struct.TypHeader* undef

bb133:                                            ; preds = %bb28
  ret %struct.TypHeader* undef
}

declare %struct.TypHeader* @FunLeftQuotient(%struct.TypHeader* nocapture) nounwind

define internal %struct.TypHeader* @CantComm(%struct.TypHeader* %hdL, %struct.TypHeader* %hdR) nounwind {
entry:
  unreachable
}

declare i16** @__ctype_b_loc() nounwind readnone

declare fastcc void @Print(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunUnbind(%struct.TypHeader* nocapture) nounwind

declare fastcc void @CopyCleanup(%struct.TypHeader*) nounwind

declare fastcc void @CopyCopy(%struct.TypHeader*, %struct.TypHeader* nocapture) nounwind

declare fastcc void @CopyForward(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare fastcc %struct.TypHeader* @CopyShadow(%struct.TypHeader*) nounwind

declare fastcc %struct.TypHeader* @Copy(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @EvFFE(%struct.TypHeader*) nounwind readnone

declare %struct.TypHeader* @EqFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @LtFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare void @PrFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @SumFFE(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffFFE(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdFFE(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @QuoFFE(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowFFE(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunIsFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunLogFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIntFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunZ(%struct.TypHeader* nocapture) nounwind

declare fastcc void @PrFF(%struct.TypHeader** nocapture, i32) nounwind

declare fastcc %struct.TypHeader* @RootFiniteField(i64) nounwind

declare fastcc %struct.TypHeader* @ConvTabIntFFE(i64) nounwind

declare fastcc %struct.TypHeader* @CommonFF(%struct.TypHeader** nocapture, %struct.TypHeader* nocapture) nounwind

declare fastcc void @ChangeEnv(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @EvFunction(%struct.TypHeader*) nounwind readnone

declare %struct.TypHeader* @EvReturn(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvFunccall(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @EvMakefunc(%struct.TypHeader* nocapture) nounwind

declare void @PrFunccall(%struct.TypHeader* nocapture) nounwind

declare void @PrFunction(%struct.TypHeader* nocapture) nounwind

declare void @PrFuncint(%struct.TypHeader* nocapture) nounwind

declare void @PrReturn(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTrace(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunUntrace(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunProfile(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunApplyFunc(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsFunc(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIgnore(%struct.TypHeader* nocapture) nounwind readonly

declare fastcc i64 @SizeObj(%struct.TypHeader*) nounwind

declare fastcc void @MarkObj(%struct.TypHeader*) nounwind

declare fastcc %struct.TypHeader* @Error(i8* nocapture, i64, i64) nounwind

declare void @longjmp(%struct.__jmp_buf_tag*, i32) noreturn nounwind

declare fastcc void @InitGap(i32, i8** nocapture) nounwind

declare i32 @_setjmp(%struct.__jmp_buf_tag*) nounwind

declare %struct.TypHeader* @FunSIZEHANDLES(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunNUMBERHANDLES(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunCoefficients(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunGASMAN(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSIZE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTYPE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunOBJ(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunHANDLE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsIdentical(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTmpName(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSizeScreen(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunRuntime(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunExec(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunHelp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunReadTest(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunLogInputTo(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunLogTo(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunAppendTo(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunPrntTo(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunPrint(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunAUTO(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunREAD(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunWindowCmd(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunError(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunBacktrace(%struct.TypHeader*) nounwind

declare i32 @main(i32, i8** nocapture) noreturn nounwind

declare fastcc void @ExitKernel(%struct.TypHeader*) nounwind

declare fastcc void @CollectGarb() nounwind

define internal fastcc %struct.TypHeader* @NewBag(i32 %type, i64 %size) nounwind {
entry:
  br i1 undef, label %bb3, label %bb2

bb2:                                              ; preds = %entry
  br i1 undef, label %bb22, label %bb28

bb3:                                              ; preds = %entry
  unreachable

bb22:                                             ; preds = %bb2
  unreachable

bb28:                                             ; preds = %bb2
  br i1 undef, label %bb30, label %bb29

bb29:                                             ; preds = %bb28
  unreachable

bb30:                                             ; preds = %bb28
  br i1 undef, label %bb33, label %bb31

bb31:                                             ; preds = %bb30
  br i1 undef, label %bb32, label %bb33

bb32:                                             ; preds = %bb31
  tail call fastcc void @Resize(%struct.TypHeader* undef, i64 undef) nounwind
  ret %struct.TypHeader* undef

bb33:                                             ; preds = %bb31, %bb30
  ret %struct.TypHeader* undef
}

define internal fastcc void @Resize(%struct.TypHeader* %hdBag, i64 %newSize) nounwind {
entry:
  br i1 undef, label %bb1, label %bb2

bb1:                                              ; preds = %entry
  br label %bb2

bb2:                                              ; preds = %bb1, %entry
  br i1 undef, label %bb3, label %bb4

bb3:                                              ; preds = %bb2
  ret void

bb4:                                              ; preds = %bb2
  %tmp53 = tail call fastcc %struct.TypHeader* @NewBag(i32 undef, i64 %newSize) nounwind ; <%struct.TypHeader*> [#uses=0]
  unreachable
}

declare fastcc i64 @completion(i8* nocapture, i64, i64) nounwind

define internal fastcc %struct.TypHeader* @FindRecname(i8* nocapture %name) nounwind {
entry:
  br i1 undef, label %bb8, label %bb5

bb5:                                              ; preds = %entry
  unreachable

bb8:                                              ; preds = %entry
  %tmp24 = tail call fastcc %struct.TypHeader* @NewBag(i32 78, i64 0) nounwind ; <%struct.TypHeader*> [#uses=1]
  %tmp26 = getelementptr inbounds %struct.TypHeader* %tmp24, i64 0, i32 1 ; <%struct.TypHeader***> [#uses=1]
  %tmp27 = load %struct.TypHeader*** %tmp26, align 8 ; <%struct.TypHeader**> [#uses=1]
  %tmp29 = bitcast %struct.TypHeader** %tmp27 to i8* ; <i8*> [#uses=1]
  %tmp30 = tail call i8* @strncat(i8* %tmp29, i8* %name, i64 undef) nounwind ; <i8*> [#uses=0]
  unreachable
}

define internal fastcc %struct.TypHeader* @FindIdent(i8* nocapture %name) nounwind {
entry:
  br i1 undef, label %bb10, label %bb8

bb8:                                              ; preds = %entry
  br label %bb10

bb10:                                             ; preds = %bb8, %entry
  %tmp28 = load %struct.TypHeader** @HdIdenttab, align 8 ; <%struct.TypHeader*> [#uses=1]
  %tmp32 = getelementptr inbounds %struct.TypHeader* %tmp28, i64 0, i32 1 ; <%struct.TypHeader***> [#uses=1]
  %tmp33 = load %struct.TypHeader*** %tmp32, align 8 ; <%struct.TypHeader**> [#uses=1]
  br label %bb12

bb11:                                             ; preds = %bb13
  br label %bb12

bb12:                                             ; preds = %bb11, %bb10
  %tmp36 = getelementptr inbounds %struct.TypHeader** %tmp33, i64 undef ; <%struct.TypHeader**> [#uses=1]
  %tmp37 = load %struct.TypHeader** %tmp36, align 8 ; <%struct.TypHeader*> [#uses=2]
  br i1 undef, label %bb19, label %bb13

bb13:                                             ; preds = %bb12
  br i1 undef, label %bb14, label %bb11

bb14:                                             ; preds = %bb13
  br i1 undef, label %bb19, label %bb15

bb15:                                             ; preds = %bb14
  br i1 undef, label %bb18, label %bb17

bb17:                                             ; preds = %bb15
  ret %struct.TypHeader* %tmp37

bb18:                                             ; preds = %bb15
  ret %struct.TypHeader* %tmp37

bb19:                                             ; preds = %bb14, %bb12
  unreachable
}

declare %struct.TypHeader* @EvInt(%struct.TypHeader*) nounwind readnone

declare %struct.TypHeader* @EqInt(%struct.TypHeader*, %struct.TypHeader*) nounwind readonly

declare %struct.TypHeader* @LtInt(%struct.TypHeader*, %struct.TypHeader*) nounwind readonly

declare void @PrInteger(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @SumInt(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffInt(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdInt(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ModInt(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowInt(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunIsInt(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunQuo(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunRem(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunGcdInt(%struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @GcdInt(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare fastcc %struct.TypHeader* @RemInt(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare fastcc %struct.TypHeader* @QuoInt(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare i64 @CantLenList(%struct.TypHeader* nocapture) nounwind

declare i64 @NotIsDenseList(%struct.TypHeader* nocapture) nounwind readnone

declare i64 @NotIsPossList(%struct.TypHeader* nocapture) nounwind readnone

declare %struct.TypHeader* @EvList(%struct.TypHeader*) nounwind readnone

declare %struct.TypHeader* @EqList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @LtList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @SumList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @QuoList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @QuoLists(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ModList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ModLists(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowLists(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @CommList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @CommLists(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @CantElmList(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @CantElmsList(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @CantAssList(%struct.TypHeader* nocapture, i64, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @CantAsssList(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare i64 @CantPosList(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture, i64) nounwind

declare void @CantPlainList(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @DepthListx(%struct.TypHeader*) nounwind

declare void @PrList(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @SumSclList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @SumListScl(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffSclList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffListScl(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdSclList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdListScl(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @EvElmList(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvElmListLevel(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvElmsList(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvElmsListLevel(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvAssList(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvAssListLevel(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvAsssList(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvAsssListLevel(%struct.TypHeader* nocapture) nounwind

declare void @PrElmList(%struct.TypHeader* nocapture) nounwind

declare void @PrElmsList(%struct.TypHeader* nocapture) nounwind

declare void @PrAssList(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvIn(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsList(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsVector(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsMat(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunLength(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunAdd(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunAppend(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunPosition(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunOnPoints(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunOnPairs(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunOnTuples(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunOnSets(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunOnRight(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunOnLeft(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDepthVector(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @CantDepthVector(%struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @AsssListLevel(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*, i64) nounwind

declare fastcc %struct.TypHeader* @AssListLevel(%struct.TypHeader*, i64, %struct.TypHeader*, i64) nounwind

declare fastcc %struct.TypHeader* @ElmsListLevel(%struct.TypHeader*, %struct.TypHeader*, i64) nounwind

declare fastcc %struct.TypHeader* @ElmListLevel(%struct.TypHeader*, i64, i64) nounwind

declare %struct.TypHeader* @ProdListList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffListList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @SumListList(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare void @PrPcPres(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTriangleIndex(%struct.TypHeader* nocapture) nounwind

declare fastcc i32 @IsNormedPcp(%struct.TypHeader*, %struct.TypHeader** nocapture) nounwind

declare %struct.TypHeader* @FunTailReducedPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunBaseReducedPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTailDepthPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDepthPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunExponentsPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunExponentPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDifferencePcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSumPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSubtractPowerPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunAddPowerPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDefinePowerPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSubtractCommPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunAddCommPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDefineCommPcp(%struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @NormalWordPcp(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunPowerPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunConjugatePcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunCommPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunQuotientPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunLeftQuotientPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunProductPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunNormalWordPcp(%struct.TypHeader* nocapture) nounwind

declare fastcc void @ShrinkSwords(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunCentralWeightsPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunShrinkPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunGeneratorsPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDefineCentralWeightsPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunExtendCentralPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunAgPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunPcp(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvPerm(%struct.TypHeader*) nounwind readnone

declare %struct.TypHeader* @EqPP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @EqPQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @EqQP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @EqQQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @LtPP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @LtPQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @LtQP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @LtQQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @EvMakeperm(%struct.TypHeader* nocapture) nounwind

declare void @PrMakeperm(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @PowPI(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowIP(%struct.TypHeader*, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @QuoIP(%struct.TypHeader*, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @PowPP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsPerm(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunPermList(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunOrderPerm(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSignPerm(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSmallestGeneratorPerm(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @CommQQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @CommQP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @CommPQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @CommPP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @PowQQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @PowQP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @PowPQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ModQQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ModQP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ModPQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ModPP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdQQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdQP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdPQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdPP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunCyclePermInt(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunCycleLengthPermInt(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunLargestMovedPointPerm(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @QuoIQ(%struct.TypHeader*, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @PowIQ(%struct.TypHeader*, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @QuoQQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @QuoQP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @QuoPQ(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @QuoPP(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @PowQI(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare void @PrPermQ(%struct.TypHeader* nocapture) nounwind

declare void @PrPermP(%struct.TypHeader* nocapture) nounwind

declare i64 @LenPlist(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @ElmfPlist(%struct.TypHeader* nocapture, i64) nounwind readonly

declare i64 @PosPlist(%struct.TypHeader* nocapture, %struct.TypHeader*, i64) nounwind

declare void @PlainPlist(%struct.TypHeader* nocapture) nounwind readnone

declare i64 @IsDensePlist(%struct.TypHeader* nocapture) nounwind readonly

declare i64 @IsPossPlist(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @EqPlist(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @LtPlist(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ElmPlist(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @ElmsPlist(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AssPlist(%struct.TypHeader*, i64, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AsssPlist(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @EvMakeList(%struct.TypHeader* nocapture) nounwind

declare void @PrMakeList(%struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @MakeList(%struct.TypHeader*, i64, %struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @UnifiedFieldVecFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare void @AddCoeffsListxListx(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare i64 @MultiplyCoeffsListxListx(%struct.TypHeader*, %struct.TypHeader*, i64, %struct.TypHeader*, i64) nounwind

declare %struct.TypHeader* @CantNormalizeCoeffs(%struct.TypHeader* nocapture) nounwind

declare void @CantShrinkCoeffs(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @CantShiftedCoeffs(%struct.TypHeader* nocapture, i64) nounwind

declare void @CantAddCoeffs(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare i64 @CantMultiplyCoeffs(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture, i64, %struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @CantProductCoeffs(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @CantProductCoeffsMod(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare i64 @CantReduceCoeffs(%struct.TypHeader* nocapture, i64, %struct.TypHeader* nocapture, i64) nounwind

declare i64 @CantReduceCoeffsMod(%struct.TypHeader* nocapture, i64, %struct.TypHeader* nocapture, i64, %struct.TypHeader* nocapture) nounwind

declare noalias %struct.TypHeader* @CantPowerModCoeffs(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @NormalizeCoeffsListx(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @NormalizeCoeffsVecFFE(%struct.TypHeader*) nounwind

declare void @ShrinkCoeffsListx(%struct.TypHeader*) nounwind

declare void @ShrinkCoeffsVecFFE(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @ShiftedCoeffsListx(%struct.TypHeader*, i64) nounwind

declare %struct.TypHeader* @ShiftedCoeffsVecFFE(%struct.TypHeader*, i64) nounwind

declare void @AddCoeffsListxVecFFE(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare void @AddCoeffsVecFFEVecFFE(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare i64 @MultiplyCoeffsVecFFEVecFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture, i64, %struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @ProductCoeffsListxListx(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProductCoeffsVecFFEVecFFE(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProductCoeffsModListxListx(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare i64 @ReduceCoeffsListxListx(%struct.TypHeader*, i64, %struct.TypHeader*, i64) nounwind

declare i64 @ReduceCoeffsVecFFEVecFFE(%struct.TypHeader* nocapture, i64, %struct.TypHeader* nocapture, i64) nounwind

declare i64 @ReduceCoeffsModListxListx(%struct.TypHeader*, i64, %struct.TypHeader*, i64, %struct.TypHeader*) nounwind

declare i64 @ReduceCoeffsModListx(%struct.TypHeader*, i64, %struct.TypHeader* nocapture, i64, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowerModListxIntListx(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowerModVecFFEIntVecFFE(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowerModListxLIntListx(%struct.TypHeader*, %struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowerModVecFFELIntVecFFE(%struct.TypHeader*, %struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunShiftedCoeffs(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunNormalizeCoeffs(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunShrinkCoeffs(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunAddCoeffs(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSumCoeffs(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunProductCoeffs(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunProductCoeffsMod(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunReduceCoeffs(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunRemainderCoeffs(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunReduceCoeffsMod(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunPowerModCoeffs(%struct.TypHeader* nocapture) nounwind

declare i64 @LenRange(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @ElmfRange(%struct.TypHeader* nocapture, i64) nounwind readonly

declare i64 @PosRange(%struct.TypHeader* nocapture, %struct.TypHeader*, i64) nounwind

declare i64 @IsDenseRange(%struct.TypHeader* nocapture) nounwind readnone

declare i64 @IsPossRange(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @LtRange(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @ElmRange(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @ElmsRange(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AssRange(%struct.TypHeader*, i64, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AsssRange(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare void @PlainRange(%struct.TypHeader*) nounwind

declare void @PrRange(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvMakeRange(%struct.TypHeader* nocapture) nounwind

declare void @PrMakeRange(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsRange(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvRat(%struct.TypHeader*) nounwind readnone

declare void @PrRat(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @SumRat(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffRat(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdRat(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @QuoRat(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ModRat(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowRat(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @EqRat(%struct.TypHeader*, %struct.TypHeader*) nounwind readonly

declare %struct.TypHeader* @LtRat(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunIsRat(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunNumerator(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDenominator(%struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @RdExpr(i64) nounwind

declare fastcc %struct.TypHeader* @RdAtom(i64) nounwind

declare fastcc %struct.TypHeader* @RdFactor(i64) nounwind

declare fastcc %struct.TypHeader* @RdTerm(i64) nounwind

declare fastcc %struct.TypHeader* @RdAri(i64) nounwind

declare fastcc %struct.TypHeader* @RdRel(i64) nounwind

declare fastcc %struct.TypHeader* @RdStats(i64) nounwind

declare fastcc %struct.TypHeader* @RdStat(i64) nounwind

declare %struct.TypHeader* @EvRec(%struct.TypHeader*) nounwind readnone

declare %struct.TypHeader* @EqRec(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @EvMakeRec(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvRecElm(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvRecAss(%struct.TypHeader* nocapture) nounwind

declare void @PrRec(%struct.TypHeader*) nounwind

declare void @PrRecElm(%struct.TypHeader* nocapture) nounwind

declare void @PrRecAss(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @SumRec(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffRec(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdRec(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @QuoRec(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ModRec(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowRec(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @LtRec(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @CommRec(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunIsRec(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunRecFields(%struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @MakeRec(%struct.TypHeader*, i64, %struct.TypHeader* nocapture) nounwind

declare fastcc i64 @OpenInput(i8* nocapture) nounwind

declare fastcc void @PutLine() nounwind

declare fastcc void @PutChr(i32 signext) nounwind

declare fastcc void @Pr(i8* nocapture, i64, i64) nounwind

declare fastcc void @SyntaxError(i8*) nounwind

declare fastcc void @GetLine() nounwind

declare fastcc void @GetIdent() nounwind

declare fastcc void @GetSymbol() nounwind

declare fastcc void @Match(i64, i8* nocapture, i64) nounwind

declare i64 @LenSet(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @ElmfSet(%struct.TypHeader* nocapture, i64) nounwind readonly

declare i64 @PosSet(%struct.TypHeader* nocapture, %struct.TypHeader*, i64) nounwind

declare void @PlainSet(%struct.TypHeader* nocapture) nounwind readnone

declare i64 @IsDenseSet(%struct.TypHeader* nocapture) nounwind readnone

declare i64 @IsPossSet(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @EqSet(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @LtSet(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ElmSet(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @ElmsSet(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AssSet(%struct.TypHeader*, i64, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AsssSet(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunSet(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsSet(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsEqualSet(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsSubsetSet(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunAddSet(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunRemoveSet(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunUniteSet(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIntersectSet(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSubtractSet(%struct.TypHeader* nocapture) nounwind

declare fastcc i64 @IsSet(%struct.TypHeader*) nounwind

declare fastcc %struct.TypHeader* @SetList(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @EvStatseq(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvIf(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvFor(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvWhile(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvRepeat(%struct.TypHeader* nocapture) nounwind

declare void @PrStatseq(%struct.TypHeader* nocapture) nounwind

declare void @PrIf(%struct.TypHeader* nocapture) nounwind

declare void @PrFor(%struct.TypHeader* nocapture) nounwind

declare void @PrWhile(%struct.TypHeader* nocapture) nounwind

declare void @PrRepeat(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvChar(%struct.TypHeader*) nounwind readnone

declare %struct.TypHeader* @EqChar(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @LtChar(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare i64 @LenString(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @ElmfString(%struct.TypHeader* nocapture, i64) nounwind readonly

declare i64 @PosString(%struct.TypHeader*, %struct.TypHeader*, i64) nounwind

declare i64 @IsDenseString(%struct.TypHeader* nocapture) nounwind readnone

declare i64 @IsPossString(%struct.TypHeader* nocapture) nounwind readonly

declare void @PrChar(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ElmString(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @ElmsString(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AssString(%struct.TypHeader*, i64, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AsssString(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare void @PlainString(%struct.TypHeader*) nounwind

declare void @PrString(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EqString(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @LtString(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @FunIsString(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvMakeString(%struct.TypHeader* nocapture) nounwind

declare fastcc i64 @IsString(%struct.TypHeader*) nounwind

declare noalias i8* @calloc(i64, i64) nounwind

declare i8* @tmpnam(i8*) nounwind

declare fastcc void @SyExit(i64) noreturn nounwind

declare void @exit(i32) noreturn nounwind

declare i64 @times(%struct.tms* nocapture) nounwind

declare i64 @fwrite(i8* nocapture, i64, i64, i8* nocapture) nounwind

declare void @syAnswerIntr(i32) nounwind

declare i64 @time(i64*) nounwind

declare void (i32)* @signal(i32, void (i32)*) nounwind

declare fastcc void @syEchoch(i32, i64) nounwind

declare i32 @fileno(%struct._IO_FILE* nocapture) nounwind

declare i64 @write(i32, i8* nocapture, i64)

declare fastcc void @syStopraw(i64) nounwind

declare i32 @ioctl(i32, i64, ...) nounwind

declare void @syAnswerTstp(i32) nounwind

declare i32 @getpid() nounwind

declare i32 @kill(i32, i32) nounwind

declare fastcc void @SyFclose(i64) nounwind

declare i32 @fclose(%struct._IO_FILE* nocapture) nounwind

declare i8* @strncat(i8*, i8* nocapture, i64) nounwind

declare i32 @strcmp(i8* nocapture, i8* nocapture) nounwind readonly

declare fastcc i64 @SyFopen(i8* nocapture, i8* nocapture) nounwind

declare noalias %struct._IO_FILE* @fopen(i8* noalias nocapture, i8* noalias nocapture) nounwind

declare void @setbuf(%struct._IO_FILE* noalias nocapture, i8* noalias) nounwind

declare i64 @strlen(i8* nocapture) nounwind readonly

declare fastcc void @syWinPut(i64, i8* nocapture, i8* nocapture) nounwind

declare i32 @isatty(i32) nounwind

declare i8* @ttyname(i32) nounwind

declare i32 @fputs(i8* noalias nocapture, %struct._IO_FILE* noalias nocapture) nounwind

declare i32 @atoi(i8* nocapture) nounwind readonly

declare noalias i8* @malloc(i64) nounwind

declare void @free(i8* nocapture) nounwind

declare i8* @getenv(i8* nocapture) nounwind readonly

declare i32 @system(i8* nocapture)

declare i64 @read(i32, i8* nocapture, i64)

declare fastcc void @SyFputs(i8* nocapture, i64) nounwind

declare fastcc i32 @syGetch(i64) nounwind

declare fastcc i32 @syStartraw(i64) nounwind

declare void @llvm.memcpy.i64(i8* nocapture, i8* nocapture, i64, i32) nounwind

declare void @syAnswerCont(i32) nounwind

declare fastcc void @syEchos(i8* nocapture, i64) nounwind

declare fastcc i8* @SyFgets(i8*, i64) nounwind

declare i8* @fgets(i8* noalias, i32, %struct._IO_FILE* noalias nocapture) nounwind

declare fastcc void @SyHelp(i8* nocapture, i64) nounwind

declare %struct.TypHeader* @FunTzRelator(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTzWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTzSortC(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTzRenumberGens(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTzReplaceGens(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTzSubstituteGen(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTzOccurrences(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTzOccurrencesPairs(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunTzSearchC(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvUnknown(%struct.TypHeader*) nounwind readnone

declare %struct.TypHeader* @EqUnknown(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @LtUnknown(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare void @PrUnknown(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @SumUnknown(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffUnknown(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdUnknown(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @QuoUnknown(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowUnknown(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunUnknown(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsUnknown(%struct.TypHeader* nocapture) nounwind

declare i64 @LenVecFFE(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @ElmlVecFFE(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @ElmrVecFFE(%struct.TypHeader* nocapture, i64) nounwind

declare i64 @PosVecFFE(%struct.TypHeader*, %struct.TypHeader*, i64) nounwind

declare i64 @IsDenseVecFFE(%struct.TypHeader* nocapture) nounwind readnone

declare i64 @IsPossVecFFE(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @DepthVecFFE(%struct.TypHeader* nocapture) nounwind readonly

declare fastcc i64 @DegreeVecFFE(%struct.TypHeader* nocapture) nounwind readonly

declare fastcc i64 @DegreeMatFFE(%struct.TypHeader** nocapture) nounwind readonly

declare %struct.TypHeader* @ElmfVecFFE(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @ElmVecFFE(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @ElmsVecFFE(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AssVecFFE(%struct.TypHeader*, i64, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AsssVecFFE(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare void @PlainVecFFE(%struct.TypHeader*) nounwind

declare i64 @IsXTypeVecFFE(%struct.TypHeader*) nounwind

declare i64 @IsXTypeMatFFE(%struct.TypHeader*) nounwind

declare void @PrVecFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @SumFFEVecFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @SumVecFFEFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @SumVecFFEVecFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @DiffFFEVecFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @DiffVecFFEFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @DiffVecFFEVecFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdFFEVecFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdVecFFEFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdVecFFEVecFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdVecFFEMatFFE(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @PowMatFFEInt(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunCharFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunDegreeFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunLogVecFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunMakeVecFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunNumberVecFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @CantIntVecFFE(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @FunIntVecFFE(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdFFEVector(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdVectorFFE(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffFFEVector(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffVectorFFE(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @SumFFEVector(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @SumVectorFFE(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @IntVecFFE(%struct.TypHeader*, i64) nounwind

declare i64 @LenVector(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @ElmfVector(%struct.TypHeader* nocapture, i64) nounwind readonly

declare i64 @PosVector(%struct.TypHeader* nocapture, %struct.TypHeader*, i64) nounwind

declare void @PlainVector(%struct.TypHeader* nocapture) nounwind readnone

declare i64 @IsDenseVector(%struct.TypHeader* nocapture) nounwind readnone

declare i64 @IsPossVector(%struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @EqVector(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @LtVector(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ElmVector(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @ElmsVector(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AssVector(%struct.TypHeader*, i64, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @AsssVector(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare i64 @IsXTypeVector(%struct.TypHeader*) nounwind

declare i64 @IsXTypeMatrix(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @SumIntVector(%struct.TypHeader*, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @SumVectorInt(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @SumVectorVector(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @DiffIntVector(%struct.TypHeader*, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @DiffVectorInt(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffVectorVector(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdIntVector(%struct.TypHeader*, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdVectorInt(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdVectorVector(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdVectorMatrix(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @PowMatrixInt(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare fastcc %struct.TypHeader* @SwordWord(%struct.TypHeader*, %struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @EvWord(%struct.TypHeader*) nounwind readnone

declare %struct.TypHeader* @EqWord(%struct.TypHeader*, %struct.TypHeader*) nounwind readonly

declare fastcc %struct.TypHeader* @WordSword(%struct.TypHeader* nocapture) nounwind

declare void @PrWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @ProdWord(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @QuoWord(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ModWord(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @PowWW(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @CommWord(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @LtWord(%struct.TypHeader*, %struct.TypHeader*) nounwind readonly

declare %struct.TypHeader* @PowWI(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @FunExpsum(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunIsWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunPosWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSubword(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunLenWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunMappedWord(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunEliminated(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunSubs(%struct.TypHeader* nocapture) nounwind

declare fastcc %struct.TypHeader* @Words(%struct.TypHeader* nocapture, i64) nounwind

declare %struct.TypHeader* @FunAbstractGenerators(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @FunAbstractGenerator(%struct.TypHeader* nocapture) nounwind

declare void @PrSword(%struct.TypHeader* nocapture) nounwind

declare %struct.TypHeader* @LtAg_DIRECT(%struct.TypHeader* nocapture, %struct.TypHeader* nocapture) nounwind readonly

declare %struct.TypHeader* @CommAg_DIRECT(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdCyc_DIRECT(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @QuoCyc_DIRECT(%struct.TypHeader*) nounwind

declare void @AddCoeffsListxListx_DIRECT(%struct.TypHeader*, %struct.TypHeader*, %struct.TypHeader*) nounwind

declare i64 @PosRange_DIRECT(%struct.TypHeader* nocapture, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdCycI_DIRECT(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare void @PrFunction_DIRECT(%struct.TypHeader* nocapture) nounwind

declare void @FunPrint_DIRECT(%struct.TypHeader* nocapture) nounwind

declare void @FunBacktrace_DIRECT() nounwind

declare %struct.TypHeader* @SumSclList_DIRECT(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @SumListScl_DIRECT(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffSclList_DIRECT(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @DiffListScl_DIRECT(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdSclList_DIRECT(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare %struct.TypHeader* @ProdListScl_DIRECT(%struct.TypHeader*, %struct.TypHeader*) nounwind

declare i64 @IsXTypeVector_DIRECT(%struct.TypHeader*) nounwind

declare %struct.TypHeader* @EqWord_DIRECT(%struct.TypHeader*, %struct.TypHeader*) nounwind readonly

declare %struct.TypHeader* @ProdWord_DIRECT(%struct.TypHeader*, %struct.TypHeader*) nounwind

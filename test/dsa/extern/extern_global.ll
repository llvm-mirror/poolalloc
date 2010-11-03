; ModuleID = 'extern_global.c'
;This tests how DSA handles external globals and their completeness in TD.

; Externally visible globals should be marked external, but complete
;RUN: dsaopt %s -dsa-td -analyze -verify-flags globalInt+GE-I
; (When linkage is not specified, it is external)
;RUN: dsaopt %s -dsa-td -analyze -verify-flags normalGlobal+GE-I
; Externally visible global and what it points to should be complete/external
;RUN: dsaopt %s -dsa-td -analyze -verify-flags globalIntPtr+GE-I
;RUN: dsaopt %s -dsa-td -analyze -verify-flags globalIntPtr:0+GE-I
; Externally visible global and what it points to should be complete/external
; this time with a struct and point to some stack memory...
;RUN: dsaopt %s -dsa-td -analyze -verify-flags globalStructWithPointers+GE-I
;RUN: dsaopt %s -dsa-td -analyze -verify-flags globalStructWithPointers:8+E-I
;RUN: dsaopt %s -dsa-td -analyze -verify-flags globalStructWithPointers:8:8+E-I
;RUN: dsaopt %s -dsa-td -analyze -verify-flags main:s+ES-I
; Globals that aren't marked 'external' shouldn't be incomplete (or external)
;RUN: dsaopt %s -dsa-td -analyze -verify-flags internalGlobal+G-IE
; Check some edges of the graph
;RUN: dsaopt %s -dsa-td -analyze -check-same-node=normalGlobal:0,globalIntPtr
;RUN: dsaopt %s -dsa-td -analyze -check-same-node=internalGlobal:0,normalGlobal
;RUN: dsaopt %s -dsa-td -analyze -check-same-node=globalStructWithPointers:8,globalStructWithPointers:8:8

target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.myStruct = type { i32, %struct.myStruct* }

@globalStructWithPointers = external global %struct.myStruct
@globalIntPtr = external global i32*
@globalInt = external global i32
@normalGlobal = global i32** null
@internalGlobal = internal global i32*** null

define i32 @main() nounwind {
entry:
;Create the struct and initialize it
  %s = alloca %struct.myStruct, align 8           ; <%struct.myStruct*> [#uses=1]
  %0 = getelementptr inbounds %struct.myStruct* %s, i32 0, i32 1 ; <%struct.myStruct**> [#uses=1]
  store %struct.myStruct* %s, %struct.myStruct** %0, align 8
  %1 = getelementptr inbounds %struct.myStruct* %s, i32 0, i32 0 ; <i32*> [#uses=1]
  store i32 100, i32* %1, align 8
;Store the struct into the global
  store %struct.myStruct* %s, %struct.myStruct** getelementptr inbounds (%struct.myStruct* @globalStructWithPointers, i64 0, i32 1), align 8
;Play around with the rest of the globals
  store i32* @globalInt, i32** @globalIntPtr, align 8
  store i32** @globalIntPtr, i32*** @normalGlobal, align 8
  store i32*** @normalGlobal, i32**** @internalGlobal, align 8
  ret i32 undef
}

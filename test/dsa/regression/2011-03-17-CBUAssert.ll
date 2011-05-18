;RUN: dsaopt %s -dsa-cbu -disable-output
; ModuleID = 'bugpoint-reduced-simplified.bc'
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64"
target triple = "x86_64-unknown-linux-gnu"

%struct.anon = type { %struct.node*, %struct.node* }
%struct.bitvect_t = type { i32*, i32, i32 }
%struct.node = type { i32, %union.anon, %struct.bitvect_t*, %struct.bitvect_t*, i32 }
%struct.regmatcher = type { [128 x i32], i32, i32**, i32* }
%union.anon = type { %struct.anon }

define %struct.regmatcher* @compile(i8* %expr) nounwind {
  entry:
    %0 = call %struct.node* (...)* bitcast (%struct.node* ()* @end_node to %struct.node* (...)*)() nounwind ; <%struct.node*> [#uses=0]
      unreachable
}

define %struct.node* @end_node() nounwind {
  entry:
    ret %struct.node* undef
}


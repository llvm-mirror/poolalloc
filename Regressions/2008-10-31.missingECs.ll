; ModuleID = '2008-10-31.missingECs.bc'
target datalayout = "e-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:32:64-v64:64:64-v128:128:128-a0:0:64-f80:128:128"
target triple = "i386-apple-darwin9.1"
	%struct.Index_Map = type { i32, %struct.item_set** }
	%struct.Item = type { [4 x i16], %struct.rule* }
	%struct.anon = type { %struct.nonterminal* }
	%struct.dimension = type { i16*, %struct.Index_Map, %struct.mapping*, i32, %struct.plankMap* }
	%struct.intlist = type { i32, %struct.intlist* }
	%struct.item_set = type { i32, i32, %struct.operator*, [2 x %struct.item_set*], %struct.item_set*, i16*, %struct.Item*, %struct.Item* }
	%struct.list = type { i8*, %struct.list* }
	%struct.mapping = type { %struct.list**, i32, i32, i32, %struct.item_set** }
	%struct.nonterminal = type { i8*, i32, i32, i32, %struct.plankMap*, %struct.rule* }
	%struct.operator = type { i8*, i8, i32, i32, i32, i32, %struct.table* }
	%struct.pattern = type { %struct.nonterminal*, %struct.operator*, [2 x %struct.nonterminal*] }
	%struct.patternAST = type { %struct.symbol*, i8*, %struct.list* }
	%struct.plank = type { i8*, %struct.list*, i32 }
	%struct.plankMap = type { %struct.list*, i32, %struct.stateMap* }
	%struct.rule = type { [4 x i16], i32, i32, i32, %struct.nonterminal*, %struct.pattern*, i8 }
	%struct.ruleAST = type { i8*, %struct.patternAST*, i32, %struct.intlist*, %struct.rule*, %struct.strTableElement*, %struct.strTableElement* }
	%struct.stateMap = type { i8*, %struct.plank*, i32, i16* }
	%struct.strTableElement = type { i8*, %struct.intlist*, i8* }
	%struct.symbol = type { i8*, i32, %struct.anon }
	%struct.table = type { %struct.operator*, %struct.list*, i16*, [2 x %struct.dimension*], %struct.item_set** }

declare void @doEnterNonTerm(%struct.ruleAST*) nounwind

declare void @doRule(%struct.ruleAST*) nounwind

define fastcc void @reveachList(i8* (i8*)* %f, %struct.list* %l) nounwind {
entry:
	unreachable
}

define void @main(i32 %argc, i8** %argv) noreturn nounwind {
entry:
	br i1 false, label %bb.i9.i.i, label %bb.i12.i.i

bb.i12.i.i:		; preds = %entry
	call fastcc void @reveachList(i8* (i8*)* bitcast (void (%struct.ruleAST*)* @doEnterNonTerm to i8* (i8*)*), %struct.list* null) nounwind
	unreachable

bb.i9.i.i:		; preds = %entry
	call fastcc void @reveachList(i8* (i8*)* bitcast (void (%struct.ruleAST*)* @doRule to i8* (i8*)*), %struct.list* null) nounwind
	unreachable
}

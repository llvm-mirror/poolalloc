; ModuleID = 'bugpoint-reduced-simplified.bc'
target endian = little
target pointersize = 64
target triple = "alphaev6-unknown-linux-gnu"
deplibs = [ "c", "crtend" ]
	%struct..TorRec = type { int, void ()* }
	%struct.CON_list_struct = type { %struct.CON_list_struct*, %struct.CON_node_struct* }
	%struct.CON_node_struct = type { %struct.DIS_list_struct*, %struct.DIS_list_struct*, int }
	%struct.Connector_struct = type { short, short, sbyte, sbyte, %struct.Connector_struct*, sbyte* }
	%struct.DIS_list_struct = type { %struct.DIS_list_struct*, %struct.DIS_node_struct* }
	%struct.DIS_node_struct = type { %struct.CON_list_struct*, %struct.List_o_links_struct*, int }
	%struct.D_type_list_struct = type { %struct.D_type_list_struct*, int }
	%struct.Dict_node_struct = type { sbyte*, %struct.Word_file_struct*, %struct.Exp_struct*, %struct.Dict_node_struct*, %struct.Dict_node_struct* }
	%struct.Disjunct_struct = type { %struct.Disjunct_struct*, short, sbyte, sbyte*, %struct.Connector_struct*, %struct.Connector_struct* }
	%struct.E_list_struct = type { %struct.E_list_struct*, %struct.Exp_struct* }
	%struct.Exp_struct = type { sbyte, ubyte, sbyte, sbyte, { sbyte* } }
	%struct.Image_node_struct = type { %struct.Image_node_struct*, %struct.Connector_struct*, int }
	%struct.Link_struct = type { int, int, %struct.Connector_struct*, %struct.Connector_struct*, sbyte* }
	%struct.Linkage_info_struct = type { int, short, short, short, short, short }
	%struct.Links_to_patch_struct = type { %struct.Links_to_patch_struct*, int, sbyte, int }
	%struct.List_o_links_struct = type { int, int, int, %struct.List_o_links_struct* }
	%struct.Match_node_struct = type { %struct.Match_node_struct*, %struct.Disjunct_struct* }
	%struct.PP_node_struct = type { %struct.D_type_list_struct**, %struct.Violation_list_struct* }
	%struct.Table_connector = type { short, short, %struct.Connector_struct*, %struct.Connector_struct*, short, int, %struct.Table_connector* }
	%struct.Tconnector_struct = type { sbyte, sbyte, %struct.Tconnector_struct*, sbyte* }
	%struct.TorRec = type { int, void ()* }
	%struct.Violation_list_struct = type { %struct.Violation_list_struct*, sbyte* }
	%struct.Word_file_struct = type { [60 x sbyte], int, %struct.Word_file_struct* }
	%struct.Word_struct = type { [60 x sbyte], %struct.X_node_struct*, %struct.Disjunct_struct* }
	%struct.X_node_struct = type { sbyte*, %struct.Exp_struct*, %struct.X_node_struct* }
	%struct._IO_FILE = type { int, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, %struct._IO_marker*, %struct._IO_FILE*, int, int, long, ushort, sbyte, [1 x sbyte], sbyte*, long, sbyte*, sbyte*, int, [44 x sbyte] }
	%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, int }
	%struct.__va_list_tag = type { sbyte*, int }
	%struct.c_list_struct = type { %struct.Connector_struct*, int, %struct.c_list_struct* }
	%struct.clause_struct = type { %struct.clause_struct*, int, int, %struct.Tconnector_struct* }
	%struct.d_tree_leaf_struct = type { %struct.domain_struct*, int, %struct.d_tree_leaf_struct* }
	%struct.domain_struct = type { sbyte*, int, %struct.List_o_links_struct*, int, int, %struct.d_tree_leaf_struct*, %struct.domain_struct* }
	%struct.label_node_struct = type { int, %struct.label_node_struct* }
	%struct.patch_element_struct = type { sbyte, sbyte, int, int }
	%struct.string_node_struct = type { sbyte*, int, %struct.string_node_struct* }
%sentence = external global [250 x %struct.Word_struct]		; <[250 x %struct.Word_struct]*> [#uses=2]
%mn_free_list = external global %struct.Match_node_struct*              ; <%struct.Match_node_struct**> [#uses=1]

implementation   ; Functions:

declare fastcc %struct.Match_node_struct* %form_match_list(%struct.Connector_struct*) 

internal fastcc void %prune() {
entry:
	%tmp.15 = getelementptr [250 x %struct.Word_struct]* %sentence, long 0, int 0, uint 2		; <%struct.Disjunct_struct**> [#uses=1]
	%tmp.16 = load %struct.Disjunct_struct** %tmp.15		; <%struct.Disjunct_struct*> [#uses=1]
	%tmp.44 = getelementptr %struct.Disjunct_struct* %tmp.16, long 0, uint 5		; <%struct.Connector_struct**> [#uses=1]
	%tmp.45 = load %struct.Connector_struct** %tmp.44		; <%struct.Connector_struct*> [#uses=1]
	free %struct.Connector_struct* %tmp.45
	ret void
}

int %main(int %argc, sbyte** %argv) {
entry:
	%tmp.2 = load %struct.Disjunct_struct** getelementptr ([250 x %struct.Word_struct]* %sentence, long 0, long 0, uint 2)		; <%struct.Disjunct_struct*> [#uses=1]
	%tmp.16 = getelementptr %struct.Disjunct_struct* %tmp.2, long 0, uint 5		; <%struct.Connector_struct**> [#uses=1]
	%tmp.17 = load %struct.Connector_struct** %tmp.16		; <%struct.Connector_struct*> [#uses=1]
	%tmp.13 = tail call fastcc int %count( %struct.Connector_struct* %tmp.17 )		; <int> [#uses=0]
	%tmp.24 = tail call fastcc int %count( %struct.Connector_struct* null )		; <int> [#uses=0]
	ret int 0
}

internal fastcc int %count(%struct.Connector_struct* %le) {
no_exit.0:		; preds = %endif.4
	%tmp.101 = tail call fastcc %struct.Match_node_struct* %form_match_list( %struct.Connector_struct* %le)
	br label %no_exit.1

no_exit.1:		; preds = %loopexit.2, %no_exit.0
	%m.2.0 = phi %struct.Match_node_struct* [ %tmp.101, %no_exit.0 ], [ %tmp.542, %loopexit.2 ]		; <%struct.Match_node_struct*> [#uses=2]
	%tmp.112 = getelementptr %struct.Match_node_struct* %m.2.0, long 0, uint 1		; <%struct.Disjunct_struct**> [#uses=1]
	%tmp.113 = load %struct.Disjunct_struct** %tmp.112		; <%struct.Disjunct_struct*> [#uses=1]
	br bool false, label %loopexit.2, label %no_exit.2.preheader

no_exit.2.preheader:		; preds = %no_exit.1
	%tmp.141 = getelementptr %struct.Disjunct_struct* %tmp.113, long 0, uint 5		; <%struct.Connector_struct**> [#uses=1]
	%tmp.432 = load %struct.Connector_struct** %tmp.141		; <%struct.Connector_struct*> [#uses=1]
	%tmp.427 = tail call fastcc int %count( %struct.Connector_struct* %tmp.432 )		; <int> [#uses=0]
	ret int 0

loopexit.2:		; preds = %no_exit.1
	%tmp.541 = getelementptr %struct.Match_node_struct* %m.2.0, long 0, uint 0		; <%struct.Match_node_struct**> [#uses=1]
	%tmp.542 = load %struct.Match_node_struct** %tmp.541		; <%struct.Match_node_struct*> [#uses=1]
	br bool false, label %no_exit.i.preheader, label %no_exit.1

no_exit.i.preheader:		; preds = %loopexit.2
	%mn_free_list.promoted = load %struct.Match_node_struct** %mn_free_list		; <%struct.Match_node_struct*> [#uses=1]
	%tmp.5.i = getelementptr %struct.Match_node_struct* %tmp.101, long 0, uint 0		; <%struct.Match_node_struct**> [#uses=1]
	store %struct.Match_node_struct* %mn_free_list.promoted, %struct.Match_node_struct** %tmp.5.i
	ret int 0
}


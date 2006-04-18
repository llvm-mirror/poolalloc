; ModuleID = 'bugpoint-reduced-simplified.bc'
target endian = little
target pointersize = 32
target triple = "i686-pc-linux-gnu"
deplibs = [ "c", "crtend" ]
	%struct.MT = type { int, [100 x [3 x [3 x int]]], [3 x [3 x int]], [3 x [3 x int]] }
	%struct._IO_FILE = type { int, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, sbyte*, %struct._IO_marker*, %struct._IO_FILE*, int, int, int, ushort, sbyte, [1 x sbyte], sbyte*, long, sbyte*, sbyte*, int, [52 x sbyte] }
	%struct._IO_marker = type { %struct._IO_marker*, %struct._IO_FILE*, int }
	%struct.cellbox = type { sbyte*, sbyte, sbyte, int, int, short, short, short, short, short, short, short, %struct.tilebox* }
	%struct.netbox = type { %struct.netbox*, int, int, int, int, int, int, short, short, short, sbyte, sbyte, sbyte, sbyte }
	%struct.termbox = type { %struct.termbox*, %struct.netbox*, int, int, short, [2 x short], [2 x short], short }
	%struct.tilebox = type { short, short, short, short, %struct.termbox* }
%carray = external global %struct.cellbox**		; <%struct.cellbox***> [#uses=1]
%foo = external global sbyte*

implementation   ; Functions:

void %main() {
entry:
	call fastcc void %readcell( )
	ret void
}

;fastcc sbyte* %safe_malloc(uint %size) {
;entry:
;	%tmp.0 = malloc sbyte, uint %size		; <sbyte*> [#uses=1]
;	ret sbyte* %tmp.0
;}

fastcc void %readcell() {
entry:
	%input = alloca [1024 x sbyte]		; <[1024 x sbyte]*> [#uses=1]
	%tmp.48384 = getelementptr [1024 x sbyte]* %input, int 0, int 0		; <sbyte*> [#uses=1]
;	%tmp.7314 = call fastcc sbyte* %safe_malloc( uint 0 )		; <sbyte*> [#uses=2]
        %tmp.7314 = malloc sbyte, uint 0	
        store sbyte* %tmp.7314, sbyte** %foo
	call void %llvm.memcpy.i32( sbyte* %tmp.7314, sbyte* %tmp.48384, uint 0, uint 1 )
	ret void
}

declare void %llvm.memcpy.i32(sbyte*, sbyte*, uint, uint)

; pointer compression tries to turn the llvm.memcpy into llvm.memcpy_pc
; this is of course bad.  It should materialize the pointers.
target endian = little
target pointersize = 64
target triple = "alphaev6-unknown-linux-gnu"
deplibs = [ "c", "crtend" ]
	%struct.spec_fd_t = type { int, int, int, ubyte* }
%spec_fd = external global [3 x %struct.spec_fd_t]		; <[3 x %struct.spec_fd_t]*> [#uses=2]

implementation   ; Functions:

declare void %llvm.memcpy(sbyte*, sbyte*, ulong, uint)

void %main() {
entry:
	br bool false, label %no_exit.0.i161, label %endif.0

endif.0:		; preds = %entry
	ret void

no_exit.0.i161:		; preds = %entry
	%tmp.25.i = getelementptr [3 x %struct.spec_fd_t]* %spec_fd, long 0, int 0, uint 3		; <ubyte**> [#uses=1]
	%tmp.26.i = malloc ubyte, uint 0		; <ubyte*> [#uses=1]
	store ubyte* %tmp.26.i, ubyte** %tmp.25.i
	br bool false, label %no_exit.1.i, label %then.4

no_exit.1.i:		; preds = %no_exit.0.i161
	%tmp.103.i = load ubyte** getelementptr ([3 x %struct.spec_fd_t]* %spec_fd, long 0, int 0, uint 3)		; <ubyte*> [#uses=1]
	%tmp.118.i = cast ubyte* %tmp.103.i to sbyte*		; <sbyte*> [#uses=1]
	tail call void %llvm.memcpy( sbyte* null, sbyte* %tmp.118.i, ulong 0, uint 1 )
	ret void

then.4:		; preds = %no_exit.0.i161
	ret void
}

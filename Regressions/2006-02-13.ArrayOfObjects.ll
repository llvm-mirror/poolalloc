; This causes a segfault in pointer compression
; The pool type is struct.DLL, but the result of malloc is an array of struct.DLL
; the GEP rewrite assumed that a pointer would have the pool type which isn't true here

target endian = little
target pointersize = 64
target triple = "alphaev6-unknown-linux-gnu"
deplibs = [ "c", "crtend" ]
	%struct.DLL = type { int, %struct.DLL*, %struct.DLL* }

implementation   ; Functions:

void %main() {
entry:
	%tmp.12.i.i = malloc [101 x %struct.DLL]		; <[101 x %struct.DLL]*> [#uses=2]
	%tmp.37.i.i = getelementptr [101 x %struct.DLL]* %tmp.12.i.i, int 0, int 0, uint 2		; <%struct.DLL**> [#uses=1]
	%tmp.42.i.i = getelementptr [101 x %struct.DLL]* %tmp.12.i.i, int 0, int 0		; <%struct.DLL*> [#uses=1]
	store %struct.DLL* %tmp.42.i.i, %struct.DLL** %tmp.37.i.i
	unreachable
}

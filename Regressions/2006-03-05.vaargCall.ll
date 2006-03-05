; ModuleID = 'bugpoint-reduced-simplified.bc'
target endian = little
target pointersize = 64
target triple = "alphaev6-unknown-linux-gnu"
deplibs = [ "c", "crtend" ]
	%struct.arg_list = type { int, %struct.arg_list* }
	%typedef.YYSTYPE = type { sbyte*, sbyte, int, %struct.arg_list* }
%yyv = external global [150 x %typedef.YYSTYPE]		; <[150 x %typedef.YYSTYPE]*> [#uses=1]
%.str_11 = external global [26 x sbyte]		; <[26 x sbyte]*> [#uses=0]

implementation   ; Functions:

void %warn(int, ...) {
entry:
	ret void
}

fastcc void %lookup(sbyte* %name) {
entry:
	br bool false, label %then.0, label %no_exit.i

no_exit.i:		; preds = %entry
	ret void

then.0:		; preds = %entry
	tail call void (int, ...)* %warn( int 0, sbyte* %name )
	ret void
}

fastcc void %addbyte() {
entry:
	ret void
}

void %main() {
entry:
	switch int 0, label %switchexit [
		 int -1, label %loopexit
		 int 118, label %label.5
		 int 115, label %label.4
		 int 119, label %label.3
		 int 105, label %label.2
		 int 108, label %label.1
		 int 99, label %label.0
	]

label.0:		; preds = %entry
	ret void

label.1:		; preds = %entry
	ret void

label.2:		; preds = %entry
	ret void

label.3:		; preds = %entry
	ret void

label.4:		; preds = %entry
	ret void

label.5:		; preds = %entry
	ret void

switchexit:		; preds = %entry
	ret void

loopexit:		; preds = %entry
	%tmp.11.i = getelementptr %typedef.YYSTYPE* getelementptr ([150 x %typedef.YYSTYPE]* %yyv, long 0, long -1), long 1, uint 0		; <sbyte**> [#uses=2]
	br bool false, label %yydefault.preheader.i, label %endif.1.i2

yystack.i:		; preds = %endif.15.i
	ret void

endif.1.i2:		; preds = %loopexit
	ret void

yydefault.preheader.i:		; preds = %loopexit
	switch short 0, label %endif.12.i [
		 short -2, label %then.7.i
		 short 0, label %then.12.i
	]

then.7.i:		; preds = %yydefault.preheader.i
	ret void

then.12.i:		; preds = %yydefault.preheader.i
	ret void

endif.12.i:		; preds = %yydefault.preheader.i
	br bool false, label %endif.15.i, label %then.15.i

then.15.i:		; preds = %endif.12.i
	ret void

endif.15.i:		; preds = %endif.12.i
	switch int 0, label %yystack.i [
		 int 97, label %label.86.i
		 int 96, label %label.85.i
		 int 95, label %label.84.i
		 int 94, label %label.83.i
		 int 93, label %label.82.i
		 int 92, label %label.81.i
		 int 91, label %label.80.i
		 int 90, label %label.79.i
		 int 89, label %label.78.i
		 int 88, label %label.77.i
		 int 87, label %label.76.i
		 int 86, label %label.75.i
		 int 85, label %label.74.i
		 int 84, label %label.73.i
		 int 83, label %label.72.i
		 int 82, label %label.71.i
		 int 81, label %label.70.i
		 int 80, label %label.69.i
		 int 79, label %label.68.i
		 int 78, label %label.67.i
		 int 77, label %label.66.i
		 int 76, label %label.61.i
		 int 75, label %label.60.i
		 int 74, label %label.59.i
		 int 73, label %label.58.i
		 int 72, label %label.57.i
		 int 71, label %label.56.i
		 int 70, label %label.55.i
		 int 69, label %label.54.i
		 int 68, label %label.53.i
		 int 67, label %label.52.i
		 int 65, label %label.51.i
		 int 64, label %label.50.i
		 int 63, label %label.49.i
		 int 62, label %label.48.i
		 int 61, label %label.47.i
		 int 59, label %label.46.i
		 int 54, label %label.41.i
		 int 53, label %label.40.i
		 int 52, label %label.39.i
		 int 50, label %label.38.i
		 int 49, label %else.i796.i
		 int 48, label %label.36.i
		 int 46, label %label.35.i
		 int 44, label %label.34.i
		 int 43, label %label.33.i
		 int 39, label %label.32.i
		 int 38, label %label.31.i
		 int 37, label %label.30.i
		 int 36, label %label.29.i
		 int 35, label %label.28.i
		 int 34, label %label.27.i
		 int 32, label %label.25.i
		 int 31, label %label.24.i
		 int 30, label %label.23.i
		 int 29, label %label.22.i
		 int 28, label %label.21.i
		 int 27, label %label.20.i
		 int 26, label %else.i586.i
		 int 25, label %else.i558.i
		 int 23, label %else.2.i
		 int 22, label %label.15.i
		 int 21, label %label.14.i
		 int 20, label %then.17.i
		 int 19, label %label.12.i
		 int 18, label %label.11.i
		 int 17, label %label.10.i
		 int 10, label %label.9.i
		 int 6, label %label.8.i
		 int 5, label %label.7.i
		 int 4, label %label.6.i
		 int 3, label %label.5.i
		 int 1, label %label.4.i
	]

label.4.i:		; preds = %endif.15.i
	ret void

label.5.i:		; preds = %endif.15.i
	ret void

label.6.i:		; preds = %endif.15.i
	ret void

label.7.i:		; preds = %endif.15.i
	ret void

label.8.i:		; preds = %endif.15.i
	ret void

label.9.i:		; preds = %endif.15.i
	ret void

label.10.i:		; preds = %endif.15.i
	ret void

label.11.i:		; preds = %endif.15.i
	ret void

label.12.i:		; preds = %endif.15.i
	ret void

then.17.i:		; preds = %endif.15.i
	ret void

label.14.i:		; preds = %endif.15.i
	ret void

label.15.i:		; preds = %endif.15.i
	ret void

else.2.i:		; preds = %endif.15.i
	ret void

else.i558.i:		; preds = %endif.15.i
	ret void

else.i586.i:		; preds = %endif.15.i
	ret void

label.20.i:		; preds = %endif.15.i
	ret void

label.21.i:		; preds = %endif.15.i
	ret void

label.22.i:		; preds = %endif.15.i
	ret void

label.23.i:		; preds = %endif.15.i
	ret void

label.24.i:		; preds = %endif.15.i
	ret void

label.25.i:		; preds = %endif.15.i
	ret void

label.27.i:		; preds = %endif.15.i
	ret void

label.28.i:		; preds = %endif.15.i
	ret void

label.29.i:		; preds = %endif.15.i
	ret void

label.30.i:		; preds = %endif.15.i
	ret void

label.31.i:		; preds = %endif.15.i
	ret void

label.32.i:		; preds = %endif.15.i
	ret void

label.33.i:		; preds = %endif.15.i
	br bool false, label %else.i912.i, label %else.i884.i

else.i884.i:		; preds = %label.33.i
	ret void

else.i912.i:		; preds = %label.33.i
	%tmp.4961260.i = load sbyte** %tmp.11.i		; <sbyte*> [#uses=1]
	free sbyte* %tmp.4961260.i
	ret void

label.34.i:		; preds = %endif.15.i
	ret void

label.35.i:		; preds = %endif.15.i
	ret void

label.36.i:		; preds = %endif.15.i
	ret void

else.i796.i:		; preds = %endif.15.i
	ret void

label.38.i:		; preds = %endif.15.i
	ret void

label.39.i:		; preds = %endif.15.i
	ret void

label.40.i:		; preds = %endif.15.i
	ret void

label.41.i:		; preds = %endif.15.i
	ret void

label.46.i:		; preds = %endif.15.i
	ret void

label.47.i:		; preds = %endif.15.i
	ret void

label.48.i:		; preds = %endif.15.i
	ret void

label.49.i:		; preds = %endif.15.i
	ret void

label.50.i:		; preds = %endif.15.i
	ret void

label.51.i:		; preds = %endif.15.i
	ret void

label.52.i:		; preds = %endif.15.i
	ret void

label.53.i:		; preds = %endif.15.i
	ret void

label.54.i:		; preds = %endif.15.i
	ret void

label.55.i:		; preds = %endif.15.i
	ret void

label.56.i:		; preds = %endif.15.i
	ret void

label.57.i:		; preds = %endif.15.i
	ret void

label.58.i:		; preds = %endif.15.i
	ret void

label.59.i:		; preds = %endif.15.i
	ret void

label.60.i:		; preds = %endif.15.i
	ret void

label.61.i:		; preds = %endif.15.i
	ret void

label.66.i:		; preds = %endif.15.i
	ret void

label.67.i:		; preds = %endif.15.i
	ret void

label.68.i:		; preds = %endif.15.i
	ret void

label.69.i:		; preds = %endif.15.i
	ret void

label.70.i:		; preds = %endif.15.i
	ret void

label.71.i:		; preds = %endif.15.i
	ret void

label.72.i:		; preds = %endif.15.i
	ret void

label.73.i:		; preds = %endif.15.i
	ret void

label.74.i:		; preds = %endif.15.i
	ret void

label.75.i:		; preds = %endif.15.i
	ret void

label.76.i:		; preds = %endif.15.i
	ret void

label.77.i:		; preds = %endif.15.i
	ret void

label.78.i:		; preds = %endif.15.i
	ret void

label.79.i:		; preds = %endif.15.i
	ret void

label.80.i:		; preds = %endif.15.i
	ret void

label.81.i:		; preds = %endif.15.i
	%tmp.1165.i = load sbyte** %tmp.11.i		; <sbyte*> [#uses=1]
	call fastcc void %lookup( sbyte* %tmp.1165.i )
	ret void

label.82.i:		; preds = %endif.15.i
	ret void

label.83.i:		; preds = %endif.15.i
	ret void

label.84.i:		; preds = %endif.15.i
	ret void

label.85.i:		; preds = %endif.15.i
	ret void

label.86.i:		; preds = %endif.15.i
	ret void
}

fastcc void %more_functions() {
entry:
	ret void
}

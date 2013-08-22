LATEX=latex

all: manual.pdf

%.pdf: %.ps
	ps2pdf $<

%.ps: %.dvi
	dvips -o $@ $< -t letter

%.dvi: %.tex
	-rm -f *.aux
	$(LATEX) $<

clean:
	rm -f *.aux *.log *.pdf *.ps *.dvi


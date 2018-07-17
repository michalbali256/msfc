GROUP_LAW_TEXS = $(addsuffix .tex,$(basename $(wildcard group_law*.gp)))

all: thesis.pdf

# LaTeX must be run multiple times to get references right
thesis.pdf: thesis.tex $(wildcard *.tex) bibliography.bib
	pdflatex $<
	bibtex thesis
	makeindex thesis.nlo -s nomencl.ist -o thesis.nls
	pdflatex $<
	pdflatex $<



clean:
	del -f *.log *.dvi *.aux *.toc *.lof *.lot *.out *.bbl *.blg $(GROUP_LAW_TEXS)
	del -f thesis.pdf

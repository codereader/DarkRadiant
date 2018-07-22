SUFFIXES = .adoc .html

.adoc.html:
	asciidoctor -a stylesheet=manual.css $^

docdir = $(pkgdatadir)/doc
doc_DATA = manual.html

docimgdir = $(docdir)/img
dist_docimg_DATA = img/*.png

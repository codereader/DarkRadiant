SUFFIXES = .adoc .html

.adoc.html:
	asciidoctor $^

docdir = $(pkgdatadir)/doc
doc_DATA = manual.html

docimgdir = $(docdir)/img
dist_docimg_DATA = img/*.png

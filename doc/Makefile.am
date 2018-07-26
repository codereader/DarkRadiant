SUFFIXES = .adoc .html

.adoc.html:
	asciidoctor -a stylesheet=manual.css $^

html_DATA = manual.html

docimgdir = $(htmldir)/img
dist_docimg_DATA = img/*.png

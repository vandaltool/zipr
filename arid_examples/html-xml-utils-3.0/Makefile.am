# Copyright © 1994-2004 World Wide Web Consortium
# See http://www.w3.org/Consortium/Legal/copyright-software-19980720.html
#
# Author: Bert Bos <bert@w3.org>
# Created: 31 Mar 2000
# Version: $Id: Makefile.am,v 1.25 2004/04/21 15:34:36 bbos Exp $

bin_PROGRAMS =		addid \
			cexport cite count extract htmlclean htmlprune incl\
			index mkbib multitoc normalize num pipe xselect\
			toc uncdata unent unpipe wls xmlns xref\
			xml2asc asc2xml
bin_SCRIPTS = 		cite-mkbib printlinks

man_MANS =		asc2xml.1 cexport.1 cite-mkbib.1 cite.1\
			count.1 htmlclean.1 htmlprune.1 incl.1 index.1\
			mkbib.1 multitoc.1 normalize.1 num.1 pipe.1 toc.1\
			unent.1 unpipe.1 wls.1 xml2asc.1 xmlns.1\
			xref.1

EXTRA_DIST = 		$(man_MANS) dtd.hash unent.hash export.h\
			$(bin_SCRIPTS) $(BUILT_SOURCES)

LDADD =			@LIBOBJS@
AM_YFLAGS =		-d
AM_LFLAGS =		@lex_opt_flags@

BUILT_SOURCES = 	connectsock.e heap.e types.e dtd.e errexit.e\
			tree.e genid.e html.e url.e openurl.e scan.e\
			textwrap.e scan.c html.c html.h dtd.c unent.c\
			class.e selector.e hash.e

asc2xml_SOURCES =	asc2xml.c
addid_SOURCES =		addid.c html.y scan.l dtd.c openurl.c errexit.c\
			url.c connectsock.c heap.c tree.c types.c genid.c\
			class.c hash.c
cexport_SOURCES =	cexport.c
cite_SOURCES =		heap.c errexit.c cite.c
count_SOURCES =		count.c html.y scan.l types.c errexit.c heap.c\
			openurl.c url.c	connectsock.c
extract_SOURCES =	extract.c html.y scan.l openurl.c url.c\
			connectsock.c heap.c errexit.c class.c
htmlclean_SOURCES =	htmlclean.c html.y tree.c types.c heap.c dtd.c\
			scan.l errexit.c
htmlprune_SOURCES =	htmlprune.c tree.c scan.l html.y errexit.c dtd.c\
			heap.c types.c openurl.c url.c connectsock.c class.c
incl_SOURCES =		incl.c scan.l html.y openurl.c url.c heap.c\
			errexit.c connectsock.c types.c
index_SOURCES =		index.c scan.l html.y openurl.c url.c heap.c class.c\
			errexit.c connectsock.c types.c tree.c genid.c dtd.c
mkbib_SOURCES =		errexit.c heap.c mkbib.c hash.c
multitoc_SOURCES =	multitoc.c html.y scan.l openurl.c url.c\
			connectsock.c heap.c errexit.c class.c
normalize_SOURCES =	normalize.c html.y scan.l openurl.c url.c\
			tree.c connectsock.c heap.c dtd.c types.c\
			textwrap.c errexit.c
num_SOURCES =		num.c html.y scan.l openurl.c url.c errexit.c\
			heap.c connectsock.c
pipe_SOURCES =		pipe.c html.y scan.l types.c errexit.c heap.c\
			openurl.c url.c connectsock.c
xselect_SOURCES =	xselect.c types.c errexit.c heap.c html.y scan.l\
			selector.c
toc_SOURCES =		html.y scan.l dtd.c openurl.c errexit.c url.c class.c\
			connectsock.c heap.c tree.c types.c genid.c toc.c\
			hash.c
uncdata_SOURCES =	uncdata.c
unent_SOURCES =		unent.c
unpipe_SOURCES =	unpipe.c heap.c errexit.c openurl.c url.c\
			connectsock.c
wls_SOURCES =		wls.c html.y scan.l openurl.c url.c\
			connectsock.c heap.c errexit.c types.c
xmlns_SOURCES =		xmlns.c html.y scan.l openurl.c url.c\
			connectsock.c heap.c errexit.c types.c
xml2asc_SOURCES =	xml2asc.c
xref_SOURCES =		html.y scan.l dtd.c openurl.c errexit.c url.c\
			connectsock.c heap.c tree.c types.c genid.c xref.c\
			hash.c

HTML_MANS =		$(man_MANS:.1=.html)
CLEANFILES =		$(HTML_MANS)

SUFFIX =		.c:sC .l:sC .y:sC .e:h .h:h
SUFFIXES =		.e

# .c.e: cexport; ./do_export $* -I. $(CPPFLAGS) -c "@CPP@"
# .c.e: cexport; ./cexport -I. $(CPPFLAGS) -c "@CPP@" $<
%.e: %.c cexport
	./cexport -I. $(CPPFLAGS) -c "@CPP@" $<

dtd.c: dtd.hash
	gperf -a -c -C -o -t -p -T -k '1,2,$$' -N lookup_element $< >$@

unent.c: unent.hash
	gperf -a -c -C -o -t -p -k '1,2,$$' -D -N lookup_entity $< >$@

# html.h: html.c
scan.o: html.h scan.c

#property.c: property.hash
#	gperf -a -c -C -o -t -p -k '1,2,12,$$' -N lookup_property $< >$@

# Convenience: publish the tar file, update the online Readme,
# Changlog and man pages
#
%.html: %.1; man2html $<

publish: publish-tar publish-doc

publish-tar: dist
	rsync -e ssh $(distdir).tar.gz ChangeLog README \
	 tux.w3.org:/afs/w3.org/pub/WWW/Tools/HTML-XML-utils/

publish-doc: $(HTML_MANS) Overview.html
	rsync -e ssh $(HTML_MANS) Overview.html \
	 tux.w3.org:/afs/w3.org/pub/WWW/Tools/HTML-XML-utils/man1/

Overview.html: $(HTML_MANS)
	echo "<title>Manual pages</title><ul>" >$@
	for f in $(HTML_MANS); do\
	 echo "<li><a href=\""$$f"\">"`basename $$f .html`"</a>" >>$@;\
	done
	echo "</ul>" >>$@

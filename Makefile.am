## Process this file with automake to produce Makefile.in
## Makefile.am for qtre

srcdir = @srcdir@
VPATH = @srcdir@

bin_PROGRAMS = qtre

dist_sysconf_DATA = qtre.pl

BUILT_SOURCES = qperlxsi.c
CLEANFILES = qperlxsi.c

qtre_SOURCES = \
	action.h \
	action.cpp \
	dialog.h \
	dialog.cpp \
	dirutil.h \
	dirutil.cpp \
	exec.h \
	exec.cpp \
	finder.h \
	finder.cpp \
	history.h \
	history.cpp \
	linkperl.h \
	linkperl.cpp \
	menu.h \
	menu.cpp \
	menuscreen.cpp \
	qperl.h \
	qperl.c \
	qtre.cpp \
	qtreassert.h \
	qtreassert.cpp \
	qtrehelp.cpp \
	qtrever.cpp \
	screen.h \
	screen.cpp \
	screenbase.h \
	screenbase.cpp \
	screenimpl.h \
	screenimpl.cpp \
	select.h \
	select.cpp \
	selectfile.h \
	selectfile.cpp \
	status.h \
	status.cpp \
	termstream.h \
	termstream.cpp \
	text.h \
	text.cpp \
	textline.h \
	textline.cpp \
	textwindow.h \
	textwindow.cpp \
	trace.h \
	trace.cpp \
	util.h

nodist_qtre_SOURCES= qperlxsi.c

# Make qperlxsi depends of almost any change in configuration,
# just in case.
qperlxsi.c: config.h
	$(PERL) -MExtUtils::Embed -e xsinit -- -o qperlxsi.c

AM_CPPFLAGS = -DSYSCONFDIR=\"$(sysconfdir)\"
AM_CFLAGS = $(PERL_CCOPTS)

qtre_LDADD= $(PERL_LDOPTS)


# This is to create and install a package in hp-ux.

depot: qtre
	strip qtre
	swpackage -v -s qtre.psf -d qtre.depot \
		-x target_type=tape

swinstall:
	swinstall -s `pwd`/qtre.depot qtre


EXTRA_DIST = \
	.qtre.pl \
	qtre.psf \
	bootstrap \
	do_conf \
	do_confdeb

# End of Makefile.am

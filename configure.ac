# configure.ac for qtre
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)

AC_INIT(qtre, 0.1.0.0, julian.notfound@gmail.com)

AC_CONFIG_HEADERS([config.h config_config_dir.h config_debug.h config_function.h config_types.h config_version.h config_locale.h config_filebuf.h config_streambuf.h config_curses.h config_dirent.h config_fork.h config_wait.h] )

AM_INIT_AUTOMAKE

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

AC_DEFINE_UNQUOTED(CONFIG_DIR, "$sysconfdir", [Directory for qtre.pl])


#####################################################################
#	Check perl
#####################################################################


# Those checks must be done before AC_PROG_CC, in order to avoid
# autoconf setting the value of CC.

AC_PATH_PROG(PERL, perl, no)
if test "$PERL" = "no"; then
	AC_MSG_ERROR([Perl is required])
fi

# Requirements for perl embedding:

# Check the C compiler used to build perl, but allow user
# to override it.

if test "$CC" = "" ; then
	AC_MSG_CHECKING([for perl Config module])
	$PERL -e 'eval { require Config; }; if ($@) { exit (1); } else { exit (0); }'
	if test "$?" = "0"; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([Perl module Config is required])
	fi
	AC_MSG_CHECKING([for compiler used to build perl])
	CC=`$PERL -MConfig -e 'print $Config{cc}."\n";'`
	AC_MSG_RESULT([$CC])
fi

# Check the availability of the ExtUtils::Embed module,
# required to set compilation flags.
# TODO: add a way for user override.

AC_MSG_CHECKING([for perl ExtUtils::Embed module])
$PERL -e 'eval { require ExtUtils::Embed; }; if ($@) { exit (1); } else { exit (0); }'
if test "$?" = "0"; then
	AC_MSG_RESULT([yes])
else
	AC_MSG_RESULT([no])
	AC_MSG_ERROR([Perl module ExtUtils::Embed is required])
fi

PERL_CCOPTS=`$PERL -MExtUtils::Embed -e ccopts`
AC_SUBST(PERL_CCOPTS)

PERL_LDOPTS=`$PERL -MExtUtils::Embed -e ldopts`
AC_SUBST(PERL_LDOPTS)

#####################################################################


AC_PROG_CC
AC_PROG_CXX

AC_PROG_INSTALL

#####################################################################
#		Options
#####################################################################


AC_ARG_ENABLE(debug, AC_HELP_STRING(
	[--enable-debug], [Debug version [[no]]] ) )

if test "$enable_debug" = "yes" ; then
	AC_MSG_RESULT([Configured for debug version])
else
	AC_MSG_RESULT([Configured for release version])
	AC_DEFINE([NDEBUG], [1], [Not in debug mode])
fi


#####################################################################
# Check for macros used in trace system
#####################################################################


if test "$enable_debug" = "yes" ; then

AC_MSG_CHECKING(wheter the C++ compiler(${CXX}) supports __FUNCTION__)
AC_TRY_COMPILE([#include <stdio.h>],[puts(__FUNCTION__);],
	have_function=yes,have_function=no)
AC_MSG_RESULT($have_function)

if test "$have_function" = "yes" ; then
	AC_DEFINE(HAVE_FUNCTION, [1], [Has __FUNCTION__])
	AC_MSG_CHECKING(wheter the C++ compiler(${CXX}) supports __PRETTY_FUNCTION__)
	AC_TRY_COMPILE([#include <stdio.h>],[puts(__PRETTY_FUNCTION__);],
		have_pretty_function=yes,have_pretty_function=no)
	AC_MSG_RESULT($have_pretty_function)
	if test "$have_pretty_function" = "yes" ; then
		AC_DEFINE(HAVE_PRETTY_FUNCTION, [1], [Has __PRETTY_FUNCTION__])
fi

fi

fi


#####################################################################
#	Check headers and libraries used
#####################################################################


AC_LANG_PUSH([C++])

AC_TYPE_PID_T
AC_TYPE_UID_T
AC_TYPE_MODE_T

AC_HEADER_DIRENT
AC_FUNC_CLOSEDIR_VOID

AC_FUNC_FORK

# This is just to avoid autoscan warn.
AC_FUNC_STAT

AC_CHECK_HEADER([sys/types.h])
AC_HEADER_SYS_WAIT

AC_CHECK_HEADERS([locale.h])
AC_CHECK_HEADERS([nl_types.h])
AC_CHECK_HEADERS([langinfo.h])

AC_CHECK_HEADERS([ostream])

AC_CHECK_HEADERS([streambuf],,
	[AC_CHECK_HEADERS([streambuf.h],,
		AC_MSG_ERROR([streambuf header not available]) ] ) )


#	Find a way to build a stream from a file handle


AC_CHECK_HEADERS([ext/stdio_filebuf.h],[filebuf=yes],[filebuf=no])

if test "$filebuf" = "yes"; then
	AC_MSG_CHECKING([for filebuf constructor parameters])
	AC_TRY_COMPILE(
		[
			#include <ext/stdio_filebuf.h>
		],
		[
			__gnu_cxx::stdio_filebuf <char>
				bufin (1, std::ios::in, size_t (1) );
		],
		filebuf_params=3
		,
		AC_TRY_COMPILE(
			[
				#include <ext/stdio_filebuf.h>
			],
			[
				__gnu_cxx::stdio_filebuf <char>
					bufin (1, std::ios::in, false, 1);
			],
			filebuf_params=4
			,
			AC_MSG_RESULT([unknown])
			AC_MSG_ERROR([unknown filebuf style])
		)
	)
	AC_MSG_RESULT([$filebuf_params])
else
	AC_MSG_CHECKING([for ifstream construction from file handle])
	AC_TRY_COMPILE(
		[
			#include <fstream>
		],
		[
			std::ifstream bufin (1);
		],
		filebuf_params=0
		AC_MSG_RESULT([yes])
		,
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([No way to build a stream from a file handle])
	)
fi

AC_DEFINE_UNQUOTED([FILEBUF_PARAMS], $filebuf_params,
	[Number of parameters for filebuf constructor])


#	Check for ncursesw, if not available check for ncurses


AC_CHECK_HEADERS([ncursesw/ncurses.h],[wncurses=yes],[wncurses=no])
if test "$wncurses" = "yes"; then
	AC_CHECK_LIB([ncursesw],[tputs],,[wncurses=no])
	if test "$wncurses" = "no"; then
		AC_MSG_ERROR([ncursesw header found but no library])
	fi
else
	AC_CHECK_HEADERS([ncurses.h],[ncurses=yes],[ncurses=no])
	if test "$ncurses" = "no"; then
		AC_MSG_ERROR([No ncursesw nor ncurses headers found])
	else
		AC_CHECK_LIB([ncurses],[tputs],,[ncurses=no])
		if test "$ncurses" = no; then
			AC_MSG_ERROR([ncurses header found but no library])
		fi
	fi
fi


AC_LANG_POP


#####################################################################
# Generate files
#####################################################################


AC_CONFIG_FILES([Makefile])

AC_OUTPUT

# End of configure.ac

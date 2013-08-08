/*
	qperl.c
	Revision 30-jan-2006
*/

#include <EXTERN.h>
#include <perl.h>
#include <XSUB.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#include "qperl.h"

#include "config_debug.h"
#include "config_types.h"

/*
This is to generate a dependency to the value of sysconfdir
defined at configure time, to automatically compile this
file when reconfigured and thus avoid mistakes.
But the value of sysconfdir used is the defined at make
time, following gnu guidelines.
*/
#include "config_config_dir.h"


/**********************************************************
		Auxiliar functions and variables.
***********************************************************/

/* Values */

#define MAXPATH 128

#define DIM(a) (sizeof (a) / sizeof (*a) )

/* Perl package names for internal functions */

#define QTRE_PREFIX "qtre::"

static const char qtre_prefix []= QTRE_PREFIX;
static const char qtre_prefix_default []= QTRE_PREFIX "default::";

/* Initialization file names */

/*static const char strGlobalPl []= "/etc/qtre.pl";*/
#define PATH_QTRE_PL SYSCONFDIR "/qtre.pl"
static const char strGlobalPl []= PATH_QTRE_PL;

static const char strPl []= ".qtre.pl";

/* Environment variables used */

static const char envQTRE_NO_INIT []=  "QTRE_NO_INIT";
static const char envQTRE_NO_DIR []=   "QTRE_NO_DIR";
static const char envQTRE_NO_ABORT []= "QTRE_NO_ABORT";
static const char envQTRE_NO_WARN []=  "QTRE_NO_WARN";
static const char envHOME []= "HOME";

/* These functions are defined after gdata */

#ifndef NDEBUG
static void check_inited ();
#else
static void check_inited () { }
#endif

static void check_safe (const char * function);

static int do_callback (qpFuncCode qpfunc, int n, void * p,
	const char * funcname);
static int do_callback_bool (qpFuncCode qpfunc, int n, void * p,
	const char * funcname);
static int do_callback_int (qpFuncCode qpfunc, const char * funcname);

static void initStringParam (qpStringParam * param)
{
	param->str= NULL;
	param->len= 0;
}

static void callback_free_string (qpStringParam * param);

static void errormessage (const char * title, const char * message);

static const char * const qpfuncname []= {
	"qpMode",
	"qpIsEmpty", "qpGetModified", "qpSetModified",
	"qpUp", "qpDown", "qpLeft", "qpRight",
	"qpHome", "qpEnd", "qpPageUp", "qpPageDown",
	"qpGotoLine",
	"qpGetChar", "qpGetWord",
	"qpDeleteChar", "qpBackspace",
	"qpDeleteLine", "qpReplaceLine",
	"qpInsertChar", "qpInsert",

	"qpGetSelection", "qpSelectionLimits",
	"qpSelect", "qpBeginSelect", "qpEndSelect",
	"qpSelectAll", "qpDeleteSelection",

	"qpGetClipboard",
	"qpSetClipboard",
	"qpCut", "qpCopy", "qpPaste",

	"qpLine", "qpNumLines", "qpCharLine",
	"qpGetLine", "qpGetPos", "qpGetCol",
	"qpGetFileExt", "qpGetFileName",
	"qpStatus", "qpStatusQuestion",
	"qpDialogGetString", "qpDialogShowText",
	"qpDefineControl",
	"qpRedraw", "qpQuiet", "qpUpdate",
	"qpGetKey",
	"qpSetAutoindent", "qpGetAutoindent", "qpSetAutocr", "qpGetAutocr",

	"qpMenuAdd", "qpMenuAddOption", "qpMenuRename",

	/* qpWindowNew, qpWindowNext, qpWindowClose, */

	"qpFileLoad", "qpFileNew", "qpFileSave", "qpFileSaveAs",
	"qpFileNext", "qpFileClose", "qpFileQuit",
	"qpHelpAddFunc",
	"qpMacro",
	"qpFreeString",
	"qpCopyNewFile",
	"qpHelp", "qpHelpPerl", "qpHelpAbout",
	"qpCompile", "qpMake", "qpCompileOptions", "qpMan", "qpManOf",

	/* Do not insert new functions after here */

	"qpFailTest"
};

const char * nameqpFunc (qpFuncCode n)
{
	assert (DIM (qpfuncname) == qpLastFuncCode + 1);
	if (n > qpLastFuncCode)
		return "Invalid func code";
	return qpfuncname [n];
}

/* Flag to control if initialisation has been ended
with the function end_init. */

static int init_ended= 0;


static void check_num_args (const char * function, unsigned int numargs,
	unsigned int minargs, unsigned int maxargs)
{
	/*
		Die if the number of arguments is not in the
		margins specified.
	*/

	const char * at_least;
	unsigned int failed;

	if (numargs >= minargs)
	{
		if (maxargs == 0)
			return;
		 if (numargs <= maxargs)
			return;
		failed= maxargs;
		at_least= (minargs < maxargs) ? "no more than " : "";
	}
	else
	{
		failed= minargs;
		at_least= (minargs < maxargs) ? "at least " : "";
	}

	if (minargs == 1)
		croak ("qtre::%s needs %s1 parameter", function, at_least);
	else
		croak ("qtre::%s needs %s%i parameters", function,
			at_least, failed);
}

static void check_num_args_safe (const char * function, unsigned int numargs,
	unsigned int minargs, unsigned int maxargs)
{
	check_num_args (function, numargs, minargs, maxargs);
	check_safe (function);
}

static void check_no_argument (const char * function, unsigned int numargs)
{
	/*
		Die if the function has arguments.
	*/

	if (numargs != 0)
		croak ("qtre::%s does not admit parameters", function);
}

/*
	Memory functions.
*/

static void no_memory (const char * function)
{
	croak ("memory full in qtre::%s", function);
}

static size_t qperl_malloc_counter= 0;

static void * qperl_malloc (size_t size, const char * function)
{
	void * aux;
	aux= malloc (size);
	if (aux == NULL)
		no_memory (function);
	else
		++qperl_malloc_counter;
	return aux;
}

static void * qperl_malloc_nocheck (size_t size)
{
	void * aux;
	aux= malloc (size);
	if (aux != NULL)
		++qperl_malloc_counter;
}

static void qperl_free (void * ptr)
{
	if (ptr != NULL)
	{
		free (ptr);
		--qperl_malloc_counter;
	}
}

void qperl_free_str (char * str)
{
	//free (str);
	qperl_free (str);
}


/**********************************************************
		Predefined qtre perl functions
***********************************************************/


static const char help_mode []=
"Cnages screen mode: 0 normal, 1 inverse";

#define generate_noarg_function(name, qpfunc, helptext) \
	static const char help_##name []= \
	helptext; \
	static XS (name) \
	{ \
		static const char function []= #name; \
		int result; \
		dXSARGS; \
		check_no_argument (function, items); \
		result= do_callback (qpfunc, 0, NULL, function); \
		XSRETURN_IV (result); \
	}

#define generate_noarg_function_safe(name, qpfunc, helptext) \
	static const char help_##name []= \
	helptext; \
	static XS (name) \
	{ \
		static const char function []= #name; \
		int result; \
		qpRetCode r; \
		dXSARGS; \
		check_no_argument (function, items); \
		check_safe (function); \
		r= do_callback (qpfunc, 0, NULL, function); \
		result= (r == qpRetOk) ? 1 : 0; \
		XSRETURN_IV (result); \
	}

#define generate_noarg_function_bool(name, qpfunc, helptext) \
	static const char help_##name []= \
	helptext; \
	static XS (name) \
	{ \
		static const char function []= #name; \
		int result; \
		dXSARGS; \
		check_no_argument (function, items); \
		check_safe (function); \
		result= do_callback_bool (qpfunc, 0, NULL, function); \
		XSRETURN_IV (result); \
	}

#define generate_noarg_function_int(name, qpfunc, helptext) \
	static const char help_##name []= \
	helptext; \
	static XS (name) \
	{ \
		static const char function []= #name; \
		int result; \
		dXSARGS; \
		check_no_argument (function, items); \
		check_safe (function); \
		result= do_callback_int (qpfunc, function); \
		XSRETURN_IV (result); \
	}

static XS (mode)
{
	static const char function []= "mode";
	int mode;
	dXSARGS;

	check_num_args (function, items, 1, 1);

	mode= SvIV (ST (0) );
	if (do_callback (qpMode, mode, NULL, function) != qpRetOk)
	{
		croak ("qtre::mode: Invalid mode %i", mode);
		XSRETURN_UNDEF;
	}
	XSRETURN_YES;
}

generate_noarg_function_bool (empty, qpIsEmpty,
	"check if the current text is empty")

static const char help_modified []=
	"Check the value of the flag that indicates if the current "
	"text has been modified after laoding or creating it, or "
	"change it if called with a parameter.";

static XS (modified)
{
	static const char function []= "modified";
	int result;
	dXSARGS;

	check_num_args_safe (function, items, 0, 1);

	switch (items)
	{
	case 0:
		result= do_callback_bool (qpGetModified, 0, NULL, function);
		break;
	case 1:
		result= do_callback_bool (qpSetModified, SvIV (ST (0) ), NULL,
			function);
		break;
	default:
		assert (0);
	}
	XSRETURN_IV (result);
}

static const char help_up []=
"Move cursor up the number of lines specified, 1 if no argument.";

static XS (up)
{
	static const char function []= "up";
	int i, count;
	dXSARGS;

	check_num_args_safe (function, items, 0, 1);

	switch (items)
	{
	case 0:
		count= 1;
		break;
	case 1:
		count= SvIV (ST (0) );
		break;
	default:
		assert (0);
	}
	for (i= 0; i < count; ++i)
		if (do_callback_bool (qpUp, 0, NULL, function) != 1)
			XSRETURN_UNDEF;
	XSRETURN_YES;
}

static const char help_down []=
"Move cursor down the number of lines specified, 1 if no argument.";

static XS (down)
{
	static const char function []= "down";
	int i, count;
	dXSARGS;

	check_num_args_safe (function, items, 0, 1);

	switch (items)
	{
	case 0:
		count= 1;
		break;
	case 1:
		count= SvIV (ST (0) );
		break;
	default:
		assert (0);
	}
	for (i= 0; i < count; ++i)
		if (do_callback_bool (qpDown, 0, NULL, function) != 1)
			XSRETURN_UNDEF;
	XSRETURN_YES;
}

static const char help_left []=
"Move cursor left the number of characters specified, 1 if no argument.";

static XS (left)
{
	static const char function []= "left";
	int i, count;
	dXSARGS;

	check_num_args_safe (function, items, 0, 1);

	switch (items)
	{
	case 0:
		count= 1;
		break;
	case 1:
		count= SvIV (ST (0) );
		break;
	default:
		assert (0);
	}
	for (i= 0; i < count; ++i)
		if (do_callback_bool (qpLeft, 0, NULL, function) != 1)
			XSRETURN_UNDEF;
	XSRETURN_YES;
}

static const char help_right []=
"Move cursor right the number of characters specified, 1 if no argument.";

static XS (right)
{
	static const char function []= "right";
	int i, count;
	dXSARGS;

	check_num_args_safe (function, items, 0, 1);

	switch (items)
	{
	case 0:
		count= 1;
		break;
	case 1:
		count= SvIV (ST (0) );
		break;
	default:
		assert (0);
	}
	for (i= 0; i < count; ++i)
		if (do_callback_bool (qpRight, 0, NULL, function) != 1)
			XSRETURN_UNDEF;
	XSRETURN_YES;
}

generate_noarg_function_safe (home, qpHome,
	"Move cursor to the begin of line")

generate_noarg_function_safe (end, qpEnd,
	"Move cursor to the end of line")

generate_noarg_function_safe (pageup, qpPageUp,
	"Move cursor one page up")

generate_noarg_function_safe (pagedown, qpPageDown,
	"Move cursor one page down")

static const char help_goto_line []=
"Go to the line number specified as parameter.";

static XS (goto_line)
{
	static const char function []= "goto_line";
	int line;
	int n;
	dXSARGS;

	check_num_args_safe (function, items, 1, 1);

	line= SvIV (ST (0) );
	n= do_callback (qpGotoLine, line, NULL, function);
	XSRETURN_IV (n);
}

static const char help_get_char []=
"Returns the character under the cursor";

static XS (get_char)
{
	static const char function []= "get_char";
	char c;
	qpRetCode r;
	SV * sv;
	dXSARGS;

	check_no_argument (function, items);
	check_safe (function);

	r= do_callback (qpGetChar, 0, & c, function);
	if (r == qpRetOk && c != 0)
		sv= sv_2mortal (newSVpv (&c, 1) );
	else sv= & PL_sv_undef;
	/*
	XPUSHs (sv);
	XSRETURN (1);
	*/
	/*
	EXTEND (SP, 1);
	PUSHs (sv);
	PUTBACK;
	XSRETURN (1);
	*/
	ST (0)= sv;
	XSRETURN (1);
}

static const char help_get_word []=
"Returns the word under  the cursor, or the previous word if "
"is at the end of one.";

static XS (get_word)
{
	static const char function []= "get_word";
	qpStringParam param;
	SV * sv;
	dXSARGS;

	check_no_argument (function, items);
	check_safe (function);

	initStringParam (& param);
	if (! do_callback (qpGetWord, 0, & param, function) )
	{
		no_memory (function);
		XSRETURN_UNDEF;
	}
	sv= sv_2mortal (newSVpv (param.str, param.len) );
	callback_free_string (& param);

	ST (0)= sv;
	PUTBACK;
	XSRETURN (1);
}

static const char help_delete_char []=
"Delete the number of characters specified from cursor to right, "
"1 if no arguemnt.";

static XS (delete_char)
{
	static const char function []= "delete_char";
	int i, count;
	dXSARGS;

	check_num_args_safe (function, items, 0, 1);

	switch (items)
	{
	case 0:
		count= 1;
		break;
	case 1:
		count= SvIV (ST (0) );
		break;
	default:
		assert (0);
	}
	for (i= 0; i < count; ++i)
		do_callback (qpDeleteChar, 0, NULL, function);
	XSRETURN_YES;
}

static const char help_backspace []=
"Delete the number of characters specified from cursor to left, "
"1 if no arguemnt.";

static XS (backspace)
{
	static const char function []= "backspace";
	int i, count;
	dXSARGS;

	check_num_args_safe (function, items, 0, 1);

	switch (items)
	{
	case 0:
		count= 1;
		break;
	case 1:
		count= SvIV (ST (0) );
		break;
	default:
		assert (0);
	}
	for (i= 0; i < count; ++i)
		do_callback (qpBackspace, 0, NULL, function);
	XSRETURN_YES;
}

static const char help_delete_line []=
"Delete the number of lines specified, from current to end of file. "
"If no argument, delete current line.";

static XS (delete_line)
{
	static const char function []= "delete_line";
	int i, count;
	dXSARGS;

	check_num_args_safe (function, items, 0, 1);

	switch (items)
	{
	case 0:
		count= 1;
		break;
	case 1:
		count= SvIV (ST (0) );
		break;
	default:
		assert (0);
	}
	for (i= 0; i < count; ++i)
		do_callback (qpDeleteLine, 0, NULL, function);
	XSRETURN_YES;
}

static const char help_replace_line []=
"Replace the content of a line:\n"
"Parameters: content, line_number\n"
"\tcontent: The new content for the line, empty if omitted.\n"
"\tline_number: The line number to replace, the current line "
"if omitted.";

static XS (replace_line)
{
	static const char function []= "replace_line";
	char *str;
	int nline;
	STRLEN len;
	dXSARGS;

	check_num_args_safe (function, items, 0, 2);

	switch (items)
	{
	case 0:
		nline= do_callback_int (qpLine, function);
		str= "";
		break;
	case 1:
		nline= do_callback_int (qpLine, function);
		str= SvPV (ST (0), len);
		break;
	case 2:
		nline= SvIV (ST (1) );
		str= SvPV (ST (0), len);
		break;
	default:
		assert (0);
	}
	do_callback (qpReplaceLine, nline, str, function);
	XSRETURN_YES;
}

static const char help_insert_char []=
"Insert a character at the current cursor position.";

static XS (insert_char)
{
	static const char function []= "insert_char";
	char *str;
	int n;
	STRLEN len;
	dXSARGS;

	check_num_args_safe (function, items, 1, 1);

	if (SvIOKp (ST (0) ) )
		n= SvIV (ST (0) );
	else
	{
		str= SvPV (ST (0), len);
		if (len != 1)
		{
			croak ("qtre: Invalid parameter to insert_char");
			XSRETURN_UNDEF;
		}
		n= str [0];
	}
	do_callback (qpInsertChar, n, NULL, function);
	XSRETURN_YES;
}

static const char help_insert []=
"Insert text at the current cursor position.";

static XS (insert)
{
	static const char function []= "insert";
	qpStringParam param;
	STRLEN len;
	dXSARGS;

	check_num_args_safe (function, items, 1, 1);

	param.str= SvPV (ST (0), len);
	param.len= len;
	do_callback (qpInsert, 0, & param, function);
	XSRETURN_YES;
}

static const char help_get_beginsel []=
"Returns line and position of the beginning of the current selection";

static XS (get_beginsel)
{
	static const char function []= "get_beginsel";
	qpSelectionLimitsParam param;
	dXSARGS;

	check_no_argument (function, items);

	if (do_callback (qpSelectionLimits, 0, & param, function) != qpRetOk)
		XSRETURN_UNDEF;
	#if 0
	XPUSHs (sv_2mortal (newSViv (param.beginline) ) );
	XPUSHs (sv_2mortal (newSViv (param.beginpos) ) );
	XSRETURN (2);
	#else

	EXTEND (SP, 2);

	#if 0
	PUSHs (sv_2mortal (newSViv (param.beginline) ) );
	PUSHs (sv_2mortal (newSViv (param.beginpos) ) );
	PUTBACK;
	#else

	ST (0)= sv_2mortal (newSViv (param.beginline) );
	ST (1)= sv_2mortal (newSViv (param.beginpos) );

	#endif

	XSRETURN (2);

	#endif
}

static const char help_get_endsel []=
"Returns line and position of the end of the current selection";

static XS (get_endsel)
{
	static const char function []= "get_endsel";
	qpSelectionLimitsParam param;
	dXSARGS;

	check_no_argument (function, items);

	if (do_callback (qpSelectionLimits, 0, & param, function) != qpRetOk)
		XSRETURN_UNDEF;
	#if 0
	XPUSHs (sv_2mortal (newSViv (param.endline) ) );
	XPUSHs (sv_2mortal (newSViv (param.endpos) ) );
	XSRETURN (2);
	#else

	EXTEND (SP, 2);

	#if 0
	PUSHs (sv_2mortal (newSViv (param.endline) ) );
	PUSHs (sv_2mortal (newSViv (param.endpos) ) );
	PUTBACK;
	#else

	ST (0)= sv_2mortal (newSViv (param.endline) );
	ST (1)= sv_2mortal (newSViv (param.endpos) );

	#endif

	XSRETURN (2);

	#endif
}

static const char help_get_selection []=
"Returns a string with the content of the current selection";

static XS (get_selection)
{
	static const char function []= "get_selection";
	qpRetCode r;
	char * str;
	qpStringParam param;
	SV * svstr;
	dXSARGS;

	check_no_argument (function, items);
	check_safe (function);

	initStringParam (& param);
	r= do_callback (qpGetSelection, 0, & param, function);
	if (r != qpRetOk)
	{
		no_memory (function);
		XSRETURN_UNDEF;
	}

	#if 0
	fprintf (stderr, "Param length: %i\n", param.len);
	fflush (stderr);
	fprintf (stderr, "Param text: %s\n", param.str);
	fflush (stderr);
	#endif

	EXTEND (SP, 1);
	/* ST(0)= sv_2mortal (newSVpv (param.str, param.len) ); */
	svstr= newSVpv (param.str, param.len);
	ST(0)= sv_2mortal (svstr);

	/*free (param.str);*/
	/*do_callback (qpFreeString, 0, & param);*/
	callback_free_string (& param);

	XSRETURN (1);
}

static const char help_get_clipboard []=
"Returns a string with the content of the clipboard";

static XS (get_clipboard)
{
	static const char function []= "get_clipboard";
	qpRetCode r;
	char * str;
	qpStringParam param;
	dXSARGS;

	check_no_argument (function, items);
	check_safe (function);

	initStringParam (& param);
	r= do_callback (qpGetClipboard, 0, & param, function);
	if (r != qpRetOk)
	{
		no_memory (function);
		XSRETURN_UNDEF;
	}
	ST(0)= sv_2mortal (newSVpv (param.str, (int) param.len) );
	/*free (param.str);*/
	/*do_callback (qpFreeString, 0, & param);*/
	callback_free_string (& param);

	XSRETURN (1);
}

static const char help_set_clipboard []=
"Set the content of the clipboard with the string passed as parameter, "
"the empty string if no parameter.";

static XS (set_clipboard)
{
	static const char function []= "set_clipboard";
	char * str;
	STRLEN len;
	dXSARGS;

	check_num_args_safe (function, items, 0, 1);

	switch (items)
	{
	case 0:
		str= "";
		break;
	case 1:
		str= SvPV (ST (0), len);
		break;
	default:
		assert (0);
	}
	do_callback (qpSetClipboard, 0, str, function);
	XSRETURN_YES;
}

generate_noarg_function_int (line, qpLine,
	"return the current 0-based line number")

generate_noarg_function_int (num_lines, qpNumLines,
	"Return the number of lines of the current text\n"
	"Always not zero because there is at least an empty "
	"line at end")

static const char help_char_line []=
"Returns the number of char that contains the line specified as "
"parameter, or in the current line if no parameter. The possible "
"newline char at the end is not counted.";

static XS (char_line)
{
	static const char function []= "char_line";
	int n;
	int nline;
	dXSARGS;

	check_num_args_safe (function, items, 0, 1);

	switch (items)
	{
	case 0:
		nline= do_callback_int (qpLine, function);
		break;
	case 1:
		nline= SvIV (ST (0) );
		break;
	default:
		assert (0);
	}
	do_callback (qpCharLine, nline, & n, function);
	XSRETURN_IV (n);
}

static const char help_get_line []=
"Returns a string with the content of the line specified as parameter, "
"or the current line if no parameter.";

static XS (get_line)
{
	static const char function []= "get_line";
	qpStringParam param;
	int n;
	int nline;
	char * str;
	SV * sv;
	dXSARGS;

	check_num_args_safe (function, items, 0, 1);

	switch (items)
	{
	case 0:
		nline= do_callback_int (qpLine, function);
		break;
	case 1:
		nline= SvIV (ST (0) );
		break;
	default:
		assert (0);
	}

	#if 0
	n= do_callback (qpCharLine, nline, NULL);
	str=  malloc (n + 1);
	if (str == NULL)
	{
		no_memory (function);
		XSRETURN_UNDEF;
	}
	do_callback (qpGetLine, nline, str);

	/*ST(0)= sv_2mortal (newSVpv (str, 0) );*/

	EXTEND (SP, 1);
	PUSHs (sv_2mortal (newSVpv (str, 0) ) );

	free (str);
	/*XSRETURN (1);*/
	PUTBACK;
	return;
	/*XSRETURN (1);*/
	#else

	initStringParam (& param);
	if (do_callback (qpGetLine, nline, & param, function) != qpRetOk)
	{
		no_memory (function);
		XSRETURN_UNDEF;
	}
	/*
	sv= sv_2mortal (newSVpv (str, 0) );
	free (str);
	*/
	sv= sv_2mortal (newSVpv (param.str, param.len) );
	/*do_callback (qpFreeString, 0, & param);*/
	callback_free_string (& param);

	/*if (items == 0)*/
	if (0)
	{
		EXTEND (SP, 1);
		PUSHs (sv);
	}
	else
		ST (0)= sv;
	PUTBACK;
	XSRETURN (1);

	#endif
}

generate_noarg_function_int (get_pos, qpGetPos,
	"Return the current position of the cursor in the current line.")

generate_noarg_function_int (get_col, qpGetCol,
	"Return the current column of the cursor in the current line.")

static const char help_get_fileext []=
"Returns the extension of the current file name, if any.";

static XS (get_fileext)
{
	static const char function []= "get_fileext";
	char buf [MAXPATH];
	dXSARGS;

	do_callback (qpGetFileExt, MAXPATH, buf, function);
	XSRETURN_PV (buf);
}

static const char help_get_filename []=
"Returns the name of the current file";

static XS (get_filename)
{
	static const char function []= "get_filename";
	dXSARGS;
	char buf [MAXPATH];
	do_callback (qpGetFileName, MAXPATH, buf, function);
	XSRETURN_PV (buf);
}

static const char help_status []=
"Show the string passed as argument in the text part of the status line.";

static XS (status)
{
	static const char function []= "status";
	char *str;
	STRLEN len;
	dXSARGS;

	check_num_args (function, items, 1, 1);

	str= SvPV (ST (0), len);
	do_callback (qpStatus, 0, str, function);
	XSRETURN_YES;
}

static const char help_status_question []=
"Help for this function not written yet";

static XS (status_question)
{
	static const char function []= "status_question";
	char * question;
	char * values;
	char r;
	qpStatusQuestionParam param;
	STRLEN len;
	dXSARGS;

	check_num_args (function, items, 2, 2);

	param.question= SvPV (ST (0), len);
	param.values= SvPV (ST (1), len);
	if (do_callback (qpStatusQuestion, 0, & param, function) != qpRetOk)
		croak ("status_question failed!");
	ST (0)= sv_2mortal (newSVpv (& param.result, 1) );
	XSRETURN (1);
}

static const char help_dialog_get_string []=
"Open a dialog box quering for a string, returning the string "
"or undef if cancelled.\n"
"Parameters: title, prompt, width, maxlength, text:\n"
"\ttitle: The title of the box.\n"
"\tprompt: Text informative that appears into the box.\n"
"\twidth: width of the text entry area.\n"
"\tmaxlenght: max length of text admisible.\n"
"\ttext: initial text of the dialog\n"
"maxlength and text are optionals. If maxlength is not specified "
"width is used as max length.";

static XS (dialog_get_string)
{
	static const char function []= "dialog_get_string";
	qpDialogGetStringParam param;
	qpRetCode r;
	STRLEN len;
	dXSARGS;

	check_num_args (function, items, 3, 5);

	param.title= SvPV (ST (0), len);
	param.prompt= SvPV (ST (1), len);
	param.width= SvIV (ST (2) );
	if (items >= 4)
		param.maxlength= SvIV (ST (3) );
	else
		param.maxlength= param.width;

	#if 0
	param.str=  malloc (param.maxlength + 1);
	if (param.str == NULL)
	{
		no_memory (function);
		XSRETURN_UNDEF;
	}
	#endif

	param.str= qperl_malloc (param.maxlength + 1, function);

	if (items >= 5)
	{
		char * aux;
		/*
		if (items > 5)
		{
			too_much (function);
			XSRETURN_UNDEF;
		}
		*/
		aux= SvPV (ST (4), len);
		strncpy (param.str, aux, param.maxlength);
		param.str [param.maxlength]= '\0';
	}
	else
		param.str [0]= '\0';

	r= do_callback (qpDialogGetString, 0, & param, function);
	if (r == qpRetTrue)
		ST(0)= sv_2mortal (newSVpv (param.str, 0) );

	//free (param.str);
	qperl_free (param.str);

	if (r == qpRetTrue)
		XSRETURN (1);
	else
		XSRETURN_UNDEF;
}

static const char help_dialog_show_text []=
"Help for this function not written yet";

static XS (dialog_show_text)
{
	static const char function []= "dialog_show_text";
	qpDialogShowTextParam param;
	STRLEN len;
	dXSARGS;

	check_num_args (function, items, 2, 5);

	param.title= SvPV (ST (0), len);
	param.str= SvPV (ST (1), len);
	param.height= 0;
	param.width= 0;
	if (items >= 3)
	{
		param.height= SvIV (ST (2) );
		if (items >= 4)
		{
			param.width= SvIV (ST (3) );
		}
	}
	do_callback (qpDialogShowText, 0, & param, function);
	XSRETURN_YES;
}

static const char help_define_control []=
"Define the effect of the control key specified in the first parameter "
"as calling the perl function whose name is contained in the second.";

static XS (define_control)
{
	static const char function []= "define_control";
	qpDefineControlParam param;
	char * str;
	STRLEN len;
	dXSARGS;

	check_num_args (function, items, 2, 2);

	str= SvPV (ST (0), len);
	if (len != 1)
		XSRETURN_UNDEF;
	param.key= str [0];
	param.function= SvPV (ST (1), len);
	do_callback (qpDefineControl, 0, & param, function);
	XSRETURN_YES;
}

generate_noarg_function (redraw, qpRedraw,
	"Redraw screen")

static const char help_quiet []=
"If the parameter is 1 enable quiet mode, if 0 disable it,"
"any other value reserved for future use.";

static XS (quiet)
{
	static const char function []= "quiet";
	int mode;
	dXSARGS;

	check_num_args (function, items, 0, 1);

	switch (items)
	{
	case 0:
		mode= 1;
		break;
	case 1:
		mode= SvIV (ST (0) );
		break;
	default:
		assert (0);
	}
	do_callback (qpQuiet, mode, 0, function);
	XSRETURN_YES;
}

generate_noarg_function (update, qpUpdate,
	"Update screen")

static const char help_toggle_select []=
"Begin selection mode or end it if already selecting.";

static XS (toggle_select)
{
	static const char function []= "toggle_select";
	int n;
	dXSARGS;

	check_no_argument (function, items);
	check_safe (function);

	n= do_callback (qpSelect, 0, NULL, function);
	XSRETURN_IV (n);
}

generate_noarg_function (begin_select, qpBeginSelect,
	"Begin selection mode")

generate_noarg_function (end_select, qpEndSelect,
	"Ends selection mode")

generate_noarg_function (select_all, qpSelectAll,
	"Select all text")

generate_noarg_function (delete_selection, qpDeleteSelection,
	"Delete selected text")

generate_noarg_function (cut, qpCut,
	"copy selection to clipboard and delete it")

generate_noarg_function (copy, qpCopy,
	"copy selection to clipboard")

generate_noarg_function (paste, qpPaste,
	"copy clipboard in current position")

generate_noarg_function_int (get_key, qpGetKey,
	"expect a key press and return his code")

static const char help_autoindent []=
"Enable or disable the auto indent mode.";

static XS (autoindent)
{
	static const char function []= "autoindent";
	int n;
	dXSARGS;

	check_num_args (function, items, 0, 1);

	switch (items)
	{
	case 0:
		n= do_callback_bool (qpGetAutoindent, 0, NULL, function);
		break;
	case 1:
		n= do_callback_bool (qpSetAutoindent, SvIV (ST (0) ), NULL,
			function);
		break;
	default:
		assert (0);
	}
	XSRETURN_IV (n);
}

static const char help_autocr []=
"Enable or disable the automatic CR mode (inserting a CR at the end of "
"line when inserting a new line if the current line already has a CR "
"at the end, useful with ms-dos text files).";

static XS (autocr)
{
	static const char function []= "autocr";
	int n;
	dXSARGS;

	check_num_args (function, items, 0, 1);

	switch (items)
	{
	case 0:
		n= do_callback_bool (qpGetAutocr, 0, NULL, function);
		break;
	case 1:
		n= do_callback_bool (qpSetAutocr, SvIV (ST (0) ), NULL,
			function);
		break;
	default:
		assert (0);
	}
	XSRETURN_IV (n);
}

static const char help_menu_add []=
"Help for this function not written yet";

static XS (menu_add)
{
	static const char function []= "menu_add";
	qpMenuAddParam param;
	int n;
	dXSARGS;

	check_num_args (function, items, 2, 2);

	param.position= SvIV (ST (0) );
	param.menuname= SvPV (ST (1), PL_na);
	n= (do_callback (qpMenuAdd, 0, & param, function) == qpRetOk);
	XSRETURN_IV (n);
}

static const char help_menu_add_option []=
"Help for this function not written yet";

static XS (menu_add_option)
{
	static const char function []= "menu_add_option";
	qpMenuAddOptionParam param;
	int n;
	dXSARGS;

	check_num_args (function, items, 4, 4);

	param.menuname= SvPV (ST (0), PL_na);
	param.position= SvIV (ST (1) );
	param.optionname= SvPV (ST (2), PL_na);
	param.perlfunc= SvPV (ST (3), PL_na);
	n= (do_callback (qpMenuAddOption, 0, & param, function) == qpRetOk);
	XSRETURN_IV (n);
}

static const char help_menu_rename []=
"Rename a menu or a menu option\n"
"Parameters: from, to\n"
"from: actual name of the menu or menu option, a '/' separate "
"menu, submenu, and option.\n"
"to: new name of menu or option. A & indicates that the next char "
"is the hotkey.";

static XS (menu_rename)
{
	static const char function []= "menu_add_option";
	qpMenuRenameParam param;
	int n;
	dXSARGS;

	check_num_args (function, items, 2, 2);

	param.from= SvPV (ST (0), PL_na);
	param.to= SvPV (ST (1), PL_na);
	n= do_callback_bool (qpMenuRename, 0, & param, function);
	XSRETURN_IV (n);
}

generate_noarg_function (file_load, qpFileLoad,
	"Open the dialog for open file.")

generate_noarg_function (file_new, qpFileNew,
	"Create a new file")

generate_noarg_function (file_save, qpFileSave,
	"Save current file.")

generate_noarg_function (file_save_as, qpFileSaveAs,
	"Save current file with a another name.")

generate_noarg_function (file_next, qpFileNext,
	"Make active the next file.")

generate_noarg_function (file_close, qpFileClose,
	"Close current file.")

generate_noarg_function (file_quit, qpFileQuit,
	"Close all files and quit qtre.")

void do_help_add_func (const char * func, const char * desc,
	const char * function)
{
	qpHelpAddFuncParam param;
	param.func= func;
	param.desc= desc;
	do_callback (qpHelpAddFunc, 0, & param, function);
}

static const char help_help_add_func []=
"Help for this function not written yet";

static XS (help_add_func)
{
	static const char function []= "help_add_func";
	dXSARGS;

	check_num_args (function, items, 2, 2);

	do_help_add_func (SvPV (ST (0), PL_na), SvPV (ST (1), PL_na),
		function);
	XSRETURN_YES;
}

generate_noarg_function (macro, qpMacro,
	"Expect a key and execute the corresponding macro, if any.")

static const char help_end_init []=
"Help for this function not written yet";

static XS (end_init)
{
	static const char function []= "end_init";
	dXSARGS;

	#if 0
	if (items != 0)
	{
		no_argument (function);
		XSRETURN_UNDEF;
	}
	#endif
	check_no_argument (function, items);

	init_ended= 1;
	XSRETURN_YES;
}

generate_noarg_function_safe (copy_new_file, qpCopyNewFile,
	"Copy selection to a new file")

generate_noarg_function_safe (help, qpHelp,
	"Show the help index")

generate_noarg_function_safe (help_perl, qpHelpPerl,
	"Show the help of predefined perl functions")

generate_noarg_function_safe (help_about, qpHelpAbout,
	"Show the about box of qtre")

generate_noarg_function_safe (compile, qpCompile,
	"Create a new file and compile the previous current file "
	"showing the output in it")

generate_noarg_function_safe (make, qpMake,
	"Create a new file and execute make in current directory "
	"showing the output in it")

generate_noarg_function_safe (compile_options, qpCompileOptions,
	"Open a dialog to change compiler options used by the "
	"'compile' function")

generate_noarg_function_safe (man, qpMan,
	"Show in a new file the man page of the word under the cursor")

generate_noarg_function_safe (man_of, qpManOf,
	"Open a dialog to enter a word and show the man page about that")

static const char help_stupid_test []=
"Only for internal test, do not use it";

static XS (stupid_test)
{
	static const char function []= "stupid_test";
	dXSARGS;
	do_callback (qpFailTest, 0, NULL, function);
	XSRETURN (1);
}

/**********************************************************
		Table of predefined functions
***********************************************************/

typedef XS ( (*perlfunction) );

struct table_t {
	const char * const name;
	perlfunction func;
	const char * helptext;
};

#define def(a) { #a, a, help_ ## a }

static struct table_t table []= {
	def (mode),
	def (empty),
	def (modified),
	def (up),
	def (down),
	def (left),
	def (right),
	def (home),
	def (end),
	def (pageup),
	def (pagedown),
	def (goto_line),
	def (get_char),
	def (get_word),
	def (delete_char),
	def (backspace),
	def (delete_line),
	def (replace_line),
	def (insert_char),
	def (insert),
	def (get_beginsel),
	def (get_endsel),
	def (get_selection),
	def (get_clipboard),
	def (set_clipboard),
	def (line),
	def (num_lines),
	def (char_line),
	def (get_line),
	def (get_pos),
	def (get_col),
	def (get_fileext),
	def (get_filename),
	def (status),
	def (status_question),
	def (dialog_get_string),
	def (dialog_show_text),
	def (define_control),
	def (redraw),
	def (quiet),
	def (update),
	def (toggle_select),
	def (begin_select),
	def (end_select),
	def (select_all),
	def (delete_selection),
	def (cut),
	def (copy),
	def (paste),
	def (get_key),
	def (autoindent),
	def (autocr),
	def (menu_add),
	def (menu_add_option),
	def (menu_rename),
	def (file_load),
	def (file_new),
	def (file_save),
	def (file_save_as),
	def (file_next),
	def (file_close),
	def (file_quit),
	def (help_add_func),
	def (macro),
	def (end_init),
	def (copy_new_file),
	def (help),
	def (help_perl),
	def (help_about),
	def (compile),
	def (make),
	def (compile_options),
	def (man),
	def (man_of),

	def (stupid_test)
};

static void loadqtrefuncs ()
{
	static char module []= "qperl.c";
	static char perlfuncname [128];
	char * func;
	struct table_t * pt;
	int i, l;

	for (i= 0, pt= table; i < DIM (table); ++i, ++pt)
	{
		/* Check that the buffer is long enough */
		assert (strlen (qtre_prefix_default) + strlen (pt->name) <
			sizeof (perlfuncname) );

		/* Actual function, can be redefined */
		strcpy (perlfuncname, qtre_prefix);
		strcat (perlfuncname, pt->name);
		newXS (perlfuncname, pt->func, module);

		/* Default function, intended to be called by
		one that redefines the actual */
		strcpy (perlfuncname, qtre_prefix_default);
		strcat (perlfuncname, pt->name);
		newXS (perlfuncname, pt->func, module);

		do_help_add_func (pt->name, pt->helptext, "loadqtrefuncs");
	}
}

static int isCurrentDir (const char * str)
{
	char cur [PATH_MAX];
	if (! getcwd (cur, sizeof (cur) ) )
		return 0;
	return strcmp (cur, str) == 0;
}

static int checkerror (const char * title)
{
	if (! SvTRUE (ERRSV) )
		return 0;
	else
	{
		/*errormessage (title, (char *) SvPV_nolen (ERRSV) );*/
		/* SvPV_nolen not available in all versions. */
		STRLEN len;
		char * strerr;
		strerr= SvPV (ERRSV, len);
		errormessage (title, strerr);
		/* Set to false the quick way.*/
		SvCUR_set (ERRSV, 0);
		return 1;
	}
}

static PerlInterpreter *MyPerl;

static int issafe_check (uid_t uid, struct stat * statbuf)
{
	/*
		Check that the file is owned by the uid specified
		and is not writable by others.
		If the owner is not root or the group is not root
		chech also that is not writable by the group.
	*/
	uid_t fileuid;
	gid_t filegid;
	mode_t filemode;

	if (statbuf == NULL)
		return 0;
	fileuid=   statbuf->st_uid;
	filegid=   statbuf->st_gid;
	filemode= statbuf->st_mode;

	if (fileuid != uid)
		return 0;
	if (filemode & S_IWOTH)
		return 0;
	if (uid == 0)
	{
		if (filegid != 0 && (filemode & S_IWGRP) )
			return 0;
	}
	else
	{
		if (filemode & S_IWGRP)
			return 0;
	}
	return 1;
}

static int issafe (uid_t uid, struct stat * statbuf, const char * strFile)
{
	int r;
	/* First check for the current user */
	r= issafe_check (uid, statbuf);

	/* If not, check for root */
	if (! r && uid != 0)
		r= issafe_check (0, statbuf);

	if (! r && getenv (envQTRE_NO_WARN) == NULL)
	{
		char text [256];
		sprintf (text, "WARNING: '%s' is not safe", strFile);
		errormessage ("Security", text);
	}
	return r; 
}

static CV * get_cv_function (const char * function)
{
	char * f;
	int l;
	CV * cv;
	char buffer [128];

	check_inited ();

	l= strlen (function) + sizeof (qtre_prefix);
	if (l > sizeof (buffer) )
		return NULL;

	strcpy (buffer, qtre_prefix);
	strcat (buffer, function);

	/*  Search for function, first with qtre prefix, if not
	    found without prefix */

	cv= perl_get_cv (buffer, 0);
	if (! cv)
		cv= perl_get_cv (buffer + sizeof (qtre_prefix) - 1, 0);
	return cv;
}

qpfunctionresult qperl_function (const char * function, void * data)
{
	qpfunctionresult r;
	CV * cv;
	dSP;

	check_inited ();

	PUSHMARK (SP);

	/*gdata= data;*/

	cv= get_cv_function (function);

	if (cv != NULL)
	{
		/*perl_call_pv (f,
			G_VOID | G_DISCARD | G_NOARGS | G_EVAL);
		*/
		perl_call_sv ( (SV *) cv,
			G_VOID | G_DISCARD | G_NOARGS | G_EVAL);
		if (checkerror ("Error in function") )
			r= qpfunction_failed;
		else
			r= qpfunction_ok;
	}
	else r= qpfunction_notfound;
	return r;
}

qpfunctionresult qperl_function_args (const char * function, void * data,
	int argc, const char * const * argv)
{
	qpfunctionresult r;
	CV * cv;
	int i;
	dSP;

	check_inited ();

	ENTER;
	SAVETMPS;
	PUSHMARK (SP);
	for (i= 0; i < argc; ++i)
	{
		/* Casting required in some versions to avoid warning */
		char * str= (char *) argv [i];
		XPUSHs (sv_2mortal (newSVpv (str, 0) ) );
	}
	PUTBACK;

	r= qpfunction_ok;
	/*gdata= data;*/
	cv= get_cv_function (function);
	if (! cv)
		r= qpfunction_notfound;
	else
	{
		perl_call_sv ( (SV *) cv,
			G_VOID | G_DISCARD | G_EVAL);
		if (checkerror ("Error in function") )
			r= qpfunction_failed;
	}

	SPAGAIN;
	PUTBACK;
	FREETMPS;
	LEAVE;
	return r;
}

qpfunctionresult qperl_function_args_r (const char * function, void * data,
	int argc, const char * const * argv, char * * result)
{
	qpfunctionresult r;
	CV * cv;
	int i;
	dSP;

	check_inited ();

	ENTER;
	SAVETMPS;
	PUSHMARK (SP);
	for (i= 0; i < argc; ++i)
	{
		/* Casting required in some versions to avoid warning */
		char * str= (char *) argv [i];
		XPUSHs (sv_2mortal (newSVpv (str, 0) ) );
	}
	PUTBACK;

	r= qpfunction_ok;
	/*gdata= data;*/
	cv= get_cv_function (function);
	if (! cv)
		r= qpfunction_notfound;
	else
	{
		perl_call_sv ( (SV *) cv,
			G_SCALAR | G_EVAL);
		if (checkerror ("Error in function") )
			r= qpfunction_failed;
		else
		{
			SV * sv;
			char * str;
			STRLEN len;
			SPAGAIN;
			/* str= SvPV (SP [0], len); */
			sv= POPs;
			str= SvPV (sv, len);
			//* result= malloc (len + 1);
			* result= qperl_malloc_nocheck (len + 1);
			if (* result != NULL)
			{
				memcpy (* result, str, len);
				(* result) [len]= '\0';
			}
		}
	}

	PUTBACK;
	FREETMPS;
	LEAVE;
	return r;
}

int qperl_eval (const char * str, void * data)
{
	SV * svstr;
	int result;
	dSP;

	check_inited ();

	PUSHMARK (SP);
	/*gdata= data;*/

	/* Need perl_eval_sv because perl_eval_pv does not report
	 syntax errors.
	 */
	svstr= newSVpv ( (char *) str, 0);
	perl_eval_sv (svstr,
		G_VOID | G_DISCARD | G_NOARGS | G_EVAL | G_KEEPERR);

	#if 0
	/*sv_2mortal (svstr); */
	sv_unref (svstr);

	return checkerror ("Error in eval");
	#endif

	result= checkerror ("Error in eval");
	sv_unref (svstr);
	return result;
}

/*
	gdata defined here to avoid accidental use
	by other functions.
*/

static void * gdata= NULL;

#ifndef NDEBUG

static void check_inited ()
{
	assert (gdata != NULL);
}

#endif

int init_finished= 0;

static void check_safe (const char * function)
{
	check_inited ();
	if (! init_finished)
		croak ("The function %s "
			"cannot be called during initialization", function);
}

static qpRetCode do_callback (qpFuncCode qpfunc, int n, void * p,
	const char * funcname)
{
	qpRetCode ret;

	assert (gdata != NULL);

	if (qpfunc == qpFreeString)
		croak ("Invalid use of qpFreeString callback");

	ret= qpCallback (gdata, qpfunc, n, p);
	if (ret == qpRetUnexpected)
		croak ("Unexpected result in function %s", funcname);
	return ret;
}


static int do_callback_bool (qpFuncCode qpfunc, int n, void * p,
	const char * funcname)
{
	qpRetCode ret;
	ret= do_callback (qpfunc, n, p, funcname);
	switch (ret)
	{
	case qpRetFalse: return 0;
	case qpRetTrue: return 1;
	default:
		croak ("Invalid return value for %s", funcname);
	}
}

static int do_callback_int (qpFuncCode qpfunc, const char * funcname)
{
	qpRetCode ret;
	int result;
	ret= do_callback (qpfunc, 0, & result, funcname);
	if (ret != qpRetOk)
		croak ("Invalid return value for %s", funcname);
	return result;
}


void callback_free_string (qpStringParam * param)
{
	assert (gdata != NULL);

	if (param->str == NULL)
	{
		assert (param->len == 0);
	}
	else
	{
		qpRetCode r;
		r= qpCallback (gdata, qpFreeString, 0, param);
		if (r != qpRetOk)
			croak ("Unexpected fail of qpFreeString");
	}
}

static void errormessage (const char * title, const char * message)
{
	/*
		This function can be called during initialization,
		show the message in stderr in that case.
	*/
	if (gdata)
	{
		qpDialogShowTextParam param;
		param.title= title;
		param.str= message;
		param.height= 0;
		param.width= 60;
		do_callback (qpDialogShowText, 0, & param, "errormessage");
	}
	else
	{
		fprintf (stderr, "\r\n%s\r\n%s\r\n", title, message);
		sleep (3);
	}
}

#ifdef __GCC__
extern void xs_init (pTHXo);
#else
extern void xs_init ();
#endif

int qperl_init (char * * env, void * data)
{
	static char * pl_argv [] = { "", "-e", "1;" };
	char * strHome;
	struct stat statbuf;
	uid_t uid;
	int abortiffail;

	if (gdata != NULL)
	{
		errormessage ("qperl_init",
			"Embedded perl is already initialized");
		return 1;
	}

	gdata= data;

	MyPerl= perl_alloc ();
	perl_construct (MyPerl);
	perl_parse (MyPerl, xs_init, DIM (pl_argv), pl_argv, env);

	loadqtrefuncs ();

	/* Option to not load initialization files. */
	if (getenv (envQTRE_NO_INIT) != NULL)
		return 0;

	/*
		Load the initilization files.
	*/

	abortiffail= getenv (envQTRE_NO_ABORT) == NULL;
	uid= getuid ();

	/* First, global. */

	if (stat (strGlobalPl, & statbuf) == 0)
	{
		/* Check safety for root */
		if (issafe (0, & statbuf, strGlobalPl) )
		{
			perl_require_pv ( (char *) strGlobalPl);
			//if (checkerror ("Error in /etc/qtre.pl") &&
			if (checkerror ("Error in " PATH_QTRE_PL) &&
				abortiffail)
			{
				qperl_destroy ();
				return 1;
			}
		}
	}

	/* /etc/qtre.pl can forbid further initialisation. */
	if (init_ended)
		return 0;

	/* Second, user. */

	strHome= getenv (envHOME);
	if (strHome)
	{
		char strPackage [PATH_MAX];
		int l;
		l= strlen (strHome);
		strcpy (strPackage, strHome);
		if (l > 0 && strPackage [l - 1] != '/')
			strcat (strPackage, "/");
		strcat (strPackage, strPl);

		if (stat (strPackage, & statbuf) == 0)
		{
			if (issafe (uid, & statbuf, strPackage) )
			{
				perl_require_pv (strPackage);
				if (checkerror ("Error in user .qtre.pl") &&
					abortiffail)
				{
					qperl_destroy ();
					return 1;
				}
			}
		}
	}

	/*  $HOME/.qtre.pl can also forbid further initialisation, */
	if (init_ended)
		return 0;

	/* Last, current directory if is not home */

	if (! strHome || ! isCurrentDir (strHome) )
	{
		/* Last chance to avoid current directory personlization:
		environment variable. */
		if (getenv (envQTRE_NO_DIR) )
			return 0;

		if (stat (strPl, & statbuf) == 0)
		{
			if (issafe (uid, & statbuf, strPl) )
			{
				perl_require_pv ( (char *) strPl);
				if (checkerror ("Error in local .qtre.pl") &&
					abortiffail)
				{
					qperl_destroy ();
					return 1;
				}
			}
		}
	}
	return 0;
}

void qperl_init_done ()
{
	/*
		Signals that now is safe to call functions
		that needs a text available.
	*/
	init_finished= 1;
}

void qperl_destroy (void)
{
	if (gdata == NULL)
	{
		errormessage ("qperl_destroy",
			"Embedded perl is not initialized");
		return;
	}
	gdata= NULL;
	init_finished= 0;

	perl_destruct (MyPerl);
	perl_free (MyPerl);
	if (qperl_malloc_counter != 0)
		fprintf (stderr, "**WARNING** perl malloc counter is %lu\n",
			(unsigned long) qperl_malloc_counter);
}

/* End of qperl.c */

// screenimpl.cpp
// Revision 7-sep-2006

#include "screenimpl.h"

#include "dirutil.h"
#include "selectfile.h"
#include "text.h"
#include "textwindow.h"
#include "termstream.h"
#include "exec.h"
#include "qperl.h"
#include "dialog.h"
#include "finder.h"
#include "history.h"

#include "util.h"

#include "trace.h"
#undef ASSERT
#include "qtreassert.h"

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <set>
#include <new>
#include <memory>
#include <exception>
#include <stdexcept>
#include <algorithm>

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <assert.h>

using std::cerr;
using std::endl;
using std::flush;
using std::string;
using std::vector;
using std::ostringstream;
using std::set;
using std::nothrow;
using std::auto_ptr;
using std::exception;
using std::runtime_error;
using std::swap;
using std::logic_error;
using std::for_each;

using util::dim_array;
using util::to_string;

extern const string strVersion;
extern const string strAbout1;
extern const string strAbout2;
extern const string strAbout3;

extern bool fExit;

//***********************************************
//		ScreenImpl
//***********************************************

ScreenImpl::ScreenImpl (bool useutf8, char * * env) :
	ScreenBase (useutf8),
	MenuBuilder (* this),
	LinkPerl (env, this),
	win (initwin () ),
	actualwin (0),
	ptext (win [actualwin].text () ),
	pclipboard (new Text (useutf8) ),
	autoindent (true),
	autocr (false),
	maxlin (lines () - 1),
	maxcol (columns () - 1),
	redraw_all_lines (true),
	fSelecting (false),
	exiting (false),
	quietmode (false),
	pfinder (0)
{
	TRF;

	inittext ();
	init_actions ();
	set_text_inited ();
	perlinitdone ();
}

ScreenImpl::~ScreenImpl ()
{
	TRF;

	if (! qpstring_alloc.empty () )
	{
		TRDEB ("*ERROR*: "
			"there is some unallocated qpStringParam objects");
		qpstring_alloc.erase (qpstring_alloc.begin (),
			qpstring_alloc.end () );
	}
}

void ScreenImpl::invalidate_all_lines ()
{
	redraw_all_lines= true;
}

void ScreenImpl::update_lines ()
{
	TRF;

	if (redraw_all_lines)
	{
		putall ();
		redraw_all_lines= false;
	}
	textwindow_at (lin () - inilin (), col () - inicol () );
}

void ScreenImpl::status_current_lincol ()
{
	status_lincol (lin (), col () );
}

//	Exec virtual functions

void ScreenImpl::ExecSendChar (char c)
{
	sendchar (c);
}

bool ScreenImpl::ExecSendBlock (const char * str, size_t size)
{
	do_instext (str, size);
	invalidate_all_lines ();
	update ();
	return true;
}

bool ScreenImpl::ExecQueryAbort ()
{
	if (checkkey () != 0)
	{
		setstatus ("Aborting execed process...");
		return true;
	}
	else
		return false;
}

void ScreenImpl::redraw ()
{
	screen_invalidate ();
	update ();
}

ScreenImpl::ContainerWindow ScreenImpl::initwin ()
{
	ContainerWindow aux;
	aux.push_back (TextWindow (usingutf8 () ) );
	return aux;
}

void ScreenImpl::changewindow (size_t newwin)
{
	TRF;

	endselect ();
	actualwin= newwin;
	ASSERT (actualwin < win.size () );
	ptext= win [actualwin].text ();

	setstatus (ptext->name () );
	invalidate_all_lines ();
}

void ScreenImpl::windownew ()
{
	win.push_back (TextWindow (usingutf8 () ) );
	changewindow (win.size () - 1);
}

void ScreenImpl::nextwindow ()
{
	changewindow ( (actualwin + 1) % win.size () );
}

bool ScreenImpl::closewindow ()
{
	TRF;

	if (! question_save () )
		return false;

	win.erase (win.begin () + actualwin);

	if (win.size () == 0 && ! exiting)
		win.push_back (TextWindow (usingutf8 () ) );

	size_t newwin= actualwin;
	if (newwin >= win.size () )
		newwin= 0;
	if (win.size () > 0)
		changewindow (newwin);
	return true;
}

namespace {

inline qpRetCode qpretbool (bool r)
{
	return r ? qpRetTrue : qpRetFalse;
}

} // namespace

qpRetCode ScreenImpl::callback (qpFuncCode f, int n, void * data)
{
	TRF;

	switch (f)
	{
	case qpMode:
		if (! setmode (n) )
			return qpRetInvalidValue;
		break;
	case qpIsEmpty:
		return qpretbool (ptext->empty () );
	case qpGetModified:
		return qpretbool (ptext->modified () );
	case qpSetModified:
		{
			qpRetCode r= qpretbool (ptext->modified () );
			ptext->modified (n);
			return r;
		}
	case qpUp:
		return qpretbool (up () );
	case qpDown:
		return qpretbool (down () );
	case qpLeft:
		return qpretbool (back () );
	case qpRight:
		return qpretbool (forward () );
	case qpHome:
		home ();
		break;
	case qpEnd:
		end ();
		break;
	case qpPageUp:
		pageup ();
		break;
	case qpPageDown:
		pagedown ();
		break;
	case qpGotoLine:
		if (static_cast <unsigned> (n) >= ptext->lines () )
			return qpRetFailed;
		gotoline (n);
		break;
	case qpGetChar:
		* (static_cast <char *> (data) )= getcurchar ();
		break;
	case qpGetWord:
		{
			qpStringParam * pparam=
				static_cast <qpStringParam *> (data);
			string str
				(ptext->wordat (lin (), col () ) );
			if (! string_to_qpstringparam (str, * pparam) )
				return qpRetFailed;
		}
		break;
	case qpDeleteChar:
		deletechar ();
		break;
	case qpBackspace:
		backdelete ();
		break;
	case qpDeleteLine:
		deleteline ();
		break;
	case qpReplaceLine:
		replaceline ( (char *) data, n);
		break;
	case qpInsertChar:
		sendchar (n);
		break;
	case qpInsert:
		{
			qpStringParam * pparam=
				static_cast <qpStringParam *> (data);
			instext (pparam->str, pparam->len);
		}
		break;
	case qpGetSelection:
		{
			qpStringParam * pparam=
				static_cast <qpStringParam *> (data);
			Text dest (usingutf8 () );
			copyselection (dest);
			string str= dest.str ();
			if (! string_to_qpstringparam (str, * pparam) )
				return qpRetFailed;
		}
		break;
	case qpSelectionLimits:
		{
			Selection & sel= selection ();
			if (sel.empty () )
				return qpRetFailed;
			qpSelectionLimitsParam * pparam=
				static_cast <qpSelectionLimitsParam *> (data);
			pparam->beginline= sel.begin ().lin;
			pparam->beginpos= sel.begin ().pos;
			pparam->endline= sel.end ().lin;
			pparam->endpos= sel.end ().pos;
		}
		break;
	case qpSelect:
		optionEditSelect ();
		break;
	case qpBeginSelect:
		beginselect ();
		break;
	case qpEndSelect:
		endselect ();
		break;
	case qpSelectAll:
		optionEditSelectAll ();
		break;
	case qpDeleteSelection:
		optionEditDelete ();
		break;
	case qpGetClipboard:
		{
			qpStringParam * pparam=
				static_cast <qpStringParam *> (data);
			Text dest (usingutf8 () );
			copyclipboard (dest);
			string str= dest.str ();
			if (! string_to_qpstringparam (str, * pparam) )
				return qpRetFailed;
		}
		break;
	case qpSetClipboard:
		TRDEB ( static_cast <char *> (data) );
		pclipboard->set ( (char *) data);
		break;
	case qpCut:
		optionEditCut ();
		break;
	case qpCopy:
		optionEditCopy ();
		break;
	case qpPaste:
		optionEditPaste ();
		break;
	case qpLine:
		* (static_cast <int *> (data) )= lin ();
		break;
	case qpNumLines:
		* (static_cast <int *> (data) )= ptext->lines ();
		break;
	case qpCharLine:
		* (static_cast <int *> (data) )= (* (ptext) ) [n].size ();
		break;
	case qpGetLine:
		{
			TRDEBS ("qpGetLine " << n);

			qpStringParam * pparam=
				static_cast <qpStringParam *> (data);
			const string & str= (* ptext) [n].str ();
			if (! string_to_qpstringparam (str, * pparam) )
				return qpRetFailed;
		}
		break;
	case qpGetPos:
		* (static_cast <int *> (data) )= getpos ();
		break;
	case qpGetCol:
		* (static_cast <int *> (data) )= col ().getn ();
		break;
	case qpGetFileExt:
		getfileext (n, (char *) data);
		break;
	case qpGetFileName:
		getfilename (n, (char *) data);
		break;
	case qpStatus:
		setstatus ( (char *) data);
		break;
	case qpStatusQuestion:
		{
			qpStatusQuestionParam * pparam=
				static_cast <qpStatusQuestionParam *> (data);
			pparam->result= status_question
				(pparam->question, pparam->values);
		}
		break;
	case qpDialogGetString:
		{
			qpDialogGetStringParam * pparam=
				static_cast <qpDialogGetStringParam *> (data);
			string str (pparam->str);
			static History history;
			bool r= dialog_get_string (
				pparam->title, pparam->prompt,
				str, history,
				pparam->width, pparam->maxlength);
			if (r)
			{
				string::size_type l= str.size ();
				ASSERT (l <= pparam->maxlength);
				str.copy (pparam->str, l);
				pparam->str [l]= '\0';
			}
			return r ? qpRetTrue : qpRetFalse;
		}
	case qpDialogShowText:
		{
			qpDialogShowTextParam * pparam=
				static_cast <qpDialogShowTextParam *> (data);
			dialog_show_text (pparam->title, pparam->str,
				pparam->height, pparam->width);
		}
		break;
	case qpDefineControl:
		{
			qpDefineControlParam * pparam=
				static_cast <qpDefineControlParam *> (data);
			string function (pparam->function);
			definecontrolkey (pparam->key, function);
		}
		break;
	case qpRedraw:
		redraw ();
		break;
	case qpQuiet:
		quietmode= n;
		break;
	case qpUpdate:
		update ();
		break;
	case qpGetKey:
		* (static_cast <int *> (data) )= getkey ();
		break;
	case qpSetAutoindent:
		{
			qpRetCode r= qpretbool (autoindent);
			autoindent= n != 0;
			return r;
		}
	case qpGetAutoindent:
		return qpretbool (autoindent);
	case qpSetAutocr:
		{
			qpRetCode r= qpretbool (autocr);
			autocr= n != 0;
			return r;
		}
	case qpGetAutocr:
		return qpretbool (autocr);
	case qpMenuAdd:
		{
			qpMenuAddParam * pparam=
				static_cast <qpMenuAddParam *> (data);

			#if 0
			ostringstream oss;
			oss << pparam->position << ' ' << pparam->menuname;
			setstatus (oss.str () );
			#endif

			TRDEB ("Adding menu");
			Menu * pmenu= new Menu ();
			Option * pop= new Option (pparam->menuname, pmenu);
			//menu.insertOption (pop, pparam->position);
			//menu.show (menuwindow);
			//redraw_menu= true;
			//menu_invalidate ();
			menu_insert_option (pop, pparam->position);
		}
		break;
	case qpMenuAddOption:
		{
			qpMenuAddOptionParam * pparam=
				static_cast <qpMenuAddOptionParam *> (data);

			#if 0
			ostringstream oss;
			oss << pparam->menuname << ' ' << pparam->position <<
				' ' << pparam->optionname << ' ' <<
				pparam->perlfunc;
			setstatus (oss.str () );
			#endif

			TRDEB ("Adding option to menu");
			auto_ptr <ScreenActionPerl> paction
				(new ScreenActionPerl
					(* this, pparam->perlfunc) );
			auto_ptr <Option> popt (new Option
				(pparam->optionname, paction.get () ) );
			paction.release ();
			bool r= menu_insert_suboption (pparam->menuname,
				popt.get (), pparam->position);
			if (r)
			{
				popt.release ();
				return qpRetFailed;
			}
			else
				return qpRetOk;
		}
		break;
	case qpMenuRename:
		{
			qpMenuRenameParam * pparam=
				static_cast <qpMenuRenameParam *> (data);
			return qpretbool
				(menu_rename (pparam->from, pparam->to) );
		}
	case qpFileLoad:
		optionFileLoad ();
		break;
	case qpFileNew:
		optionFileNew ();
		break;
	case qpFileSave:
		optionFileSave ();
		break;
	case qpFileSaveAs:
		optionFileSaveAs ();
		break;
	case qpFileNext:
		optionFileNext ();
		break;
	case qpFileClose:
		optionFileClose ();
		break;
	case qpFileQuit:
		optionFileQuit ();
		break;
	case qpHelpAddFunc:
		{
			qpHelpAddFuncParam * pparam=
				static_cast <qpHelpAddFuncParam *> (data);
			perlfunchelp_set (pparam->func, pparam->desc);
		}
		break;
	case qpMacro:
		macro ();
		break;
	case qpFreeString:
		{
			qpStringParam * pparam=
				static_cast <qpStringParam *> (data);
			free_qpstringparam (* pparam);
		}
		break;
	case qpCopyNewFile:
		optionEditCopyToNew ();
		break;
	case qpHelp:
		optionHelpIndex ();
		break;
	case qpHelpPerl:
		optionHelpPerl ();
		break;
	case qpHelpAbout:
		optionHelpAbout ();
		break;
	case qpCompile:
		optionProgCompile ();
		break;
	case qpMake:
		optionProgMake ();
		break;
	case qpCompileOptions:
		optionProgCompileOptions ();
		break;
	case qpMan:
		optionProgMan ();
		break;
	case qpManOf:
		optionProgManOf ();
		break;
	case qpFailTest:
		throw runtime_error ("This is a fail test");
	default:
		return qpRetInvalidFunc;
	}
	return qpRetOk;
}

void ScreenImpl::showerror (const char * str)
{
	// Try by all means to show the error without
	// propagating exceptions.
	try
	{
		dialog_show_text ("ERROR", str,
			std::min <size_t> (lines () / 2, 7), columns () / 2);
	}
	catch (...)
	{
		try
		{
			setstatus (string ("ERROR: ") + str);
		}
		catch (...)
		{
			cerr << "ERROR: " << str << endl;
		}
	}
}

void ScreenImpl::getline (int nline, char * str)
{
	const TextLine & s= (* ptext) [nline];
	size_t l= s.size ();
	size_t i;
	for (i= 0; i < l; ++i)
		str [i]= s [i];
	str [i]= '\0';
}

bool ScreenImpl::getline (int nline, char * & str, size_t & len)
{
	const TextLine & s= (* ptext) [nline];
	len= s.size ();
	str= new (nothrow) char [len + 1];
	if (str == NULL)
		return false;
	s.str ().copy (str, len);
	str [len]= '\0';
	return true;
}

void ScreenImpl::getfileext (int n, char * str)
{
	TRF;

	string ext (dirutil::extension (ptext->name () ) );
	string::size_type l;
	if ( (l= ext.size () ) < string::size_type (n) )
		ext.copy (str, l);
	else
		l= 0;
	str [l]= 0;
	TRDEB (str);
}

void ScreenImpl::getfilename (int n, char * str)
{
	TRF;

	string file (dirutil::base_name (ptext->name () ) );
	string::size_type l;
	if ( (l= file.size () ) < string::size_type (n) )
		file.copy (str, l);
	else
		l= 0;
	str [l]= 0;
	TRDEB (str);
}

void ScreenImpl::instext (const char * str)
{
	util::bool_saver saveautoindent (autoindent, false);
	util::bool_saver saveautocr (autocr, false);
	for (; * str != '\0'; ++str)
		sendchar (* str);
}

namespace {

const char * findnewline (const char * str, size_t len)
{
	for ( ; len > 0; --len, ++str)
		if (* str == '\n')
			return str;
	return 0;
}

} // namespace

void ScreenImpl::do_instext (const char * str, size_t len)
{
	util::bool_saver saveautoindent (autoindent, false);
	util::bool_saver saveautocr (autocr, false);
	size_t initial= lin ();
	size_t pos= getpos ();
	const char * straux= findnewline (str, len);
	if (! straux)
	{
		const string & s= (* ptext) [initial].str ();
		ptext->setline (initial, s.substr (0, pos) +
			string (str, len) + s.substr (pos) );
		pos+= len;
		col ()= (* ptext) [initial].pos2col (pos);
		adjustinicol ();
		if (! redraw_all_lines)
			putlineabs (initial);
	}
	else
	{
		ptext->insertline (initial, col (), false);
		size_t l= straux - str;
		ptext->append (initial, string (str, l) );
		size_t current= initial + 1;
		str= straux + 1;
		len-= l + 1;

		while ( (straux= findnewline (str, len) ) != 0)
		{
			l= straux - str;
			ptext->insertline (current, string (str, l) );
			str= straux + 1;
			len-= l + 1;
			++current;
		}

		const string & s= (* ptext) [current].str ();
		ptext->setline (current, string (str, len) + s);

		lin ()= current;
		col ()= (* ptext) [current].pos2col (len);
		adjustinicol ();
		adjustinilin ();
		invalidate_all_lines ();
	}
}

void ScreenImpl::instext (const char * str, size_t len)
{
	try
	{
		do_instext (str, len);
	}
	catch (exception &e)
	{
		setstatus (string ("Error: ") + e.what () );
	}
	catch (...)
	{
		setstatus ("Unexpected error");
	}
}

void ScreenImpl::inittext ()
{
	inilin ()= 0;
	inicol ()= Col ();
	lin ()= 0;
	col ()= Col ();
	fSelecting= false;
}

void ScreenImpl::beginselect ()
{
	setstatus ("Selecting");
	Selection & sel= win [actualwin].sel;
	bool fSelPrev= sel.begin () != sel.end ();

	sel.init (lin (), getpos () );

	fSelecting= true;
	if (fSelPrev)
		invalidate_all_lines ();
}

void ScreenImpl::endselect ()
{
	status_clear ();
	fSelecting= false;
}

void ScreenImpl::selectall ()
{
	TRF;

	Selection & sel= win [actualwin].sel;
	TRDEB ("Moving begin");
	sel.movebegin (0, 0);
	TRDEB ("Evaulating last line");
	size_t n= ptext->lines ();
	if (n == 0)
		sel.moveend (0, 0);
	else
	{
		TRDEB ("Evaluating last col");
		size_t pos= (* ptext) [n - 1].size ();
		TRDEB ("Moving end");
		sel.moveend (n - 1, pos);
	}
	fSelecting= false;
	invalidate_all_lines ();
}

void ScreenImpl::copyselection (Text & dest)
{
	TRF;

	endselect ();
	Selection & sel= win [actualwin].sel;
	if (sel.begin () == sel.end () )
		return;

	ptext->copyselection (sel, dest);
}

void ScreenImpl::copyselection ()
{
	TRF;

	copyselection (* pclipboard);
}

void ScreenImpl::copyclipboard (Text & dest)
{
	TRF;

	dest= * pclipboard;
}

void ScreenImpl::deleteselection ()
{
	endselect ();
	Selection & sel= win [actualwin].sel;
	if (sel.begin () == sel.end () )
		return;
	size_t linbeg= sel.begin ().lin;
	size_t linend= sel.end ().lin;
	TextFile & text= * ptext;
	if (linbeg == linend)
		text.setline (linbeg,
			text [linbeg].substr (0, sel.begin ().pos) +
			text [linbeg].substr (sel.end ().pos) );
	else
	{
		text.setline (linbeg,
			text [linbeg].substr (0, sel.begin ().pos) );
		text.setline (linend,
			text [linend].substr (sel.end ().pos) );
		for (size_t lin= linbeg + 1; lin < linend; ++lin)
			text.deleteline (linbeg + 1);
		text.joinline (linbeg);
	}
	lin ()= linbeg;
	col ()= text [lin () ].pos2col (sel.begin ().pos);
	sel.init (0, 0);
	adjustinilin ();
	invalidate_all_lines ();
}

void ScreenImpl::cut ()
{
	endselect ();
	Selection & sel= win [actualwin].sel;
	if (sel.begin () == sel.end () )
		return;
	setstatus ("Cutting");
	copyselection ();
	deleteselection ();
}

void ScreenImpl::copy ()
{
	endselect ();
	Selection & sel= win [actualwin].sel;
	if (sel.begin () == sel.end () )
		return;
	setstatus ("Copying");
	copyselection ();	
}

void ScreenImpl::paste ()
{
	endselect ();
	setstatus ("Pasting");
	util::bool_saver saveautoindent (autoindent, false);
	util::bool_saver saveautocr (autocr, false);
	util::bool_saver savequietmode (quietmode, true);

	size_t l= pclipboard->lines ();
	size_t current= lin ();
	size_t pos= getpos ();
	if (l == 1)
	{
		const string & s= (* ptext) [current].str ();
		const string & str= (* pclipboard) [0].str ();
		ptext->setline (current, s.substr (0, pos) + str +
			s.substr (pos) );
		pos+= str.size ();
		col ()= (* ptext) [current].pos2col (pos);
		adjustinicol ();
		if (! redraw_all_lines)
			putlineabs (current);
	}
	else
	{
		ptext->insertline (current, col (), false);
		ptext->append (current, (* pclipboard) [0].str () );
		++current;

		for (size_t i= 1; i < l - 1; ++i, ++current)
		{
			ptext->insertline
				(current, (* pclipboard) [i].str () );
		}

		const string & s= (* ptext) [current].str ();
		const string & lastclip= (* pclipboard) [l - 1].str ();
		ptext->setline (current, lastclip + s);

		lin ()= current;
		col ()= (* ptext) [current].pos2col (lastclip.size () );
		adjustinicol ();
		adjustinilin ();
		invalidate_all_lines ();
	}
	status_clear ();
}

void ScreenImpl::gotoline (size_t l, Col gotocol)
{
	TRF;

	ASSERT (l < ptext->lines () );

	PosText pt;
	if (fSelecting)
		pt.set (lin (), getpos () );
	size_t iniant= inilin ();
	if (l < inilin () )
	{
		lin ()= l;
		inilin ()= l;
	}
	else
		if (l < inilin () + maxlin)
			lin ()= l;
		else
		{
			inilin ()= l - maxlin;
			lin ()= l;
		}
	col ()= gotocol;
	if (fSelecting)
		movesel (pt, lin (), getpos () );
	if (inilin () != iniant || fSelecting)
		invalidate_all_lines ();
	adjustinicol ();
}

void ScreenImpl::gotoline (size_t l)
{
	gotoline (l, Col (0) );
}

char ScreenImpl::getcurchar ()
{
	const TextLine & s= (* ptext) [lin () ];
	size_t pos= s.col2pos (col () );
	if (pos >= s.size () )
	{
		if (lin () >= ptext->lines () - 1) return '\0';
		else return '\n';
	}
	return s [pos];
}

namespace {

std::string expandline (const TextLine & s, bool useutf8,
	size_t inipos, size_t maxpos,
	//Col maxcol, bool initescape)
	Col begcol, Col endcol, bool isinicol)
{
	TRFDEBS ("inipos= " << inipos <<
		" maxpos= " << maxpos <<
		//" maxcol= " << maxcol <<
		//" initescape: " << initescape <<
		" begcol= " << begcol <<
		" endcol= " << endcol <<
		"source: '" << s.substr (inipos, maxpos) << '\'');

	// Testing
	bool usetab= getenv ("QTRE_USE_TAB") != NULL;

	maxpos= std::min (maxpos, s.size () );
	if (inipos >= maxpos)
		return string ();

	std::string result;
	//Col col;
	//if (initescape)
	if (isinicol && begcol.odd () )
	{
		ASSERT (inipos > 0);
		ASSERT (escapable (s [inipos - 1], useutf8) );
		result+= escaped (s [inipos - 1] );
		//++col;
		//++begcol;
	}

	//size_t pos (inipos);
	//for ( ; pos < maxpos && col <= maxcol; ++pos)
	Col col (begcol);
	for (size_t pos= inipos; pos < maxpos && col < endcol; ++pos)
	{
		char c= s [pos];
		if (c == '\t')
		{
			if (usetab)
			{
				col.nexttab ();
				result+= '\t';
			}
			else
			{
				//TEST
				Col prevcol (col);
				col.nexttab ();
				for ( ; prevcol < col; ++prevcol)
					result+= ' ';
			}
		}
		else
		{
			if (unsigned char escchar= escapable (c, useutf8) )
			{
				++col;
				result+= escchar;
				//if (col <= maxcol)
				if (col < endcol)
				{
					result+= escaped (c);
					++col;
				}
			}
			else
			{
				++col;
				result+= c;
			}
		}
	}

	TRDEBS ("result: '" << result << '\'');
	return result;
}

} // namespace

void ScreenImpl::putline (size_t winline)
{
	TRFDEBS ("Screen line: " << winline);

	ASSERT (winline <= maxlin);
	if (redraw_all_lines)
		return;

	const size_t absline= winline + inilin ();
	TRDEBS ("Document line: " << absline);

	textwindow_at (winline);
	if (absline >= ptext->lines () )
	{
		textwindow_clreol ();
		return;
	}
	const TextLine & s= (* ptext) [absline];
	const size_t s_size= s.size ();

	// Attempt to optimize in very large lines: if initial
	// column is greater than the maximum posiible end column
	// of current line, quick exit.
	if (inicol () > Col (8 * s_size) )
	{
		textwindow_clreol ();
		return;
	}

	const size_t inipos= s.col2pos (inicol () );
	if (inipos >= s_size)
	{
		textwindow_clreol ();
		return;
	}
	Col begcol (s.pos2col (inipos) );

	Selection & sel= win [actualwin].sel;
	const PosText & selbeg (sel.begin () );
	const PosText & selend (sel.end () );
	bool somesel= selbeg != selend;

	const size_t linbegsel= somesel ? selbeg.lin : 0;
	const size_t linendsel= somesel ? selend.lin : 0;
	/*const*/ size_t posbegsel= somesel ? selbeg.pos : 0;
	/*const*/ size_t posendsel= somesel ? selend.pos : 0;

	bool fSelected= somesel &&
		absline >= linbegsel && absline <= linendsel;
	bool fSelectedBeg= absline == linbegsel;

	const size_t width= columns ();

	const Col endcol (inicol () + maxcol + 1);

	#define TESTING_EXPANDLINE
	#ifdef TESTING_EXPANDLINE
	if (! fSelected)
	{
		//size_t pos= s.col2pos (inicol () );
		//if (inipos <= s_size)
		//{
			ASSERT (inipos < s_size);
			//Col ini (s.pos2col (inipos) );
			string str (expandline (s, usingutf8 (), inipos,
				static_cast <size_t> (-1),
				//maxcol, ini.odd () ) );
				begcol, endcol, true) );
			outstring (str);
			if (str.size () < width)
				textwindow_clreol ();
		//}
		//else
		//	textwindow_clreol ();
		return;
	}
	if (linbegsel < absline && linendsel > absline)
	{
		//size_t pos= s.col2pos (inicol () );
		//if (inipos <= s_size)
		//{
			ASSERT (inipos < s_size);
			textwindow_inverse (true);

			//Col ini (s.pos2col (inipos) );
			string str (expandline (s, usingutf8 (),
				inipos,
				static_cast <size_t> (-1),
				//maxcol, ini.odd () ) );
				begcol, endcol, true) );
			outstring (str);

			textwindow_inverse (false);
			if (str.size () < width)
				textwindow_clreol ();
		//}
		//else
		//	textwindow_clreol ();
		return;
	}

	if (linbegsel < absline)
	{
		//ASSERT (linbegsel == absline);
		posbegsel= 0;
	}
	if (linendsel > absline)
	{
		posendsel= size_t (-1);
	}
	else
	{
		//ASSERT (linendsel == absline);
	}

	// Write a partially selected line.
	bool isinicol= true;
	size_t used= 0;
	if (inipos < posbegsel)
	{
		string str (expandline (s, usingutf8 (), inipos, posbegsel,
			begcol, endcol, true) );
		outstring (str);
		used+= str.size ();
		if (used > 0)
			isinicol= false;
	}

	Col colbegsel (s.pos2col (posbegsel) );
	if (begcol > colbegsel)
	{
		colbegsel= begcol;
		posbegsel= s.col2pos (begcol);
	}
	if (colbegsel < endcol)
	{
		textwindow_inverse (true);
		string str (expandline (s, usingutf8 (), posbegsel, posendsel,
			colbegsel, endcol, isinicol) );
		outstring (str);
		used+= str.size ();
		if (used > 0)
			isinicol= false;
		textwindow_inverse (false);
	}
	Col colendsel (s.pos2col (posendsel) );
	if (begcol > colendsel)
	{
		colendsel= begcol;
		posendsel= s.col2pos (begcol);
	}
	if (colendsel < endcol)
	{
		string str (expandline (s, usingutf8 (), posendsel, size_t (-1),
			colendsel, endcol, isinicol) );
			outstring (str);
		used+= str.size ();
	}
	if (used < width)
		textwindow_clreol ();
	else
	{
		ASSERT (used == width);
	}

	return;

	#endif

	bool fSelectedEnd= absline == sel.end ().lin;
	if (fSelected && ! fSelectedBeg)
		textwindow_inverse (true);

	//size_t pos= s.col2pos (inicol () );
	Col ini;
	if (inipos <= s_size)
	{
		ini= s.pos2col (inipos);
		ASSERT (ini == inicol () || ini == inicol () + 1);
	}
	unsigned char c;
	if (ini.odd () && inipos <= s_size)
	{
		ASSERT (ini == inicol () + 1);
		ASSERT (inipos > 0);
		ASSERT (escapable (s [inipos - 1], usingutf8 () ) );
		outchar (escaped (s [inipos - 1] ) );
	}
	Col acol;
	for (size_t pos= inipos; acol <= maxcol && pos < s_size; ++pos)
	{
		if (fSelectedBeg && pos >= posbegsel)
		{
			textwindow_inverse (true);
			fSelectedBeg= false;
		}
		if (fSelectedEnd && pos >= posendsel)
		{
			textwindow_inverse (false);
			fSelectedEnd= false;
		}
		c= s [pos];
		if (c == '\t')
		{
			acol.nexttab ();
			outchar ('\t');
		}
		else
			if (unsigned char escchar= escapable (c, usingutf8 () ) )
			{
				acol+= 2;
				outchar (escchar);
				if (acol <= maxcol + 1)
					outchar (escaped (c) );
			}
			else
			{
				++acol;
				outchar (c);
			}
	}
	if (acol <= maxcol)
		textwindow_clreol ();
	textwindow_inverse (false);
}

inline void ScreenImpl::putlineabs (size_t line)
{
	size_t ini= inilin ();
	ASSERT (line >= ini);
	if (line < ini)
		return;
	size_t l= line - ini;
	ASSERT (l <= maxlin);
	if (l > maxlin)
		return;
	putline (l);
}

inline void ScreenImpl::putline (const PosText & pt)
{
	putlineabs (pt.lin);
}

inline void ScreenImpl::putactualline ()
{
	putlineabs (lin () );
}

void ScreenImpl::putall ()
{
	TRF;

	redraw_all_lines= false;
	if (win.size () == 0)
		return;
	for (size_t i= 0; i <= maxlin; ++i)
		putline (i);
}

void ScreenImpl::scrollup ()
{
	--inilin ();
	if (quietmode)
		invalidate_all_lines ();
	else
	{
		textwindow_at (0);
		textwindow_insline ();
		putline (0);
	}
}

void ScreenImpl::scrolldown ()
{
	++inilin ();
	if (quietmode)
		invalidate_all_lines ();
	else
	{
		textwindow_at (0);
		textwindow_delline ();
		putline (maxlin);
	}
}

void ScreenImpl::adjustmaxcol ()
{
	const TextLine & s= (* ptext) [lin () ];
	Col m= s.colmax ();
	if (col () > m)
		col ()= m;
	else
		col()= s.pos2col (s.col2pos (col () ) );
}

void ScreenImpl::movesel (const PosText & opt, size_t lin, size_t pos)
{
	PosText npt (lin, pos);
	Selection & sel= win [actualwin].sel;
	if (opt == sel.end () )
		sel.moveend (npt);
	else
	{
		ASSERT (opt == sel.begin () );
		sel.movebegin (npt);
	}
}

void ScreenImpl::home ()
{
	if (col () == Col () )
		return;
	PosText opt;
	if (fSelecting)
		//opt.set (lin (), (* ptext) [lin () ].col2pos (col () ) );
		opt.set (lin (), getpos () );
	col ()= Col ();
	adjustinicol ();
	if (fSelecting)
	{
		movesel (opt, lin (), 0);
		if (! redraw_all_lines)
			putactualline ();
	}
}

void ScreenImpl::end ()
{
	const TextLine & s= (* ptext) [lin () ];
	size_t npos= s.size ();
	Col acol (col () );
	col ()= s.pos2col (npos);
	if (col () == acol)
		return;
	adjustinicol ();
	if (fSelecting)
	{
		movesel (PosText (lin (), s.col2pos (acol) ), lin (), npos);
		if (! redraw_all_lines)
			putactualline ();
	}
}

bool ScreenImpl::up ()
{
	if (lin () == 0)
		return false;
	PosText opt;
	TextFile & text= * ptext;
	if (fSelecting)
		opt.set (lin (), text [lin () ].col2pos (col () ) );
	--lin ();
	adjustmaxcol ();
	if (fSelecting)
	{
		size_t npos= text [lin () ].col2pos (col () );
		movesel (opt, lin (), npos);
		putline (opt);
	}
	if (lin () < inilin () )
		scrollup ();
	else
		if (fSelecting)
			putactualline ();
	return true;
}

void ScreenImpl::move_up ()
{
	up ();
}

void ScreenImpl::pageup ()
{
	size_t oini= inilin ();
	PosText opt;
	TextFile & text= * ptext;
	if (fSelecting)
		opt.set (lin (), text [lin () ].col2pos (col () ) );
	if (inilin () > 0)
	{
		if (inilin () > maxlin) inilin ()-= maxlin;
		else inilin ()= 0;
		if (lin () > maxlin) lin()-= maxlin;
		else lin ()= 0;
	}
	else
	{
		if (lin () == 0) return;
		lin ()= 0;
	}
	adjustmaxcol ();
	if (fSelecting)
		movesel (opt, lin (), text [lin () ].col2pos (col () ) );
	if (oini != inilin () || fSelecting)
		invalidate_all_lines ();
}

bool ScreenImpl::down ()
{
	TextFile & text= * ptext;
	if (lin () + 1 >= text.lines () ) return false;
	PosText opt;
	if (fSelecting)
		opt.set (lin (), text [lin () ].col2pos (col () ) );
	++lin ();
	adjustmaxcol ();
	if (fSelecting)
	{
		size_t npos= text [lin () ].col2pos (col () );
		movesel (opt, lin (), npos);
		putline (opt);
	}
	if (lin () > inilin () + maxlin)
	{
		scrolldown ();
		ASSERT (lin () == inilin () + maxlin);
	}
	else
		if (fSelecting)
			putactualline ();
	return true;
}

void ScreenImpl::move_down ()
{
	down ();
}

void ScreenImpl::pagedown ()
{
	TextFile & text= * ptext;
	if (inilin () >= text.lines () - 1)
	{
		ASSERT (inilin () == text.lines () - 1);
		return;
	}
	PosText opt;
	if (fSelecting)
		opt.set (lin (), text [lin () ].col2pos (col () ) );
	if (inilin () + maxlin < text.lines () )
	{
		inilin ()+= maxlin;
		lin ()+= maxlin;
		if (lin () >= text.lines () )
			lin ()= text.lines () - 1;
	}
	else
	{
		inilin ()= text.lines () - 1;
		lin ()= inilin ();
	}
	adjustmaxcol ();
	if (fSelecting)
		movesel (opt, lin (), text [lin () ].col2pos (col () ) );
	invalidate_all_lines ();
}

void ScreenImpl::go_begin ()
{
	gotoline (0);
}

void ScreenImpl::go_end ()
{
	size_t l= ptext->lines () - 1;
	gotoline (l, (* ptext) [l].colmax () );
}

bool ScreenImpl::back ()
{
	TextFile & text= * ptext;
	const TextLine & s= text [lin () ];
	size_t pos= s.col2pos (col () );
	PosText opt;
	if (fSelecting)
		opt.set (lin (), pos);
	if (pos > 0)
	{
		--pos;
		col ()= s.pos2col (pos);
	}
	else
	{
		if (lin () == 0) return false;
		--lin ();
		col ()= text [lin () ].colmax ();
		pos= text [lin ()].size ();
	}
	if (fSelecting)
		movesel (opt, lin (), pos);
	if (lin () < inilin () )
		scrollup ();
	else
		if (fSelecting && lin () != opt.lin)
			putline (opt);
	if (fSelecting)
		putactualline ();
	return true;
}

void ScreenImpl::move_left ()
{
	back ();
}

bool ScreenImpl::forward ()
{
	TextFile & text= * ptext;
	const TextLine & s= text [lin () ];
	size_t pos= s.col2pos (col () );
	PosText opt;
	if (fSelecting)
		opt.set (lin (), pos);
	if (pos < s.size () )
	{
		++pos;
		col ()= s.pos2col (pos);
	}
	else
	{
		if (lin () + 1 >= text.lines () )
			return false;
		++lin ();
		col ()= Col ();
		pos= 0;
	}
	if (fSelecting)
		movesel (opt, lin (), pos);
	if (lin () - inilin () > maxlin)
		scrolldown ();
	else
		if (fSelecting && lin () != opt.lin)
			putline (opt);
	if (fSelecting)
		putactualline ();
	return true;
}

void ScreenImpl::move_right ()
{
	forward ();
}

void ScreenImpl::deletechar ()
{
	endselect ();
	const TextLine & s= (* ptext) [lin () ];
	size_t pos= s.col2pos (col () );

	if (pos >= s.size () )
	{
		if (lin () < ptext->lines () - 1)
		{
			ptext->joinline (lin () );
			textwindow_invalidate ();
		}
	}
	else
	{
		ptext->deletecharpos (lin (), pos);
		putactualline ();
	}
}

void ScreenImpl::backdelete ()
{
	if (back () )
		deletechar ();
}

void ScreenImpl::inschar (unsigned char c)
{
	//TRACEFUNC (tr, "ScreenImpl::inschar");

	endselect ();
	ptext->insertchar (lin (), col (), c);
}

void ScreenImpl::sendnewline ()
{
	TRF;

	const TextLine & tline= (* ptext) [lin () ];
	const size_t pos= tline.col2pos (col () );
	size_t s= tline.size ();

	bool atend= pos == s;
	bool hascr= s > 0 && tline [s - 1] == '\r';
	const size_t newpos= ptext->insertline
		(lin (), col (), autoindent);
	if (hascr && autocr)
	{
		if (atend)
			ptext->appendchar (lin () + 1, '\r');
		else
			ptext->appendchar (lin (), '\r');
	}
	down ();
	ASSERT (lin () > 0);

	if (pos == 0)
	{
		TRDEB ("at 0");
		textwindow_at (lin () - inilin () - 1);
		textwindow_insline ();
	}
	else
	{
		putlineabs (lin () - 1);
		textwindow_at (lin () - inilin () );
		textwindow_insline ();
	}
	putactualline ();
	col ()= (*ptext) [lin () ].pos2col (newpos);
}

void ScreenImpl::sendchar (unsigned char c)
{
	//TRACEFUNC (tr, "ScreenImpl::sendchar");

	endselect ();
	switch (c)
	{
	case '\n':
		sendnewline ();
		break;
	default:
		inschar (c);
		forward ();
		putactualline ();
	}
}

void ScreenImpl::adjustinicol ()
{
	TRF;

	if (col () < inicol () )
	{
		TRDEB ("adjusting left");
		inicol ()= col ();
		inicol ().previoustab ();
		invalidate_all_lines ();
	}
	else if (col () > inicol () + maxcol)
	{
		TRDEB ("adjusting right");
		inicol ()= col () - maxcol;
		inicol ().nexttab ();
		invalidate_all_lines ();
	}
}

void ScreenImpl::adjustinilin ()
{
	TRF;

	if (lin () < inilin () )
		inilin ()= lin ();
	else
		if (lin () > inilin () + maxlin)
			inilin ()= lin () - maxlin;
		else return;
	invalidate_all_lines ();
}

bool ScreenImpl::do_save_as ()
{
	TRF;

	update ();

	string strFile;
	bool r= dialog_get_string ("Save as",
		"File name to save", strFile, historyfilename, 60);

	if (! r)
		return false;
	if (strFile.empty () )
		return false;

	screen_invalidate ();
	ptext->save (strFile);
	setstatus (ptext->name () );

	return true;
}

bool ScreenImpl::do_save ()
{
	TRF;

	if (ptext->hasname () )
	{
		screen_invalidate ();
		ptext->save ();
		setstatus (ptext->name () );
		return true;
	}
	else
		return do_save_as ();
}

bool ScreenImpl::do_save_all ()
{
	TRF;

	for (size_t i= 0; i < win.size (); ++i)
	{
		TextFile * aux= win [i].text ();
		if (! aux->saved () )
		{
			if (aux->hasname () )
			{
				screen_invalidate ();
				aux->save ();
			}
			else
			{
				changewindow (i);
				if (! do_save_as () )
					return false;
			}
		}
	}
	return true;
}

void ScreenImpl::do_execute (const string & str)
{
	TRF;

	util::bool_saver saveautoindent (autoindent, false);
	setstatus ("Executing \"" + str + '"');
	Command (str);
	setstatus ('"' + str + "\" done");
}

bool ScreenImpl::question_save ()
{
	TRF;

	if (ptext->saved () )
		return true;

	//update ();
	dialog_result dr= dialog_question ("Save changes?");
	textwindow_invalidate ();

	switch (dr)
	{
	case dialog_cancel:
		return false;
	case dialog_yes:
		return do_save ();
	default:
		;
	}
	return true;
}

namespace {

class RedrawerScreen : public RedrawerSelectFile {
public:
	RedrawerScreen (ScreenImpl & screen) :
		screen (screen)
	{
		screen.update ();
	}
	void doit ()
	{
		screen.textwindow_invalidate ();
		screen.update ();
	}
private:
	ScreenImpl & screen;
};

} // namespace

bool ScreenImpl::askSearchOptions (SearchOptions & options, bool replace)
{
	string helptext=
		"I-Ignore case, B-Backwards, W-Whole word "
		"R-Regular expresssion";
	if (replace)
		helptext+= " F-Function";
	string strOptions;
	bool r= dialog_get_string (
		"Options",
		helptext,
		strOptions, historysearchoptions, 10);
	if (! r)
		return false;

	bool invalid= false;
	for (string::size_type i= 0, l= strOptions.size (); i < l; ++i)
	{
		switch (strOptions [i] )
		{
		case 'I': case 'i':
			options.nocase= true;
			break;
		case 'B': case 'b':
			options.backwards= true;
			break;
		case 'W': case 'w':
			options.wholeword= true;
			break;
		case 'R': case 'r':
			options.regex= true;
			break;
		case 'F': case 'f':
			if (! replace)
				invalid= true;
			else
				options.function= true;
			break;
		default:
			invalid= true;
		}
		if (invalid)
			throw runtime_error ("Invalid option");
	}
	if (options.regex && options.wholeword)
		throw runtime_error ("Options R and W are icompatibles");
	return true;
}

void ScreenImpl::buildfinder (const SearchOptions & options,
	const string & strFind)
{
	Finder * poldfinder= pfinder;
	pfinder= createFinder (options, strFind);
	delete poldfinder;
}

bool ScreenImpl::searchtext (const string & /*searched*/,
	SearchOptions /*options*/)
{
	TRF;

	setstatus ("Searching...");

	TextFile & text= * ptext;
	// Close possible open selcting.
	//fSelecting= false;
	PosText opt;
	if (fSelecting)
		opt.set (lin (), text [lin () ].col2pos (col () ) );

	ASSERT (pfinder != NULL);

	const TextLine & firstline= text [lin () ];
	const string & firststr= firstline.str ();
	string::size_type r;
	bool fSigueBuscando= true;
	string::size_type ini= firstline.col2pos (col () );

	size_t l= lin ();

	r= pfinder->find (firststr, ini);
	fSigueBuscando= r == string::npos;

	if (fSigueBuscando)
	{
		for (;;)
		{
			if (pfinder->backwards () )
			{
				if (l == 0)
					return false;
				--l;
			}
			else
			{
				++l;
				if (l >= text.lines () )
					return false;
			}
			const string & str= text [l].str ();

			r= pfinder->find (str);
			if (r != string::npos)
				break;
		}
		if (l >= text.lines () )
			return false;
	}
	ASSERT (r != string::npos);
	r+= pfinder->length ();
	if (l != lin () )
	{
		lin ()= l;
		adjustinilin ();
	}
	col ()= text [lin () ].pos2col (r);

	if (fSelecting)
	{
		movesel (opt, lin (), r);
		invalidate_all_lines ();
	}

	return true;
}

namespace {

class PopenCloser {
public:
	PopenCloser (const string str)
	{
		int fnull= open ("/dev/null", O_WRONLY);
		saveerr= dup (STDERR_FILENO);
		dup2 (fnull, STDERR_FILENO);
		close (fnull);
		f= popen (str.c_str (), "r");
	}
	~PopenCloser ()
	{
		dup2 (saveerr, STDERR_FILENO);
		if (f)
			pclose (f);
	}
	bool failed () const { return ! f; }
	operator FILE * () { return f; }
private:
	FILE * f;
	int saveerr;
};

} // namespace

void ScreenImpl::openman (const string & str)
{
	TRF;

	//string command= "man " + str + " 2> /dev/null";
	string command= "man " + str;
	const string st ("executing " + command);
	setstatus (st);
	setstatus (st);
	PopenCloser f (command);
	if (f.failed () )
	{
		setstatus ("Error in popen");
		return;
	}

	// Reads the man page skipping the backspaced
	// chars used for bold and underlined.

	int c;
	int prev= fgetc (f);

	if (prev == EOF)
		setstatus ("No man for " + str);
	else
	{
		windownew ();
		putall ();
		setstatus (st);
		textwindow_at (0);
		util::bool_saver saveautoindent (autoindent, false);
		util::bool_saver saveautocr (autocr, false);
		util::bool_saver savequietmode (quietmode, true);
	
		while ( (c= fgetc (f) ) != EOF)
		{
			if (c != '\x08')
				sendchar (prev);
			else
				c= fgetc (f);
			prev= c;
		}
		sendchar (prev);
		ptext->modified (false);
		ptext->readonly (true);
		go_begin ();
		putall ();
		status_clear ();
	}
}

void ScreenImpl::perlfunc (const string & func)
{
	callperlfunc (func);
}

void ScreenImpl::macro ()
{
	TRF;

	int c2= statusgetkey ("Macro key: ");
	status_clear ();
	if (c2 > 255 || c2 < 0)
		return;

	string macro ("macro_");
	// Don't use isalnum because is locale dependant.
	if ( (c2 >= '0' && c2 <= '9') ||
		(c2 >= 'a' && c2 <= 'z') || (c2 >= 'A' && c2 <= 'Z') )
	{
		macro+= char (tolower (c2) );
	}
	else if (c2 >= 1 && c2 <= 'z' - 'a' + 1)
	{
		macro+= "ctrl_";
		macro+= char (c2 - 1 + 'a');
	}
	else
	{
		// Convert to hex the easy way.
		ostringstream oss;
		oss.setf (std::ios::uppercase);
		oss << std::hex << c2;
		macro+= oss.str ();
	}
	TRDEB (macro);

	perlfunc (macro);
}

bool ScreenImpl::fastkey (int c)
{
	TRF;

	//if (Action * action= mapkeyaction.get (c) )
	if (Action * action= getKeyAction (c) )
	{
		action->doit ();
		return true;
	}
	return false;
}

void ScreenImpl::sendkey (int c)
{
	TRF;

	using namespace key;

	if (fastkey (c) )
		return;

	if (c <= 255 && (c >= 32 || c == '\t' || c == '\n') )
	{
		sendchar (c);
	}
}

void ScreenImpl::deleteline ()
{
	ptext->deleteline (lin () );

	textwindow_at (lin () - inilin () );
	textwindow_delline ();

	putline (maxlin);
	if (lin () >= ptext->lines () )
		up ();
	else
		adjustmaxcol ();
}

void ScreenImpl::replaceline (char *str, int line)
{
	TRF;

	size_t n= static_cast <size_t> (line);
	{
		TRDEBS ("Replacing line " << n << " with " << str);
	}
	ptext->setline (n, str);
	if (n == lin () )
		adjustmaxcol ();
	//putactualline ();
	if (n >= inilin () && n <= inilin () + maxlin)
		if (quietmode)
			invalidate_all_lines ();
		else
			putlineabs (n);
}

void ScreenImpl::do_interact ()
{
	TRF;

	int c;
	while (! fExit)
	{
		adjustinicol ();
		update ();

		TRDEB ("Waiting key");
		c= getkey ();
		if (! fExit)
			sendkey (c);
	}
}

void ScreenImpl::interact ()
{
	TRF;

	for (;;)
	{
		try
		{
			do_interact ();
			break;
		}
		catch (exception & e)
		{
			dialog_show_text ("Error", e.what (), 0, 50);
		}
		catch (const string & str)
		{
			dialog_show_text ("Error", str.c_str (), 0, 50);
		}
	}
}

void ScreenImpl::loadtext (FILE * fin)
{
	setstatus ("Loading stdin...");
	screen_invalidate ();
	status_update ();
	ptext->load (fin);
}

void ScreenImpl::loadtext (const string & strFileName)
{
	TRF;

	setstatus (string ("Loading ") + strFileName + "...");
	screen_invalidate ();
	status_update ();
	try
	{
		vector <string> args;
		args.push_back (strFileName);
		callperlfunc ("before_load", args);

		if (! ptext->load (strFileName) )
		{
			string dir= dirutil::dir_name (strFileName);
			TRDEB (dir);
			if (dir.empty () || dirutil::is_directory (dir) )
			{
				TRDEB ("New file");
				// Ignore error, the perl function
				// is not mandatory.
				qperl_function ("new_file", this);
				setstatus ("New file");
			}
			else
			{
				TRDEB ("Load error");
				dialog_show_text ("Error",
					"Directory not exist", 0, 50);
			}
		}
		else
		{
			TRDEB ("File loaded");
			setstatus ("File loaded");

			callperlfunc ("after_load", args);
		}
	}
	catch (exception & e)
	{
		TRDEB (string ("Exception: ") + e.what () );
		dialog_show_text ("Error", e.what (), 0, 50);
		status_clear ();
	}
	status_update ();
}

void ScreenImpl::definecontrolkey (char k, const string & function)
{
	TRF;

	if (k >= 'a' && k <= 'z')
		k-= 'a';
	else if (k >= 'A' && k<= 'Z')
		k-= 'A';
	else if (k < key::Ctl_A || k > key::Ctl_Z)
	{
		TRDEB ("Not a control char");
		return;
	}

	// Cast to char required here, or fails in gcc 1.95
	TRDEBS ("Defining 'control-" << char (k + 'A') <<
		"' as '" << function << '\'');

	auto_ptr <ScreenActionPerl> paction
		(new ScreenActionPerl (* this, function) );
	addKeyAction (k + 1, paction.get () );
	paction.release ();
}

void ScreenImpl::optionFileLoad ()
{
	TRF;

	if (! question_save () )
		return;
	string strFile;
	int k;
	set <int> keyender;
	keyender.insert (key::F5);
	keyender.insert (key::Ctl_S);
	bool r= dialog_get_string (k, keyender,
		"Open", "File name to open",
		strFile, historyfilename, 60);

	if (!r && keyender.find (k) != keyender.end () )
	{
		RedrawerScreen redrawer (* this);
		strFile= selectfile (get_textwindow (), "Load file", redrawer);
		if (! strFile.empty () )
			r= true;
	}
	if (r)
	{
		loadtext (strFile);
		inittext ();
	}
	textwindow_invalidate ();
}

void ScreenImpl::optionFileNew ()
{
	TRF;

	windownew ();
}

void ScreenImpl::optionFileSave ()
{
	TRF;

	do_save ();
}

void ScreenImpl::optionFileSaveAs ()
{
	TRF;

	do_save_as ();
}

void ScreenImpl::optionFileNext ()
{
	TRF;

	nextwindow ();
}

void ScreenImpl::optionFileClose ()
{
	TRF;

	closewindow ();
}

void ScreenImpl::optionFileQuit ()
{
	TRF;

	exiting= true;
	for (size_t i= 0, l= win.size (); i < l; ++i)
	{
		if (! closewindow () )
			return;
	}
	fExit= true;
}

void ScreenImpl::optionFileSelect ()
{
	TRF;

	vector <string> vstr;
	for (size_t i= 0; i < win.size (); ++i)
	{
		TextFile * pt= win [i].text ();
		if (! pt->hasname () )
			vstr.push_back ("(no name)");
		else
			vstr.push_back (pt->name () );
	}
	if (! vstr.empty () )
	{
		size_t r= selector ("Select file", vstr);
		if (r != size_t (-1) )
			changewindow (r);
	}
}

void ScreenImpl::optionFileOptions ()
{
	TRF;

	bool readonly= ptext->readonly ();
	bool utf8= ptext->getutf8 ();
	vdopts_t vdopts;
	vdopts.push_back (vdopts_t::value_type ("Read only", & readonly) );
	vdopts.push_back (vdopts_t::value_type ("Use utf8", & utf8) );

	if (dialog_options ("File options", vdopts) )
	{
		ptext->readonly (readonly);
		ptext->setutf8 (utf8);
	}
}

void ScreenImpl::optionEditSelect ()
{
	TRF;

	if (! fSelecting)
		beginselect ();
	else
		endselect ();
}

void ScreenImpl::optionEditCut ()
{
	TRF;

	cut ();
}

void ScreenImpl::optionEditCopy ()
{
	TRF;

	copy ();
}

void ScreenImpl::optionEditPaste ()
{
	TRF;

	paste ();
}

void ScreenImpl::optionEditDelete ()
{
	TRF;

	deleteselection ();
}

void ScreenImpl::optionEditSelectAll ()
{
	TRF;

	selectall ();
}

void ScreenImpl::optionEditCopyToNew ()
{
	TRF;

	auto_ptr <Text> clipaux;
	if (! selection ().empty () )
	{
		clipaux.reset (new Text (usingutf8 () ) );
		copyselection (* clipaux);
	}
	else
	{	if (pclipboard->empty () )
			throw runtime_error
				("Nothing to copy");
	}
	windownew ();
	if (clipaux.get () )
		swap (* clipaux, * pclipboard);
	paste ();
	if (clipaux.get () )
		swap (* clipaux, * pclipboard);
}

void ScreenImpl::optionEditOptions ()
{
	TRF;

	bool optautoindent= autoindent;
	bool optautocr= autocr;
	vdopts_t vdopts;
	vdopts.push_back
		(vdopts_t::value_type ("Auto indent", & optautoindent) );
	vdopts.push_back
		(vdopts_t::value_type ("Auto CR", & optautocr) );
	if (dialog_options ("Edit options", vdopts) )
	{
		autoindent= optautoindent;
		autocr= optautocr;
	}
}

void ScreenImpl::optionFindFind ()
{
	TRF;

	string newfind;
	bool r= dialog_get_string (
		"Find", "String to find",
		newfind, historyfind, 60);
	if (! r)
	{
		status_clear ();
		return;
	}

	strFind= newfind;
	SearchOptions options;
	r= askSearchOptions (options, false);
	if (! r)
		return;
	lastsearchoptions= options;

	buildfinder (options, strFind);

	optionFindAgain ();
}

void ScreenImpl::optionFindAgain ()
{
	TRF;

	if (! pfinder)
	{
		setstatus ("No previous search");
		return;
	}

	if (searchtext (strFind, lastsearchoptions) )
		setstatus ("Found");
	else
		setstatus ("Not found");
}

void ScreenImpl::optionFindReplace ()
{
	TRF;

	string newstring;
	bool r= dialog_get_string (
		"Replace", "Replace from",
		newstring, historyfind, 60);
	if (! r)
		return;

	strFind= newstring;

	newstring.erase ();
	r= dialog_get_string (
		"Replace to", "Replace to",
		newstring, historyreplace, 60);
	if (! r)
		return;

	strReplace= newstring;

	SearchOptions options;
	r= askSearchOptions (options, true);
	if (! r)
		return;
	lastsearchoptions= options;

	buildfinder (options, strFind);

	bool ask= true;
	size_t nfound= 0, nreplace= 0;
	for (bool looping= true; looping; )
	{
		size_t linebeginsearch= lin ();
		size_t posbeginsearch=
			(* ptext) [linebeginsearch].col2pos (col () ) + 1;

		if (! searchtext (strFind, options) )
			break;

		++ nfound;
		adjustinicol ();

		Selection & sel= win [actualwin].sel;
		if (sel.begin () != sel.end () )
			redraw_all_lines= true;
		size_t posendsel= (* ptext) [lin () ].col2pos (col () );

		size_t findsize= pfinder->length ();
		size_t posbeginsel= posendsel - findsize;
		sel.init (lin (), posbeginsel);

		sel.moveend (PosText (lin (), posendsel) );
		
		update ();

		char r= 'Y';
		if (ask)
		{
			r= status_question (
				"Replace? (Yes/No/All/One/Cancel):",
				"YyNnAaOoCc");
		}
		setstatus ("Replacing...");
		bool do_it= false;
		switch (r)
		{
		case 'c': case 'C':
			looping= false;
			break;
		case 'n': case 'N':
			break;
		case 'a': case 'A':
			ask= false;
			do_it= true;
			break;
		case 'y': case 'Y':
			do_it= true;
			break;
		case 'o': case 'O':
			do_it= true;
			looping= false;
		}
		if (do_it)
		{
			++nreplace;

			if (lin () != linebeginsearch)
				posbeginsearch= 0;

			const TextLine & t= (* ptext) [lin () ];
			string::size_type pos= t.col2pos (col () ) - findsize;

			// Temporary check.
			ASSERT (pos == posbeginsel);

			const string replacefrom=
				//t.str ().substr (posbeginsearch);
				t.str ();
			string replaced=
				options.function ?
				pfinder->replace (strReplace, replacefrom,
					pos, this) :
				pfinder->replace (strReplace, replacefrom);

			TRDEBS ("Replace by: '" << replaced << '\'' );

			ptext->replace (lin (), pos, findsize, replaced);
			pos+= replaced.size ();

			sel.moveend (PosText (lin (), pos) );

			if (looping)
				--pos; // searchtext begins in +1 position
			col ()= t.pos2col (pos);
			adjustinicol ();

			// TODO: better way to work here.
			putlineabs (lin () );

			update ();
		}
	}
	ostringstream oss;
	oss << "Replace: " << nfound << " found, " <<
		nreplace << " replaced";
	setstatus (oss.str () );
}

void ScreenImpl::optionGoBegin ()
{
	TRF;

	go_begin ();
}

void ScreenImpl::optionGoEnd ()
{
	TRF;

	go_end ();
}

void ScreenImpl::optionGoLine ()
{
	TRF;

	string strNum;
	static History history;
	bool r= dialog_get_string (
		"Go to", "Line number",
		strNum, history, 6, 6);
	if (r)
	{
		size_t num= atoi (strNum.c_str () );
		if (num == size_t (-1) || num == 0)
		{
			setstatus ("Invalid number");
			return;
		}
		if (num > ptext->lines () )
		{
			setstatus ("Out of text");
			return;
		}
		gotoline (num - 1);
		status_clear ();
	}
	else
		status_clear ();
}

void ScreenImpl::optionSystemShell ()
{
	TRF;

	mainwindow_clrscr ();
	otermstream::shellmode ();

	const char * strShell= getenv ("SHELL");
	if (strShell == NULL)
		strShell= "sh";
	TRDEBS ("Using shell " << strShell);

	system (strShell);

	otermstream::progmode ();

	screen_invalidate ();
}

void ScreenImpl::optionSystemExecute ()
{
	TRF;

	string strCommand;
	static History history;
	endselect ();
	bool r= dialog_get_string (
		"Execute", "Command to execute",
		strCommand, history, 60);
	if (! r)
	{
		status_clear ();
		return;
	}
	do_execute (strCommand);
}

void ScreenImpl::optionSystemExecuteLine ()
{
	TRF;

	string strCommand= (* ptext) [lin () ].str ();
	TRDEB (strCommand);
	const string::size_type l= strPrompt.size ();
	if (l)
	{
		if (strCommand.substr (0, l) == strPrompt)
			strCommand.erase (0, l);
		else
		{
			home ();
			instext (strPrompt.c_str () );
		}
	}
	end ();
	sendchar ('\n');
	do_execute (strCommand);
	if (l)
		instext (strPrompt.c_str () );
}

void ScreenImpl::optionSystemPrompt ()
{
	TRF;

	std::string newprompt;
	bool r= dialog_get_string (
		"Prompt", "Enter new prompt",
		newprompt, historyprompt, 60);
	if (r)
	{
		strPrompt= newprompt;
		setstatus ("Prompt set to: " + newprompt);
	}
}

void ScreenImpl::optionPerlEval ()
{
	TRF;

	string neweval;
	bool r= dialog_get_string (
		"Evaluate", "Write a perl expression",
		neweval, historyeval, 60);
	if (r)
	{
		strEval= neweval;
		status_clear ();

		// Just in case the evaluated expression wait
		// for a key or keeps running long time, to
		// not force to include an update call in it.
		update ();

		//if (qperl_eval (strEval.c_str (), this) != 0)
		if (callperleval (strEval) != 0)
			setstatus ("Error evaluating");
	}
}

void ScreenImpl::optionPerlEvalSelection ()
{
	TRF;

	Text evaltext (usingutf8 () );
	copyselection (evaltext);
	string str (evaltext.str () );

	//if (qperl_eval (str.c_str (), this) != 0)
	if (callperleval (str) != 0)
		setstatus ("Error evaluating selection");
}

void ScreenImpl::optionPerlEvalClipboard ()
{
	TRF;

	string str (pclipboard->str () );
	//if (qperl_eval (str.c_str (), this) != 0)
	if (callperleval (str) != 0)
		setstatus ("Error evaluating clipboard");
}

void ScreenImpl::optionPerlEvalLine ()
{
	TRF;

	string str= (* ptext) [lin () ].str ();
	end ();
	sendchar ('\n');
	//if (qperl_eval (str.c_str (), this) != 0)
	if (callperleval (str) != 0)
		setstatus ("Error evaluating line");
}

void ScreenImpl::optionPerlDefineControl ()
{
	TRF;

	string key;
	static History historykey;
	bool r= dialog_get_string (
		"Define control key", "Key to define",
		key, historykey, 1, 1);
	if (! r)
		return;
	if (key.empty () )
		return;
	char k= key [0];
	if (! isalpha (k) ||
			! (k >= 'A' && k >= 'Z') || ! (k>= 'a' && k<= 'z') )
		return;

	string function;
	static History historydef;
	r= dialog_get_string (
		"Define control key", "Function to execute",
		function, historydef, 60);
	if (! r)
		return;
	definecontrolkey (k, function);
}

void ScreenImpl::optionProgCompile ()
{
	if (! ptext->saved () && ! do_save () )
		return;
	string command ("gcc -c ");
	if (! strCompileOptions.empty () )
	{
		command+= strCompileOptions;
		command+= ' ';
	}
	command+= ptext->name ();
	windownew ();
	putall ();
	do_execute (command);
	ptext->modified (false);
	ptext->readonly (true);
}

void ScreenImpl::optionProgMake ()
{
	if (! do_save_all () )
		return;
	windownew ();
	putall ();
	do_execute ("make");
	ptext->modified (false);
	ptext->readonly (true);
}

void ScreenImpl::optionProgCompileOptions ()
{
	TRF;

	std::string newoptions;
	bool r= dialog_get_string (
		"Compile options",
		"Enter compile options",
		newoptions, historycompileoptions, 60);
	if (r)
	{
		strCompileOptions= newoptions;
		setstatus ("Compile options set to: " + newoptions);
	}
}

void ScreenImpl::optionProgManOf ()
{
	TRF;

	string strMan;
	bool r= dialog_get_string (
		"Man", "Man de",
		strMan, historyman, 60);
	if (! r || strMan.empty () )
		return;
	openman (strMan);
}

void ScreenImpl::optionProgMan ()
{
	TRF;

	string str= ptext->wordat (lin (), col () );
	if (str.empty () )
		return;
	openman (str);
}

void ScreenImpl::optionHelpIndex ()
{
	TRF;

	try
	{
		const size_t xmarg= 5, ymarg= 3;
		size_t w= columns ();
		size_t x1= xmarg;
		size_t x2= w - xmarg;
		if (x1 + 10 >= x2)
			throw 1;
		w= x2 - x1;
		size_t h= lines ();
		size_t y1= ymarg;
		size_t y2= h - ymarg;
		if (y1 + 5 >= y2)
			throw 1;
		h= y2 - y1;
		extern const char strHelp [];
		dialog_show_text ("Help", strHelp, h, w);
	}
	catch (...)
	{
		setstatus ("Error showing help");
	}
}

void ScreenImpl::optionHelpPerl ()
{
	TRF;

	perlfunchelp_show ();
}

void ScreenImpl::optionHelpAbout ()
{
	TRF;

	try
	{
		int width= 40;
		int w= columns ();
		int x= (w - width) / 2;
		if (x < 0) x= 0;
		if (width > w) width= w;
		size_t h= lines ();
		const size_t height= 9;
		if (h < height)
			throw 1;
		size_t y= (h - height) / 2;
		otermstream ots (get_textwindow (), height, width, y + 1, x);
		ots.cursor (false);
		ots << clrscr;
		ots.border ();
		centertext (ots, 0, width, strAbout1);
		centertext (ots, 2, width, strAbout2);
		centertext (ots, 3, width, strAbout3);
		centertext (ots, 4, width, strVersion);
		centertext (ots, 6, width, "Press any key");
		ots.getkey ();
		ots.cursor (true);
	}
	catch (...) { }
	putall ();
}

void ScreenImpl::optionQuitOrClose ()
{
	TRF;

	if (win.size () == 1)
		optionFileQuit ();
	else
		closewindow ();
}

//*********************************************************
//		qpStringParam aux functions
//*********************************************************

bool ScreenImpl::string_to_qpstringparam
	(const string & str, qpStringParam & param)
{
	TRF;

	param.str= new (nothrow) char [str.size () + 1];
	if (param.str == NULL)
		return false;

	try
	{
		qpstring_alloc.insert (param.str);
	}
	catch (...)
	{
		delete [] param.str;
		throw;
	}

	param.len= str.size ();
	str.copy (param.str, str.size () );
	param.str [str.size () ]= '\0';
	return true;
}

void ScreenImpl::free_qpstringparam (qpStringParam & param)
{
	TRF;

	qpstring_alloc_t::iterator it= qpstring_alloc.find (param.str);
	if (it == qpstring_alloc.end () )
		throw logic_error ("Trying to delete invalid qpStringParam");

	qpstring_alloc.erase (it);
	delete [] param.str;
}

namespace {

struct KeyAction;

class ScreenActionMapKey : public ScreenAction, public MapKeyAction
{
public:
	ScreenActionMapKey (ScreenImpl & screen, KeyAction * ka, size_t n);
	void doit ();
};

struct KeyAction {
	int key;
	union {
		void (ScreenImpl::*function) ();
		const char * perlfunc;
		KeyAction * pka;
	};
	size_t n;
	enum TypeAction { Function, Perl, Key };
	TypeAction type;
	KeyAction (int key, void (ScreenImpl::* function) () ) :
		key (key), function (function), type (Function)
	{ }
	KeyAction (int key, const char * perlfunc) :
		key (key), perlfunc (perlfunc), type (Perl)
	{ }
	KeyAction (int key, KeyAction * pka, size_t n) :
		key (key), pka (pka), n (n), type (Key)
	{ }
	ScreenAction * action (ScreenImpl & screen) const
	{
		TRF;

		ScreenAction * a;
		switch (type)
		{
		case Function:
			a= new ScreenActionFunction (screen, function);
			break;
		case Perl:
			a= new ScreenActionPerl (screen, perlfunc);
			break;
		case Key:
			a= new ScreenActionMapKey (screen, pka, n);
			break;
		default:
			a= 0;
		}
		return a;
	}
};

class KeyActionAdder {
public:
	KeyActionAdder (ScreenImpl & screen, MapKeyAction & mka) :
		screen (screen),
		mka (mka)
	{ }
	void operator () (const KeyAction & ka)
	{
		mka.addKeyAction (ka.key, ka.action (screen) );
	}
private:
	ScreenImpl & screen;
	MapKeyAction & mka;
};

ScreenActionMapKey::ScreenActionMapKey (ScreenImpl & screen,
		KeyAction * ka, size_t n) :
	ScreenAction (screen)
{
	for_each (ka, ka + n, KeyActionAdder (screen, * this) );
}

void ScreenActionMapKey::doit ()
{
	int key= screen.statusgetkey ("");
	if (key >= 'A' && key <= 'Z')
		key= tolower (key);
	Action * action= getKeyAction (key);
	if (action)
		action->doit ();
}

KeyAction progkeyaction []= {
	KeyAction ('c',            & ScreenImpl::optionProgCompile),
	KeyAction ('m',            & ScreenImpl::optionProgMake),
	KeyAction ('o',            & ScreenImpl::optionProgCompileOptions),
};

KeyAction keyaction []= {
	KeyAction (key::Home,      "home"),
	KeyAction (key::End,       "end"),
	KeyAction (key::Up,        "up"),
	KeyAction (key::Down,      "down"),
	KeyAction (key::Left,      "left"),
	KeyAction (key::Right,     "right"),
	KeyAction (key::PageUp,    "pageup"),
	KeyAction (key::PageDown,  "pagedown"),
	KeyAction (key::Delete,    "delete_char"),
	KeyAction (key::Backspace, "backspace"),
	KeyAction (0x7F,           "backspace"),
	KeyAction (key::Select,    "toggle_select"),

	KeyAction (key::Ctl_A,     & ScreenImpl::optionFindAgain),
	KeyAction (key::Ctl_C,     "copy"),
	KeyAction (key::Ctl_E,     & ScreenImpl::optionPerlEval),
	KeyAction (key::Ctl_F,     & ScreenImpl::optionFindFind),
	KeyAction (key::Ctl_G,     & ScreenImpl::optionGoLine),
	KeyAction (key::Ctl_H,     "backspace"),
	KeyAction (key::Ctl_K,     "macro"),
	KeyAction (key::Ctl_L,     "redraw"),
	KeyAction (key::Ctl_N,     & ScreenImpl::menu_select),
	KeyAction (key::Ctl_O,     progkeyaction, dim_array (progkeyaction) ),
	KeyAction (key::Ctl_Q,     "file_quit"),
	KeyAction (key::Ctl_R,     & ScreenImpl::optionFindReplace),
	KeyAction (key::Ctl_S,     "toggle_select"),
	KeyAction (key::Ctl_V,     "paste"),
	KeyAction (key::Ctl_X,     "cut"),
	KeyAction (key::Ctl_Y,     "delete_line"),

	KeyAction (key::F1,        & ScreenImpl::optionHelpIndex),
	KeyAction (key::F2,        "file_save"),
	KeyAction (key::F3,        & ScreenImpl::optionProgMan),
	KeyAction (key::F5,        "file_next"),
	KeyAction (key::F7,        & ScreenImpl::optionProgMake),
	KeyAction (key::F8,        & ScreenImpl::optionQuitOrClose),
	KeyAction (key::F9,        & ScreenImpl::optionSystemExecuteLine),
	KeyAction (key::F10,       & ScreenImpl::menu_select),
};

} // namespace

void ScreenImpl::init_actions ()
{
	TRF;

	for_each (keyaction, keyaction + dim_array (keyaction),
		KeyActionAdder (* this, * this) );
}

//***********************************************
//		Screen creation.
//***********************************************

Screen *  createScreen (bool useutf8, char * * env)
{
	TRF;

	return new ScreenImpl (useutf8, env);
}

//***********************************************
//		Callback from qperl
//***********************************************

qpRetCode qpCallback (void * thisclass, qpFuncCode f, int n, void * data)
{
	TRFDEB (nameqpFunc (f) );

	ScreenImpl *p= static_cast <ScreenImpl *> (thisclass);

	// Use assert here, not ASSERT.
	assert (Screen::screenpointervalid (p) );

	// Ensure no exception is propagated back to qperl.

	try
	{
		qpRetCode r= p->callback (f, n, data);
		TRDEBS ("Returning " << r);
		return r;
	}
	catch (exception & e)
	{
		p->showerror (e.what () );
	}
	catch (...)
	{
		p->showerror ("Unexpected exception");
	}
	TRDEB ("Returning -1");
	return qpRetUnexpected;
}

// End of screenimpl.cpp

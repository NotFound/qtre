// screenbase.cpp
// Revision 29-jul-2009

#include "screenbase.h"

#include "dialog.h"
#include "select.h"

#include "trace.h"
#undef ASSERT
#include "qtreassert.h"

#include <string>
#include <sstream>
#include <vector>

using std::string;
using std::vector;

#include <stdlib.h>

//***********************************************
//		ScreenBase
//***********************************************

ScreenBase::ScreenBase (bool useutf8) :
	mainwindow (),
	textwindow (mainwindow.lines () - 2, mainwindow.columns (), 1, 0),
	menuwindow (1, mainwindow.columns (), 0, 0),
	status (mainwindow.columns (), mainwindow.lines () - 1),
	flagutf8 (useutf8),
	text_inited (false),
	redraw_text_window (true),
	redraw_menu (true)
{
	TRF;

	mainwindow.setcolor (1, Color::Cyan, Color::Black);
	setmode (0);
	status.settext ("Status bar");
	menuwindow.color (1);
}

ScreenBase::~ScreenBase ()
{
	TRF;
}

bool ScreenBase::usingutf8 () const
{
	return flagutf8;
}

void ScreenBase::set_text_inited ()
{
	text_inited= true;
}

otermstream & ScreenBase::get_textwindow ()
{
	return textwindow;
}

void ScreenBase::mainwindow_clrscr ()
{
	mainwindow << clrscr;
}

void ScreenBase::screen_invalidate ()
{
	menu_invalidate ();
	textwindow_invalidate ();
	status_invalidate ();
}
void ScreenBase::textwindow_invalidate ()
{
	redraw_text_window= true;
	if (text_inited)
		invalidate_all_lines ();
}

void ScreenBase::textwindow_update ()
{
	if (redraw_text_window)
	{
		textwindow.clrscr ();
		redraw_text_window= false;
	}
}

bool ScreenBase::setmode (int n)
{
	switch (n)
	{
	case 0:
		textwindow.setcolor (2, Color::White, Color::Black);
		textwindow.color (2);
		textwindow.background (2);
		return true;
	case 1:
		textwindow.setcolor (2, Color::Black, Color::White);
		textwindow.color (2);
		textwindow.background (2);
		return true;
	default:
		return false;
	}
}

int ScreenBase::getkey ()
{
	TRF;

	//return textwindow.getkey ();
	int k= textwindow.getkey ();
	if (flagutf8)
	{
		unsigned int uk (k);

		TRDEBS (std::hex << uk);

		if ( (uk & 0xFC) == 0xC0)
		{
			TRDEB ("is utf8");
			int k2= textwindow.getkey ();
			unsigned char uk2 (k2);
			uk= ( (uk & 0x03) << 6) | (uk2 & 0x3F);
			k= uk;
		}
	}
	return k;
}

int ScreenBase::checkkey ()
{
	return textwindow.checkkey ();
}

//size_t ScreenBase::maxlin () const
size_t ScreenBase::lines ()
{
	//return textwindow.getHeight () - 1;
	return textwindow.lines ();
}

//Col ScreenBase::maxcol () const
size_t ScreenBase::columns ()
{
	//return Col (textwindow.getWidth () - 1);
	return textwindow.columns ();
}

void ScreenBase::textwindow_at (size_t lin, Col col)
{
	textwindow << at (lin, col.getn () );
}

void ScreenBase::textwindow_clreol ()
{
	const char * fill;
	if ( (fill= getenv ("QTRE_FILL_SPACE") ) == NULL)
	{
		textwindow << clreol;
	}
	else
	{
		char c= fill [0];
		if (c == '\0')
			c= ' ';
		size_t cur= columns () - textwindow.wherex ();
		textwindow << string (cur, c) << std::flush;
	}
}

void ScreenBase::textwindow_inverse (bool flag)
{
	textwindow << inverse (flag);
}

void ScreenBase::textwindow_insline ()
{
	textwindow << insline;
}

void ScreenBase::textwindow_delline ()
{
	textwindow << delline;
}

void ScreenBase::outchar (char c)
{
	//TRF;

	if (flagutf8)
	{
		const unsigned int uc (static_cast <unsigned char> (c) );
		if (uc & 0x80)
		{
			const unsigned int uc1 (0xC0 | ( (uc & 0xC0) >> 6) );
			const unsigned int uc2 ( (uc & 0x3F) | 0x80);

			//TRDEBS (std::hex << uc1 << ':' << std::hex << uc2);

			char c1 (static_cast <char> (static_cast <unsigned char> (uc1) ) );
			char c2 (static_cast <char> (static_cast <unsigned char> (uc2) ) );
			textwindow << c1 << c2;
		}
		else
		{
			textwindow << c;
		}
	}
	else
	{
		textwindow << c;
	}
}

void ScreenBase::outstring (const std::string & str)
{
	//textwindow << str;
	for (size_t i= 0; i < str.size (); ++i)
		outchar (str [i] );
}

size_t ScreenBase::selector (const std::string & title,
	const std::vector <std::string> & vstr)
{
	update ();
	size_t r= selectlist (textwindow, title, vstr);
	textwindow_invalidate ();
	return r;
}

void ScreenBase::dialog_show_text (const char * title, const char * str,
	int height, int width)
{
	update ();
	dialogShowText (textwindow, title, str, height, width);
	textwindow_invalidate ();
}

bool ScreenBase::dialog_get_string (
	int & keyend, const std::set <int> & keyender,
	const std::string & title, const std::string & prompt,
	std::string & str, History & history,
	size_t width, std::string::size_type maxlength)
{
	update ();
	bool r= dialogGetString (textwindow,
		keyend, keyender,
		title, prompt, str,
		history, width, maxlength);
	textwindow_invalidate ();
	return r;
}

bool ScreenBase::dialog_get_string (
	const std::string & title, const std::string & prompt,
	std::string & str, History & history,
	size_t width, std::string::size_type maxlength)
{
	update ();
	bool r= dialogGetString (textwindow,
		title, prompt, str,
		history, width, maxlength);
	textwindow_invalidate ();

	return r;
}

ScreenBase::dialog_result ScreenBase::dialog_question
	(const std::string & str)
{
	//update ();
	dialogResult r= dialogQuestion (textwindow, str);
	textwindow_invalidate ();

	switch (r)
	{
	case dialogYes: return dialog_yes;
	case dialogNo:  return dialog_no;
	default: break;
	}
	return dialog_cancel;
}

bool ScreenBase::dialog_options (const std::string & title, vdopts_t vdopts)
{
	update ();
	DialogOptions dialog (textwindow, title);
	for (size_t i= 0; i < vdopts.size (); ++i)
		dialog.add (vdopts [i].first, * vdopts [i].second);
	bool r= dialog.do_it ();
	textwindow_invalidate ();
	return r;
}

void ScreenBase::menu_show ()
{
	menu.show (menuwindow);
}

void ScreenBase::menu_invalidate ()
{
	redraw_menu= true;
}

void ScreenBase::menu_update ()
{
	if (redraw_menu)
	{
		menu_show ();
		redraw_menu= false;
	}
}

void ScreenBase::menu_add_option (Option * popt)
{
	menu.addOption (popt);
	menu_invalidate ();
}

void ScreenBase::menu_insert_option (Option * pop, size_t position)
{
	menu.insertOption (pop, position);
	menu_invalidate ();
}

bool ScreenBase::menu_insert_suboption (const std::string & submenuname,
	Option * pop, size_t position)
{
	Menu * pmenu= menu.getSubMenu (submenuname);
	if (pmenu == NULL)
		return false;
	pmenu->insertOption (pop, position);
	return true;
}

bool ScreenBase::menu_rename (const std::string & from, const std::string & to)
{
	bool r= menu.rename (from, to);
	if (r)
		menu_invalidate ();
	return r;
}

void ScreenBase::menu_select ()
{
	TRF;

	Action * action= menu.selectoption (menuwindow);
	if (action)
		action->doit ();
}

void ScreenBase::status_invalidate ()
{
	status.invalidate ();
}

void ScreenBase::status_update ()
{
	status.update ();
}

void ScreenBase::status_lincol (size_t lin, Col col)
{
	status.lincol (lin, col.getn () );
}

void ScreenBase::status_clear ()
{
	status.clear ();
}
void ScreenBase::setstatus (const std::string & str)
{
	status.settext (str);
}

int ScreenBase::statusgetkey (const std::string & str)
{
	return status.getkey (str);
}

char ScreenBase::status_question
	(const std::string & q, const std::string & values)
{
	return status.question (q, values);
}

void ScreenBase::perlfunchelp_set (const char * func, const char * helptext)
{
	perlfunchelp [func]= helptext;
}

void ScreenBase::perlfunchelp_show ()
{
	vector <string> vstr;
	for (perlfunchelp_t::iterator it= perlfunchelp.begin ();
		it != perlfunchelp.end ();
		++it)
	{
		vstr.push_back (it->first);
	}
	if (vstr.empty () )
		return;
	for (;;)
	{
		size_t r= selector ("Select function", vstr);

		if (r == size_t (-1) )
			break;

		dialog_show_text (
			vstr [r].c_str (),
			perlfunchelp [vstr [r] ].c_str (),
			2 * lines () / 3,
			2 * columns () / 3);
	}
}

void ScreenBase::update ()
{
	TRF;

	menu_update ();

	if (text_inited)
		status_current_lincol ();
	status_update ();

	textwindow_update ();

	if (text_inited)
		update_lines ();
}

// End of screenbase.cpp

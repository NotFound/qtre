#ifndef INCLUDE_SCREENBASE_H
#define INCLUDE_SCREENBASE_H

// screenbase.h
// Revision 25-jan-2006

#include "screen.h"

#include "termstream.h"
#include "text.h"
#include "menu.h"
#include "status.h"
#include "history.h"

#include <string>
#include <set>

//***********************************************
//		ScreenBase
//***********************************************

class ScreenBase : public Screen
{
protected:
	ScreenBase (bool useuft8);
	virtual ~ScreenBase ();

	void set_text_inited ();

	otermstream & get_textwindow ();

	void mainwindow_clrscr ();

	void screen_invalidate ();
	virtual void invalidate_all_lines ()= 0;
	virtual void update_lines ()= 0;
public:
	void textwindow_invalidate ();
protected:
	bool usingutf8 () const;
	void textwindow_update ();
	bool setmode (int n);
	int getkey ();
	int checkkey ();
	//size_t maxlin () const;
	//Col maxcol () const;
	size_t lines ();
	size_t columns ();
	void textwindow_at (size_t lin, Col col= Col () );
	void textwindow_clreol ();
	void textwindow_inverse (bool flag);
	void textwindow_insline ();
	void textwindow_delline ();
	void outchar (char c);
	void outstring (const std::string & str);

	size_t selector (const std::string & title,
		const std::vector <std::string> & vstr);

	void dialog_show_text (const char * title, const char * str,
		int height, int width);
	bool dialog_get_string (
		int & keyend, const std::set <int> & keyender,
		const std::string & title, const std::string & prompt,
		std::string & str, History & history,
		size_t width,
		std::string::size_type maxlength= std::string::npos);
	bool dialog_get_string (
		const std::string & title, const std::string & prompt,
		std::string & str, History & history,
		size_t width,
		std::string::size_type maxlength= std::string::npos);
	enum dialog_result { dialog_cancel, dialog_yes, dialog_no };
	dialog_result dialog_question (const std::string & str);

	typedef std::vector <std::pair <std::string, bool *> > vdopts_t;
	bool dialog_options (const std::string & title, vdopts_t vdopts);

	void menu_invalidate ();
	void menu_update ();
public:
	void menu_add_option (Option * popt);
protected:
	void menu_insert_option (Option * pop, size_t position);
	bool menu_insert_suboption (const std::string & submenuname,
		Option * pop, size_t position);
	bool menu_rename (const std::string & from, const std::string & to);
public:
	void menu_select ();

protected:
	void status_invalidate ();
	void status_update ();
	virtual void status_current_lincol ()= 0;
	void status_lincol (size_t lin, Col col);
	void status_clear ();
	void setstatus (const std::string & str);
public:
	int statusgetkey (const std::string & str);
protected:
	char status_question
		(const std::string & q, const std::string & values);

	void perlfunchelp_set (const char * func, const char * helptext);
	void perlfunchelp_show ();
public:
	void update ();
private:
	otermstream mainwindow;
	otermstream textwindow;
	Menu menu;
	otermstream menuwindow;
	StatusLine status;
	bool flagutf8;
	bool text_inited;
	bool redraw_text_window;
	bool redraw_menu;

	typedef std::map <std::string, std::string> perlfunchelp_t;
	perlfunchelp_t perlfunchelp;

	void menu_show ();
};

#endif

// End of screenbase.h

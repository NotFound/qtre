#ifndef INCLUDE_SCREENIMPL_H
#define INCLUDE_SCREENIMPL_H

// screenimpl.h
// Revision 7-sep-2006

#include "screen.h"
#include "screenbase.h"
#include "text.h"
#include "textwindow.h"
#include "exec.h"
#include "qperl.h"
#include "finder.h"
#include "linkperl.h"

#include <string>
#include <set>
#include <memory>

class ScreenImpl;

class MenuBuilder {
protected:
	MenuBuilder (ScreenImpl & screen);
	~MenuBuilder ();
};

#if 0
class PerlInitializer {
public:
	PerlInitializer (char * * env, Screen * pscreen);
	~PerlInitializer ();
};
#endif

class ScreenImpl : public ScreenBase,
	public MapKeyAction,
	private Exec,
	private MenuBuilder,
	private LinkPerl
{
public:
	ScreenImpl (bool useutf8, char * * env);
	~ScreenImpl ();

	void windownew ();
	size_t textlines () const { return ptext->lines (); }
	void deleteselection ();
	void gotoline (size_t l);
	void gotoline (size_t l, Col gotocol);
	void putall ();
	void go_begin ();
	void go_end ();

	void optionFileLoad ();
	void optionFileNew ();
	void optionFileSave ();
	void optionFileSaveAs ();
	void optionFileNext ();
	void optionFileClose ();
	void optionFileQuit ();
	void optionFileSelect ();
	void optionFileOptions ();

	void optionEditSelect ();
	void optionEditCut ();
	void optionEditCopy ();
	void optionEditPaste ();
	void optionEditDelete ();
	void optionEditSelectAll ();
	void optionEditCopyToNew ();
	void optionEditOptions ();

	void optionFindFind ();
	void optionFindAgain ();
	void optionFindReplace ();

	void optionGoBegin ();
	void optionGoEnd ();
	void optionGoLine ();

	void optionSystemShell ();
	void optionSystemExecute ();
	void optionSystemExecuteLine ();
	void optionSystemPrompt ();

	void optionPerlEval ();
	void optionPerlEvalSelection ();
	void optionPerlEvalClipboard ();
	void optionPerlEvalLine ();
	void optionPerlDefineControl ();

	void optionProgCompile ();
	void optionProgMake ();
	void optionProgCompileOptions ();
	void optionProgMan ();
	void optionProgManOf ();

	void optionHelpIndex ();
	void optionHelpPerl ();
	void optionHelpAbout ();

	// Options not in default menu.
	void optionQuitOrClose ();

	void interact ();
	void loadtext (FILE * fin);
	void loadtext (const std::string & strFileName);
	void changewindow (size_t newwin);
	void showerror (const char * str);

	void perlfunc (const std::string & func);

	qpRetCode callback (qpFuncCode f, int n, void * data);

private:
	//Screen & screen;

	typedef std::vector <TextWindow> ContainerWindow;

	ContainerWindow win;
	size_t actualwin;

	TextFile * ptext;

	std::auto_ptr <Text> pclipboard;
	//StatusLine & status;
	//otermstream & ot;
	//otermstream & mainwindow;
	bool autoindent;
	bool autocr;
	size_t maxlin;
	Col maxcol;
	bool redraw_all_lines;
	//bool redraw_text_window;
	bool fSelecting;
	bool exiting;
	bool quietmode;

	std::string strFind;
	std::string strReplace;
	Finder * pfinder;
	History historyfind;
	History historyreplace;
	History historysearchoptions;
	SearchOptions lastsearchoptions;

	std::string strEval;
	History historyeval;

	History historyfilename;

	std::string strPrompt;
	History historyprompt;

	std::string strCompileOptions;
	History historycompileoptions;

	History historyman;

	void invalidate_all_lines ();
	void update_lines ();
	void status_current_lincol ();

	// Exec virtual functions.
	void ExecSendChar (char c);
	bool ExecSendBlock (const char * str, size_t size);
	bool ExecQueryAbort ();

	Selection & selection () { return win [actualwin].sel; }
	void redraw ();
	void nextwindow ();
	size_t & lin () { return win [actualwin].lin; }
	Col & col () { return win [actualwin].col; }
	size_t getpos () { return (* ptext) [lin () ].col2pos (col () ); }
	size_t & inilin () { return win [actualwin].inilin; }
	Col & inicol () { return win [actualwin].inicol; }
	void beginselect ();
	void endselect ();
	void selectall ();

	void copyselection (Text & dest);
	void copyselection ();
	//void copyselection (char * str);
	//bool copyselection (char * & str, size_t & len);

	void copyclipboard (Text & dest);
	//void copyclipboard (char * str);
	//bool copyclipboard (char * & str, size_t & len);

	void cut ();
	void copy ();
	void paste ();
	char getcurchar ();
	void getline (int nline, char * str);
	bool getline (int nline, char * & str, size_t & len);
	void getfileext (int n, char * str);
	void getfilename (int n, char * str);
	void instext (const char * str);
	void do_instext (const char * str, size_t len);
	void instext (const char * str, size_t len);
	void putline (size_t winline);
	inline void putline (const PosText & pt);
	inline void putlineabs (size_t i);
	inline void putactualline ();
	void scrollup ();
	void scrolldown ();
	void adjustmaxcol ();
	void movesel (const PosText & opt, size_t lin, size_t pos);
	void home ();
	void end ();
	bool up ();
	void move_up ();
	void pageup ();
	bool down ();
	void move_down ();
	void pagedown ();
	bool back ();
	void move_left ();
	bool forward ();
	void move_right ();
	void deletechar ();
	void backdelete ();
	void inschar (unsigned char c);
	void sendnewline ();
	void sendchar (unsigned char c);
	void adjustinicol ();
	void adjustinilin ();
	bool do_save ();
	bool do_save_as ();
	bool do_save_all ();
	void do_execute (const std::string & str);
	bool question_save ();
	bool askSearchOptions (SearchOptions & options, bool replace);
	void buildfinder (const SearchOptions & options,
		const std::string & strFind);
	bool searchtext (const std::string & searched, SearchOptions options);
	void openman (const std::string & str);
	void macro ();

	bool fastkey (int c);
	void definecontrolkey (char k, const std::string & function);
	void sendkey (int c);
	void deleteline ();
	void replaceline (char * str, int line);
	void inittext ();
	ContainerWindow initwin ();
	bool closewindow ();
	void init_actions ();
	void do_interact ();

	// qpStringParam aux functions

	typedef std::set <char *> qpstring_alloc_t;
	qpstring_alloc_t qpstring_alloc;

	bool string_to_qpstringparam
		(const std::string & str, qpStringParam & param);
	void free_qpstringparam (qpStringParam & param);
};

//***********************************************
//		ScreenAction
//***********************************************

class ScreenAction : public Action {
public:
	ScreenAction (ScreenImpl & screen) :
		screen (screen)
	{ }
protected:
	ScreenImpl & screen;
};

class ScreenActionFunction : public ScreenAction {
public:
	ScreenActionFunction (ScreenImpl & screen,
			void (ScreenImpl::* function) () ) :
		ScreenAction (screen),
		function (function)
	{ }
	void doit ();
private:
	void (ScreenImpl::* function) ();
};

class ScreenActionPerl : public ScreenAction {
public:
	ScreenActionPerl (ScreenImpl & screen,
			const std::string & function) :
		ScreenAction (screen),
		function (function)
	{ }
	void doit ();
private:
	const std::string function;
};

#endif

// End of screenimpl.h

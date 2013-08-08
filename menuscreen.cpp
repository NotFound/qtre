// menuscreen.cpp
// Revision 27-jan-2006

#include "screenimpl.h"
#include "menu.h"

#include "util.h"

#include "trace.h"
#undef ASSERT
#include "qtreassert.h"

#include <memory>
#include <algorithm>

using std::auto_ptr;
using std::for_each;

using util::dim_array;

namespace {

struct MenuOption {
	const char * str;
	union Func {
		void (ScreenImpl::*function) ();
		const char * perlfunc;
		Func (void (ScreenImpl::*function) () ) :
			function (function)
		{ }
		Func (const char * perlfunc) :
			perlfunc (perlfunc)
		{ }
	};
	Func func;
	enum TypeOption { Function, Perl };
	TypeOption type;

	MenuOption (const char * str, void (ScreenImpl::* function) () ) :
		str (str), func (function), type (Function)
	{ }
	MenuOption (const char * str, const char * perlfunc) :
		str (str), func (perlfunc), type (Perl)
	{ }
	Option * option (ScreenImpl & screen) const
	{
		auto_ptr <ScreenAction> paction;
		switch (type)
		{
		case Function:
			paction.reset (new ScreenActionFunction
				(screen, func.function) );
			break;
		case Perl:
			paction.reset (new ScreenActionPerl
				(screen, func.perlfunc) );
			break;
		default:
			break;
		}
		Option * popt= new Option (str, paction.get () );
		paction.release ();
		return popt;
	}
};

MenuOption
mopFile []= {
	MenuOption ("&Open",                  "file_load"),
	MenuOption ("&New",                   "file_new"),
	MenuOption ("&Save",                  "file_save"),
	MenuOption ("save &As",               "file_save_as"),
	MenuOption ("nex&T",                  "file_next"),
	MenuOption ("&Close",                 "file_close"),
	MenuOption ("e&Xit",                  "file_quit"),
	MenuOption ("s&Elect",                & ScreenImpl::optionFileSelect),
	MenuOption ("op&Tions",               & ScreenImpl::optionFileOptions),
},

mopEdit []= {
	MenuOption ("&Select",                "toggle_select"),
	MenuOption ("cu&T",                   "cut"),
	MenuOption ("&Copy",                  "copy"),
	MenuOption ("&Paste",                 "paste"),
	MenuOption ("&Delete",                "delete_selection"),
	MenuOption ("sel. &Todo",             "select_all"),
	MenuOption ("copy to &New",           "copy_new_file"),
	MenuOption ("&Options",               & ScreenImpl::optionEditOptions),
},
mopFind []= {
	MenuOption ("&Find",                  & ScreenImpl::optionFindFind),
	MenuOption ("find &Again",            & ScreenImpl::optionFindAgain),
	MenuOption ("&Replace",               & ScreenImpl::optionFindReplace),
},
mopGo []= {
	MenuOption ("&Begin",                 & ScreenImpl::optionGoBegin),
	MenuOption ("&End",                   & ScreenImpl::optionGoEnd),
	MenuOption ("line &Number",           & ScreenImpl::optionGoLine),
},
mopSystem []= {
	MenuOption ("&Shell",                 & ScreenImpl::optionSystemShell),
	MenuOption ("&Execute",               & ScreenImpl::optionSystemExecute),
	MenuOption ("execute &Line",          & ScreenImpl::optionSystemExecuteLine),
	MenuOption ("&Prompt",                & ScreenImpl::optionSystemPrompt),
},
mopPerl []= {
	MenuOption ("&Evaluate",              & ScreenImpl::optionPerlEval),
	MenuOption ("evaluate &Selection",    & ScreenImpl::optionPerlEvalSelection),
	MenuOption ("evaluate &Clipboard",    & ScreenImpl::optionPerlEvalClipboard),
	MenuOption ("evaluate &Line",         & ScreenImpl::optionPerlEvalLine),
	MenuOption ("&Define control",        & ScreenImpl::optionPerlDefineControl),
},
mopProgram []= {
	MenuOption ("&Compile",               "compile"),
	MenuOption ("&Make",                  "make"),
	MenuOption ("&Options",               "compile_options"),
	MenuOption ("m&An",                   "man"),
	MenuOption ("man o&F",                "man_of"),
},
mopHelp []= {
	MenuOption ("&Index",                 "help"),
	MenuOption ("&Perl functions",        "help_perl"),
	MenuOption ("&About",                 "help_about"),
};

class MenuOptionAdder {
public:
	MenuOptionAdder (ScreenImpl & screen, Menu & menu);
	void operator () (const MenuOption & mop);
private:
	ScreenImpl & screen;
	Menu & menu;
};


MenuOptionAdder::MenuOptionAdder (ScreenImpl & screen, Menu & menu) :
	screen (screen), menu (menu)
{
	TRF;
}

void MenuOptionAdder::operator () (const MenuOption & mop)
{
	TRF;

	auto_ptr <Option> popt (mop.option (screen) );
	menu.addOption (popt.get () );
	popt.release ();
}

Menu * createSub (const MenuOption * mop, size_t nopts, ScreenImpl & screen)
{
	TRF;

	auto_ptr <Menu> pmenu (new Menu);
	Menu & menu= * pmenu.get ();
	for_each (mop, mop + nopts, MenuOptionAdder (screen, menu) );
	Menu * aux= pmenu.get ();
	pmenu.release ();
	return aux;
}

struct MenuList {
	const char * str;
	const MenuOption * mop;
	size_t nops;
	Option * option (ScreenImpl & screen) const
	{
		auto_ptr <Menu> psubmenu (createSub (mop, nops, screen) );
		Option * popt= new Option (str, psubmenu.get () );
		psubmenu.release ();
		return popt;
	}
};

MenuList menulist []= {
	{ "&File",         mopFile,    dim_array (mopFile) },
	{ "&Edit",         mopEdit,    dim_array (mopEdit) },
	{ "&Find",         mopFind,    dim_array (mopFind) },
	{ "&Go",           mopGo,      dim_array (mopGo) },
	{ "&System",       mopSystem,  dim_array (mopSystem) },
	{ "&Perl",         mopPerl,    dim_array (mopPerl) },
	{ "p&Rogramming",  mopProgram, dim_array (mopProgram) },
	{ "&Help",         mopHelp,    dim_array (mopHelp) },
};

class MenuAdder {
public:
	MenuAdder (ScreenImpl & screen);
	~MenuAdder ();
	void operator () (const MenuList & ml);
private:
	ScreenImpl & screen;
};

MenuAdder::MenuAdder (ScreenImpl & screen) :
	screen (screen)
{
	TRF;
}

MenuAdder::~MenuAdder ()
{
	TRF;
}

void MenuAdder::operator () (const MenuList & ml)
{
	TRF;

	auto_ptr <Option> popt (ml.option (screen) );
	screen.menu_add_option (popt.get () );
	popt.release ();
}

void createMenu (ScreenImpl & screen)
{
	TRF;

	for_each (menulist, menulist + dim_array (menulist),
		MenuAdder (screen) );
}

} // namespace

//***********************************************
//		MenuBuilder
//***********************************************

MenuBuilder::MenuBuilder (ScreenImpl & screen)
{
	TRF;

	createMenu (screen);
}

MenuBuilder::~MenuBuilder ()
{
	TRF;
}

//***********************************************
//		ScreenAction
//***********************************************

void ScreenActionFunction::doit ()
{
	(screen.*function) ();
}

void ScreenActionPerl::doit ()
{
	screen.perlfunc (function);
}

// End of menuscreen.cpp

// menu.cpp
// Revision 29-jan-2006

#include <stdexcept>
#include <algorithm>

#include "menu.h"
#include "util.h"

#include "trace.h"
#undef ASSERT
#include "qtreassert.h"

using std::string;

using util::to_string;

//*********************************************************
//			Option
//*********************************************************

namespace {

class OptionInstanceCounter {
public:
	OptionInstanceCounter ();
	~OptionInstanceCounter ();
	void operator ++ () { ++count; }
	void operator -- ();
private:
	size_t count;
};

OptionInstanceCounter::OptionInstanceCounter () : count (0)
{
	TRF;
}

OptionInstanceCounter::~OptionInstanceCounter ()
{
	TRF;

	if (count != 0)
	{
		TRDEBS ("ERROR: Option instance counter is not 0, is " <<
			count);
	}
}

void OptionInstanceCounter::operator -- ()
{
	ASSERT (count > 0);
	--count;
}

OptionInstanceCounter optioncount;

} // namespace

Option::Option (std::string ntext, Menu * nmenu) :
	str (parsestr (ntext) ),
	typeopt (OptionIsSub),
	pointer (nmenu)
{
	TRFDEB (str);

	++optioncount;
}

Option::Option (std::string ntext, Action * naction) :
	str (parsestr (ntext) ),
	typeopt (OptionIsAction),
	pointer (naction)
{
	TRFDEB (str);

	++optioncount;
}

Option::~Option ()
{
	TRF;

	switch (typeopt)
	{
	case OptionIsSub:
		delete pointer.menu;
		break;
	case OptionIsAction:
		delete pointer.action;
		break;
	}

	--optioncount;
}

const std::string & Option::name () const
{
	return str;
}

void Option::setname (const std::string & newname)
{
	TRFDEBS ("From: " << str << " To: " << newname);

	str= parsestr (newname);
}

Option::TypeOption Option::type () const
{
	return typeopt;
}

Option::pos_t Option::pos () const
{
	return position;
}

void Option::pos (pos_t npos)
{
	position= npos;
}

char Option::charabrev () const
{
	return abrev;
}

Menu * Option::submenu () const
{
	ASSERT (typeopt == OptionIsSub);
	return pointer.menu;
}

Action * Option::getAction () const
{
	ASSERT (typeopt == OptionIsAction);
	return pointer.action;
}

std::string Option::parsestr (const std::string & str)
{
	using std::string;
	abrev= '\0';
	string::size_type pos= str.find ('&');
	if (pos != string::npos)
	{
		if (pos == str.size () -1)
			throw std::runtime_error ("Bad menu");
		abrev= str [pos + 1];
		string s= str;
		s.erase (pos, 1);
		return s;
	}
	return str;
}

//*********************************************************
//		Menu::MenuOption
//*********************************************************

Menu::MenuOption::MenuOption (Option * poption) :
	poption (poption)
{ }

Menu::MenuOption::~MenuOption ()
{
	//delete poption;
}

void Menu::MenuOption::erase ()
{
	delete poption;
}

std::string Menu::MenuOption::name () const
{
	return poption->name ();
}

void Menu::MenuOption::setname (const std::string & newname)
{
	poption->setname (newname);
}

Option::pos_t Menu::MenuOption::pos () const
{
	return poption->pos ();
}

void Menu::MenuOption::pos (Option::pos_t npos)
{
	poption->pos (npos);
}

char Menu::MenuOption::charabrev () const
{
	return poption->charabrev ();
}

Option::TypeOption Menu::MenuOption::type () const
{ 
	return poption->type ();
}

Menu * Menu::MenuOption::submenu () const
{
	return poption->submenu ();
}

Action * Menu::MenuOption::getAction () const
{
	return poption->getAction ();
}

//*********************************************************
//			Menu
//*********************************************************

namespace {

const char separator= '|';

class MenuInstanceCounter {
public:
	MenuInstanceCounter ();
	~MenuInstanceCounter ();
	void operator ++ () { ++count; }
	void operator -- ();
private:
	size_t count;
};

MenuInstanceCounter::MenuInstanceCounter () : count (0)
{
	TRF;
}

MenuInstanceCounter::~MenuInstanceCounter ()
{
	TRF;

	if (count != 0)
	{
		TRDEBS ("ERROR: Menu instance counter is not 0, is " << count);
	}
}

void MenuInstanceCounter::operator -- ()
{
	ASSERT (count > 0);
	--count;
}

MenuInstanceCounter menucount;

} // namespace

Menu::Menu () :
	pot (0),
	width (0),
	inipos (0),
	numopts (0),
	active (NO_SEL)
{
	TRFDEBS ("width= " << width);

	++menucount;
}

Menu::~Menu ()
{
	TRF;

	//for (size_t i= 0, l= options.size (); i < l; ++i)
	//	delete options [i].poption;
	for (size_t i= 0, l= options.size (); i < l; ++i)
		options [i].erase ();

	--menucount;
}

void Menu::addOption (Option * poption)
{
	TRF;

	ASSERT (poption != NULL);

	options.push_back (MenuOption (poption) );
}

void Menu::insertOption (Option * poption, size_t position)
{
	TRF;

	ASSERT (poption != NULL);

	if (position > options.size () )
		addOption (poption);
	else
		options.insert (options.begin () + position,
			MenuOption (poption) );
}

bool Menu::rename (const std::string & from, const std::string & to)
{
	TRFDEBS ("From: " << from << " To: " << to);

	string::size_type s= from.find ('/');
	string optionname= (s != string::npos) ?
		from.substr (0, s) : from;
	string suboptionname= (s != string::npos) ?
		from.substr (s +1) : string ();

	const size_t optsize= options.size ();
	size_t optnum;
	for (optnum= 0; optnum < optsize; ++optnum)
		if (options [optnum].name () == optionname)
			break;
	if (optnum >= optsize)
		return false;

	MenuOption & mopt= options [optnum];
	if (suboptionname.empty () )
	{
		mopt.setname (to);
		return true;
	}
	else
	{
		if (mopt.type () != Option::OptionIsSub)
			return false;
		return mopt.submenu ()->rename (suboptionname, to);
	}
}

Menu * Menu::getSubMenu (const std::string & name)
{
	for (size_t i= 0, l= options.size (); i < l; ++i)
		if (options [i].type () == Option::OptionIsSub &&
			options [i].name () == name)
		{
			return options [i].submenu ();
		}
	return NULL;
}

void Menu::calcpositions ()
{
	ASSERT (pot);

	width= pot->columns ();
	clear= std::string (width, ' ');

	numopts= options.size ();
	size_t position= 0;
	for (size_t i= 0; i < numopts; ++i)
	{
		options [i].pos (position);
		position+= options [i].name ().size () + 1;
	}
}

void Menu::auxputopt (size_t nopt)
{
	const size_t pos= options [nopt].pos () - inipos + 1;
	std::string text= options [nopt].name ();
	const size_t maxl= width - pos;
	if (maxl < text.size () )
		text.erase (maxl);
	* pot << text;
}

void Menu::putopt (size_t nopt)
{
	ASSERT (pot);

	size_t pos= options [nopt].pos ();
	//const std::string & text= options [nopt].poption->text ();
	#if 0
	* pot << at (0, pos - inipos) <<
		//' ' << text << ' ' << std::flush;
		separator << text << separator << std::flush;
	#else

	* pot << at (0, pos - inipos + 1);
	auxputopt (nopt);
	* pot << std::flush;

	#endif
}

void Menu::putall ()
{
	ASSERT (pot);

	* pot << at (0, 0);

	const size_t endpos= inipos + width;

	for (size_t i= 0; i < numopts; ++i)
	{
		//const Option & opt= * options [i].poption;
		const MenuOption & opt= options [i];

		#if 0
		
		if (opt.pos () >= inipos &&
			//opt.pos () + opt.text ().size () <= inipos + width)
			opt.pos () < inipos + width);
		{
			if (i == active)
				* pot << inverse (false);
			putopt (i);
			if (i == active)
				* pot << inverse (true);
		}

		#else

		const Option::pos_t opbeg= opt.pos ();
		if (opbeg < inipos)
			continue;
		if (opbeg >= endpos)
			break;
		//const Option::pos_t opend= opbeg + opt.text ().size () + 1;

		const char sep= (i > 0) ? separator : ' ';
		* pot << /* at (0, opbeg - inipos) << */ sep;

		if (i == active)
			* pot << inverse (false);

		//putopt (i);
		auxputopt (i);

		if (i == active)
			* pot << inverse (true);
		//if (opend < endpos)
		//	* pot << /*at (0, opend) << */ separator;

		#endif
	}

	* pot << std::flush;
}

void Menu::setactive (size_t newactive)
{
	TRF;

	ASSERT (pot);

	if (newactive == active)
		return;
	if (active != NO_SEL)
	{
		putopt (active);
	}
	active= newactive;
	if (newactive != NO_SEL)
	{
		size_t pos= options [active].pos ();
		const std::string & text= options [active].name ();
		TRDEBS ("endpos= " << pos + text.size () );
		if (pos < inipos)
		{
			inipos= pos;
			//putall ();
			show ();
		}
		else if (pos + text.size () >= inipos + width - 2)
		{
			size_t aux= pos + text.size () - width + 2;
			size_t n= 0;
			for ( ; options [n].pos () < aux; ++n)
				continue;
			inipos= options [n].pos ();
			//putall ();
			show ();
		}
		else
		{
			* pot << inverse (false);
			putopt (active);
			* pot << inverse (true);
		}
	}
}

void Menu::show ()
{
	TRF;

	ASSERT (pot);

	* pot << at (0, 0) << inverse (true) << clear;
	//for (size_t i= 0; i < options.size (); ++i)
	//	putopt (i);
	putall ();
}

void Menu::show (otermstream & ots)
{
	TRF;

	otermstream * prevpot= pot;
	pot= & ots;
	calcpositions ();
	show ();
	pot= prevpot;
}

namespace {

class CursorHide
{
public:
	CursorHide (otermstream & ot);
	~CursorHide ();
private:
	otermstream & ot;
};

CursorHide::CursorHide (otermstream & ot) :
	ot (ot)
{
	ot.cursor (false);
}

CursorHide::~CursorHide ()
{
	ot.cursor (true);
}

} // namespace

Action * Menu::selectoption (otermstream & ots)
{
	TRF;

	CursorHide ch (ots);
	return do_selectoption (ots);
}

Action * Menu::do_selectoption (otermstream & ots)
{
	TRF;

	using namespace key;

	if (options.empty () )
		return NULL;

	pot= & ots;
	show (ots);
	//ASSERT (selected != 0);
	//size_t active= 0;
	setactive (0);
	bool fSelecting= true;
	bool fSelected= false;
	Action * result= NULL;
	do
	{
		int c;
		switch ( (c= pot->getkey () ) )
		{
		case '\r': case '\n':
			if ( (result= activate (active, ots) ) != 0)
			{
				fSelected= true;
				fSelecting= false;
			}
			//redraw (newactive);
			show ();
			break;
		case Esc:
		case F8:
			fSelecting= false;
			break;
		case Left:
			setactive ( (active + numopts - 1) % numopts);
			break;
		case Right:
			setactive ( (active + 1) % numopts);
			break;
		default:
			if (c > 0xFF)
				break;
			c= toupper (c);
			if (options [active].charabrev () != c)
			{
				size_t i;
				for (i= (active + 1) % numopts;
					i!= active; i= (i + 1) % numopts)
				{
					if (options [i].charabrev () == c)
						break;
				}
				if (i == active)
					break;
				setactive (i);
			}
			if ( (result= activate (active, ots) ) != 0)
			{
				fSelected= true;
				fSelecting= false;
			}
			//redraw (newactive);
			show ();
			//prev= NO_SEL;
		} // switch
	} while (fSelecting);
	* pot << inverse (true);
	//putopt (newactive);
	setactive (NO_SEL);
	inipos= 0;
	show ();
	//return fSelected;
	if (fSelected)
	{
		ASSERT (result != 0);
		return result;
	}
	else
		return 0;
}

Action * Menu::activate (size_t n, otermstream & ots)
{
	if (options [n].type () == Option::OptionIsSub)
	{
		Menu * submenu= options [n].submenu ();
		return submenu->do_selectoption (ots);
	}
	else
	{
		return options [n].getAction ();
	}
}

// End of menu.cpp

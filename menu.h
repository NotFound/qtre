#ifndef INCLUDE_MENU_H_
#define INCLUDE_MENU_H_

// menu.h
// Revision 29-jan-2006

#include "termstream.h"
#include "action.h"

#include <string>
#include <vector>
#include <map>

class Menu;

class Option {
public:
	typedef size_t pos_t;
	typedef enum { OptionIsSub, OptionIsAction } TypeOption;

	Option (std::string ntext, Menu * nmenu);
	Option (std::string ntext, Action * naction);
	~Option ();

	const std::string & name () const;
	void setname (const std::string & newname);
	pos_t pos () const;
	void pos (pos_t npos);
	char charabrev () const;

	TypeOption type () const;
	Menu * submenu () const;
	Action * getAction () const;
private:
	Option (const Option & opt); // Forbidden
	Option & operator= (const Option & opt); // Forbidden

	std::string str;
	pos_t position;
	char abrev;

	TypeOption typeopt;
	union UPointer{
		Menu * menu;
		Action * action;
		UPointer (Menu * m) { menu= m; }
		UPointer (Action * a) { action= a; }
	} pointer;
	std::string parsestr (const std::string & str);
};

class Menu {
public:
	Menu ();
	~Menu ();

	void addOption (Option * pop);
	void insertOption (Option * pop, size_t position);

	bool rename (const std::string & from, const std::string & to);

	Menu * getSubMenu (const std::string & name);
	void show ();
	void show (otermstream & ots);
	Action * selectoption (otermstream & ots);
private:
	Menu (const Menu &); // Forbidden
	void operator= (const Menu &); // Forbidden
	otermstream * pot;
	class MenuOption {
	public:
		MenuOption (Option * poption);
		~MenuOption ();
		void erase ();
		std::string name () const;
		void setname (const std::string & newname);
		Option::pos_t pos () const;
		void pos (Option::pos_t npos);
		char charabrev () const;
		Option::TypeOption type () const;
		Menu * submenu () const;
		Action * getAction () const;
	private:
		Option * poption;
	};
	std::vector <MenuOption> options;
	size_t width;
	std::string clear;
	size_t inipos;
	size_t numopts;
	static const size_t NO_SEL= size_t (-1);
	size_t active;

	Action * do_selectoption (otermstream & ots);
	void calcpositions ();
	void auxputopt (size_t nopt);
	void putopt (size_t nopt);
	void putall ();
	void setactive (size_t newactive);
	Action * activate (size_t n, otermstream & ots);
};

#endif

// Fin de menu.h

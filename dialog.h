#ifndef INCLUDE_DIALOG_H_
#define INCLUDE_DIALOG_H_

// dialog.h
// Revision 2-oct-2004

#include "termstream.h"
#include "history.h"

#include <string>
#include <set>

void centertext (otermstream & ot, int lin, int width, std::string str);

bool dialogGetString (otermstream &ots,
	int & keyend, const std::set <int> & keyender,
	const std::string & title, const std::string & prompt,
	std::string & str, History & history,
	size_t width, std::string::size_type maxlength= std::string::npos);

bool dialogGetString (otermstream &ots,
	const std::string & title, const std::string & prompt,
	std::string & str, History & history,
	size_t width, std::string::size_type maxlength= std::string::npos);

void dialogShowText (otermstream & ots, const char * title, const char * str,
	int height, int width);


enum dialogResult { dialogCancel, dialogYes, dialogNo };

dialogResult dialogQuestion (otermstream & ots, const std::string & str);

class DialogOptions {
public:
	DialogOptions (otermstream & ots, const std::string & title);
	~DialogOptions ();
	void add (const std::string & description, bool & option);
	bool do_it ();
private:
	class Internal;
	Internal * pin;
};

#endif

// Fin de dialog.h

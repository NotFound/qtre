#ifndef INCLUDE_SCREEN_H_
#define INCLUDE_SCREEN_H_

// screen.h
// Revision 25-jan-2006

//#include "menu.h"
//#include "qperl.h"

#include <string>

#include <stdio.h>

class Screen
{
protected:
	Screen ();
public:
	virtual ~Screen ();

	#ifndef NDEBUG
	static bool screenpointervalid (Screen * p);
	#endif

	virtual void windownew ()= 0;
	virtual size_t textlines () const= 0;
	virtual void gotoline (size_t l)= 0;
	virtual void loadtext (FILE * fin)= 0;
	virtual void loadtext (const std::string & strFileName)= 0;
	virtual void changewindow (size_t newwin)= 0;
	virtual void interact ()= 0;
};

Screen *  createScreen (bool useutf8, char * * env);

#endif

// End of screen.h

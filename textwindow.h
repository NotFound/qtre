#ifndef INCLUDE_TEXTWINDOW_H_
#define INCLUDE_TEXTWINDOW_H_

// textwindow.h
// Revision 25-jan-2006

#include "text.h"

class TextWindow
{
private:
	size_t * pcount;
	TextFile * ptext;
public:
	TextWindow (bool useutf8);
	TextWindow (const TextWindow & tw);
	~TextWindow ();
	void operator = (const TextWindow & tw);
	TextFile * text ();
	size_t lin;
	Col col;
	size_t inilin;
	Col inicol;
	Selection sel;
};

#endif

// End of textwindow.h

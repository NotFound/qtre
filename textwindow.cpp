// textwindow.cpp
// Revision 27-jan-2006

#include "textwindow.h"

#include "trace.h"
#undef ASSERT
#include "qtreassert.h"

TextWindow::TextWindow (bool useutf8) :
	pcount (new size_t),
	ptext (new TextFile (useutf8) ),
	lin (0),
	col (0),
	inilin (0),
	inicol (0)
{
	TRF;

	* pcount= 1;
}

TextWindow::TextWindow (const TextWindow & tw) :
	pcount (tw.pcount),
	ptext (tw.ptext),
	lin (tw.lin),
	col (tw.col),
	inilin (tw.inilin),
	inicol (tw.inicol),
	sel (tw.sel)
{
	TRF;

	++ * pcount;
}

TextWindow::~TextWindow ()
{
	TRF;

	if (-- * pcount == 0)
		delete ptext;
}

void TextWindow::operator = (const TextWindow & tw)
{
	TRF;

	if (this != & tw)
	{
		if (-- * pcount == 0)
			delete ptext;
		pcount= tw.pcount;
		++ * pcount;
		ptext= tw.ptext;
		lin= tw.lin;
		col= tw.col;
		inilin= tw.inilin;
		inicol= tw.inicol;
		sel= tw.sel;
	}
}

TextFile * TextWindow::text ()
{
	return ptext;
}

// End of textwindow.cpp

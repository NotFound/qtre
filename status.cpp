// status.cpp
// Revision 27-jan-2006

#include "status.h"
#include "trace.h"

#include <iomanip>

using std::string;
using std::flush;

namespace {

const size_t width= 7;

const size_t postext= width * 2 + 1;

} // namespace

StatusLine::StatusLine (int ancho, int y) :
	ot (1, ancho, y, 0),
	invalid (true)
{
	TRF;

	ot.inverse (true);
	max= ancho - postext;
	ot << at (0, 0) << string (ancho, ' ') << flush;
}

StatusLine::~StatusLine ()
{
	TRF;
}

void StatusLine::invalidate ()
{
	invalid= true;
}

void StatusLine::refresh ()
{
	plincol ();
	puttext ();
}

void StatusLine::update ()
{
	if (invalid)
	{
		refresh ();
		invalid= false;
	}
}

void StatusLine::settext (std::string str)
{
	TRFDEB (str);

	size_t l= str.size ();
	if (l < max)
		str+= string (max - l, ' ');
	else if (l > max)
		str= str.substr (0, max);
	this->str= str;
	if (! invalid)
		puttext ();
}

int StatusLine::getkey (const std::string & str)
{
	clear ();
	ot << at (0, postext) << ' ' << str;
	int key= ot.getkey ();
	update ();
	return key;
}

void StatusLine::clear ()
{
	TRF;

	str.assign (max, ' ');
	puttext ();
}

void StatusLine::lincol (int y, int x)
{
	this->x= x; this->y= y;
	if (! invalid)
	{
		plincol ();
		ot << flush;
	}
}

void StatusLine::plincol ()
{
	ot << at (0, 0) << std::setw (width) << y + 1 << ' ' <<
		std::setw (width) << x + 1;
}

void StatusLine::puttext ()
{
	TRF;

	ot << at (0, postext) << ' ' << str << flush;
}

char StatusLine::question (const std::string & q, const std::string & values)
{
	TRFDEB (values);

	settext (q);
	ot << at (0, postext + 2 + q.size () );
	char c;
	do
	{
		int i= ot.getkey ();
		c= (i > 255 | i < 0) ? 0 : static_cast <char> (i);
	} while (values.find (c) == std::string::npos);
	ot << c << flush;
	return c;
}

// Fin de status.cpp

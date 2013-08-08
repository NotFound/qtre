// select.cpp
// Revision 29-jan-2006

#include "select.h"
#include "qtreassert.h"

namespace {

class Select {
public:
	Select (otermstream & ot, const std::string & title,
		const std::vector <std::string> & vstr);
	~Select ();
	size_t doit ();
private:
	std::string title;
	const std::vector <std::string> & vstr;
	const size_t n;
	otermstream * pot;
	size_t elem;
	size_t width;
	size_t lines;
	size_t first;
	size_t actual;
	size_t maxline;
	size_t maxfirst;

	void putall ();
	void put (size_t i);
};

Select::Select (otermstream & ot, const std::string & title,
		const std::vector <std::string> & vstr) :
	vstr (vstr),
	n (vstr.size () ),
	pot (0),
	first (0),
	actual (0)
{
	ASSERT (n > 0);
	lines= vstr.size () ;
	if (lines > size_t (ot.lines () ) - 2)
		lines= ot.lines () - 2;
	width= title.size ();
	for (size_t i= 0; i < n; ++i)
		if (vstr [i].size () > width)
			width= vstr [i].size ();
	if (width > size_t (ot.columns () ) - 2)
		width= ot.columns () - 2;
	elem= vstr.size ();
	if (elem > lines)
		elem= lines;
	maxline= vstr.size () - 1;
	maxfirst= maxline - lines + 1;

	this->title= title.substr (0, width);
	pot= new otermstream (ot, lines + 2, width + 2,
		(ot.lines () - lines - 2) / 2,
		(ot.columns () - width - 2) / 2);
	pot->cursor (false);
}

Select::~Select ()
{
	pot->cursor (true);
	delete pot;
}

namespace {

char firstchar (const std::string & str)
{
	if (str.empty () )
		return '\0';
	return str [0];
}

} // namespace

size_t Select::doit ()
{
	using namespace key;

	pot->clrscr ();
	pot->border ();
	* pot << at (0, 1 + (width - title.size () ) / 2) << title;

	putall ();

	for (;;)
	{
		size_t ant= actual;
		size_t antfirst= first;
		int k= pot->getkey ();
		switch (k)
		{
		case '\r': case '\n':
			return actual;
		case Esc: case F8:
			return size_t (-1);
		case Down:
			if (actual < vstr.size () - 1)
				++actual;
			break;
		case Up:
			if (actual > 0)
				--actual;
			break;
		case PageUp:
			if (first > lines - 1)
				first-= lines - 1;
			else
				first= 0;
			if (actual > lines - 1)
				actual-= lines - 1;
			else
				actual= 0;
			break;
		case PageDown:
			first+= lines - 1;
			if (first > maxfirst)
				first= maxfirst;
			actual+= lines - 1;
			if (actual > maxline)
				actual= maxline;
			break;
		case Home:
			actual= 0;
			first= 0;
			break;
		case End:
			actual= maxline;
			first= maxfirst;
			break;
		default:
			if (k <= 0 || k > 255)
				break;
			for (size_t i= actual + 1, l= vstr.size (); i < l; ++i)
				if (firstchar (vstr [i] ) == k)
				{
					actual= i;
					break;
				}
		}
		if (actual >= first + lines)
			first= actual - lines + 1;
		if (actual < first)
			first= actual;
		if (first != antfirst)
			putall ();
		else
			if (ant != actual)
			{
				put (ant);
				put (actual);
			}
	}
}

void Select::putall ()
{
	for (size_t i= first, end= first + lines; i < end; ++i)
	{
		if (i < vstr.size () )
		{
			put (i);
		}
		else
			* pot << at (i - first + 1, 1) <<
				std::string (' ', width);
	}
	pot->flush ();
}

void Select::put (size_t i)
{
	if (i == actual)
		* pot << inverse (true);
	std::string str (vstr [i].substr (0, width) );
	if (str.size () < width)
		str+= std::string (width - str.size (), ' ');
	* pot << at (i - first + 1, 1) << str;
	if (i == actual)
		* pot << inverse (false);
}

} // namespace

size_t selectlist (otermstream & ot, const std::string & title,
	const std::vector <std::string> & vstr)
{
	Select s (ot, title, vstr);
	return s.doit ();
}

// End of select.cpp

// history.cpp
// Revision 21-nov-2003

#include "history.h"
//#include <deque>
#include <vector>

class History::Internal {
public:
	size_t size () const
	{
		return h.size ();
	}
	std::string operator [] (size_t n) const
	{
		return h [n];
	}
	void push_back (const std::string & str)
	{
		if (size () >= maxsize)
		{
			h.erase (h.begin (),
				h.begin () + size () - maxsize + 1);
		}
		if (! str.empty () && (h.empty () || str != h.back () ) )
			h.push_back (str);
	}
private:
	static const size_t maxsize= 50;
	//std::deque <std::string> h;
	std::vector <std::string> h;
};

const size_t History::Internal::maxsize;

History::History () :
	pin (new Internal)
{
}

History::~History ()
{
	delete pin;
}

size_t History::size () const
{
	return pin->size ();
}

std::string History::operator [] (size_t n) const
{
	return (* pin) [n];
}

void History::push_back (const std::string & str)
{
	pin->push_back (str);
}

// End of history.cpp

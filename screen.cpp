// screen.cpp
// Revision 27-jan-2006

#include "screen.h"

#include "trace.h"

#include <set>
#include <stdexcept>

using std::set;
using std::logic_error;

//***********************************************
//		Screen
//***********************************************

#ifndef NDEBUG

namespace {

set <Screen *> validscreen;

} // namespace

#endif

Screen::Screen ()
{
	TRF;

	#ifndef NDEBUG

	if (validscreen.find (this) != validscreen.end () )
		throw logic_error ("Error registering Screen instance");
	validscreen.insert (this);

	#endif
}

Screen::~Screen ()
{
	TRF;

	#ifndef NDEBUG

	if (validscreen.find (this) == validscreen.end () )
		throw logic_error ("Error unregistering Screen instance");
	validscreen.erase (this);

	#endif
}

#ifndef NDEBUG

bool Screen::screenpointervalid (Screen * p)
{
	return validscreen.find (p) != validscreen.end ();
}

#endif


// End of screen.cpp

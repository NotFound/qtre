// action.cpp
// Revision 27-jan-2006

#include "action.h"

#include "util.h"

#include "trace.h"
#undef ASSERT
#include "qtreassert.h"

#include <algorithm>

using util::to_string;

namespace {

#ifndef NDEBUG

class ActionInstanceCounter {
public:
	ActionInstanceCounter ();
	~ActionInstanceCounter ();
	void operator ++ () { ++count; }
	void operator -- ();
private:
	size_t count;
};

ActionInstanceCounter::ActionInstanceCounter () : count (0)
{
	TRF;
}

ActionInstanceCounter::~ActionInstanceCounter ()
{
	TRF;

	if (count != 0)
		TRDEB ("ERROR: Action instance counter "
			"is not 0, is " + to_string (count) );
}

void ActionInstanceCounter::operator -- ()
{
	ASSERT (count > 0);
	--count;
}

ActionInstanceCounter actioncount;

#endif

} // namespace

Action::Action ()
{
	#ifndef NDEBUG
	++actioncount;
	#endif
}

Action::~Action ()
{
	#ifndef NDEBUG
	--actioncount;
	#endif
}

namespace {

struct Destroy {
	void operator () (MapKeyAction::mapka::value_type & v)
	{
		delete v.second;
	}
};

} // namespace

void MapKeyAction::addKeyAction (int key, Action * action)
{
	mapka::iterator it= m.find (key);
	if (it != m.end () )
		Destroy () (* it);
	m [key]= action;
}

Action * MapKeyAction::getKeyAction (int key)
{
	mapka::iterator it= m.find (key);
	if (it != m.end () )
		return it->second;
	else
		return 0;
}

MapKeyAction::~MapKeyAction ()
{
	std::for_each (m.begin (), m.end (), Destroy () );
}

// End of action.cpp

#ifndef INCLUDE_HISTORY_H_
#define INCLUDE_HISTORY_H_

// history.h
// Revision 18-sep-2004

#include <string>

class History {
public:
	History ();
	~History ();
	size_t size () const;
	std::string operator [] (const size_t n) const;
	void push_back (const std::string & str);
private:
	class Internal;
	Internal * pin;
};

#endif

// End of history.h

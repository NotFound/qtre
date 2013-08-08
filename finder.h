#ifndef INCLUDE_FINDER_H_
#define INCLUDE_FINDER_H_

// finder.h
// Revision 21-sep-2004

#include <string>

struct SearchOptions {
	bool backwards;
	bool nocase;
	bool wholeword;
	bool regex;
	bool function;
	SearchOptions ();
};

class Finder {
public:
	typedef std::string string;
	typedef std::string::size_type size_type;
	static const size_type npos= string::npos;

	virtual bool backwards () const { return false; }
	virtual size_type find (const string & str)= 0;
	virtual size_type find (const string & str, size_type from) = 0;
	virtual size_type length () const = 0; // Length of last match.
	virtual string replace (const string & str, const string & line)= 0;
	virtual string replace (const string & function, const string & line,
		size_type pos, void * data) = 0;

	virtual ~Finder () { }
};

Finder * createFinder
	(const SearchOptions & options, const std::string & strFind);


#endif

// End of finder.h

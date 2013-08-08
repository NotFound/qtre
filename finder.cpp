// finder.cpp
// Revision 27-jan-2006

#include "finder.h"

#include "util.h"
#include "qperl.h"

#include "trace.h"
#undef ASSERT
#include "qtreassert.h"


#include <vector>
#include <map>
#include <algorithm>
#include <stdexcept>

#include <ctype.h>
#include <regex.h>

using std::string;
using std::map;
using std::logic_error;

//***********************************************
//	Auxiliary functions.
//***********************************************

namespace {

struct equal_nocase {

	// Used by the nocase finders.

	bool operator () (const char c1, const char c2) const
	{
		return toupper (c1) == toupper (c2);
	}
};

} // namespace

//***********************************************
//	Finder derived classes declarations
//***********************************************

class StringFinder : public Finder {

	// Is as base of all no regex Finder classes.

public:
	StringFinder (const string & searched) :
		searched (searched),
		l (searched.size () )
	{ }
	size_type length () const { return l; }
	virtual bool adjustfrom (const string & str, size_type & from);
	virtual string replace (const string & str, const string & line);
	virtual string replace (const string & function, const string & line,
		size_type pos, void * data);
protected:
	const string searched;
private:
	const size_type l;
};

class NormalFinder : public StringFinder {
public:
	NormalFinder (const string & searched) :
		StringFinder (searched)
	{ }
	size_type find (const string & str);
	size_type find (const string & str, size_type from);
};

class BackFinder : public StringFinder {
public:
	BackFinder (const string & searched) :
		StringFinder (searched)
	{ }
	bool backwards () const { return true; }
	size_type find (const string & str);
	size_type find (const string & str, size_type from);
};
class NocaseFinder : public StringFinder {
public:
	NocaseFinder (const string & searched) :
		StringFinder (searched)
	{ }
	size_type find (const string & str);
	size_type find (const string & str, size_type from);
};

class BackNocaseFinder : public StringFinder {
public:
	BackNocaseFinder (const string & searched) :
		StringFinder (searched)
	{ }
	bool backwards () const { return true; }
	size_type find (const string & str);
	size_type find (const string & str, size_type from);
private:
	bool adjustfrom (const string & str, size_type & from);
};

class WordBaseFinder : public StringFinder {
public:
	WordBaseFinder (const string & searched);

	size_type find (const string & str);
	size_type find (const string & str, size_type from) ;
protected:
	bool checkword (const string & str, size_type r);
	const size_type ssize;
	const bool alnumfirst;
	const bool alnumlast;

	virtual size_type do_find (const string & str, size_type from)= 0;
};

class WordFinder : public WordBaseFinder {
public:
	WordFinder (const string & searched) :
		WordBaseFinder (searched)
	{ }
private:
	virtual size_type do_find (const string & str, size_type from);
};

class WordBackFinder : public WordBaseFinder {
public:
	WordBackFinder (const string & searched) :
		WordBaseFinder (searched)
	{ }
	bool backwards () const { return true; }
private:
	virtual size_type do_find (const string & str, size_type from);
};

class WordNocaseFinder : public WordBaseFinder {
public:
	WordNocaseFinder (const string & searched) :
		WordBaseFinder (searched)
	{ }
private:
	virtual size_type do_find (const string & str, size_type from);
};

class WordBackNocaseFinder : public WordBaseFinder {
public:
	WordBackNocaseFinder (const string & searched) :
		WordBaseFinder (searched)
	{ }
	bool backwards () const { return true; }
private:
	bool adjustfrom (const string & str, size_type & from);
	virtual size_type do_find (const string & str, size_type from);
};

//	Regex Finder classes.

class RegexBaseFinder : public Finder {
public:
	RegexBaseFinder (const string & searched, bool nocase);
	~RegexBaseFinder ();

	//virtual size_type find (const string & str)= 0;
	//virtual size_type find (const string & str, size_type from)= 0;
	size_type length () const;
#if 0
protected:
	class Internal;
	Internal * pin;
private:
#endif
	string replace (const string & str, const string & line);
	string replace (const string & function, const string & line,
		size_type pos, void * data);
//private:
protected:
	regex_t reg;

	typedef std::vector <regmatch_t> vregmatch;
	vregmatch pmatch;

	#if 0
	// This does not need to be a member, but that way
	// avoids creating and destroying it in each search.
	vregmatch auxmatch;
	#endif

	size_t pmlen;
	size_type desp;

	//size_type szlength;
	//void setlength ();

	//size_type do_find (const string & str, size_type from);
	void checkresult (int r);
};

class RegexFinder : public RegexBaseFinder {
public:
	RegexFinder (const string & searched, bool nocase);
	size_type find (const string & str);
	size_type find (const string & str, size_type from);
private:
	size_type do_find (const string & str, size_type from);
};

class RegexBackFinder : public RegexBaseFinder {
public:
	RegexBackFinder (const string & searched, bool nocase);
	bool backwards () const { return true; }
	size_type find (const string & str);
	size_type find (const string & str, size_type from);
private:
	// This does not need to be a member, but that way
	// avoids creating and destroying it in each search.

	vregmatch auxmatch;
};

//***********************************************
//		SearchOptions
//***********************************************

SearchOptions::SearchOptions () :
	backwards (false),
	nocase (false),
	wholeword (false),
	regex (false),
	function (false)
{ }

//***********************************************
//		Finder creator
//***********************************************

namespace {

// Create a map between SearchOptions and functions that create
// a Finder of the appropiate type.

typedef Finder::string string;

typedef Finder * (* createfinderfunc) (const string & str);

Finder * createnormal (const string & str)
{ return new NormalFinder (str); }

Finder * createback (const string & str)
{ return new BackFinder (str); }

Finder * createnocase (const string & str)
{ return new NocaseFinder (str); }

Finder * createbacknocase (const string & str)
{ return new BackNocaseFinder (str); }


Finder * createword (const string & str)
{ return new WordFinder (str); }

Finder * createwordback (const string & str)
{ return new WordBackFinder (str); }

Finder * createwordnocase (const string & str)
{ return new WordNocaseFinder (str); }

Finder * createwordbacknocase (const string & str)
{ return new WordBackNocaseFinder (str); }


Finder * createregex (const string & str)
{ return new RegexFinder (str, false); }

Finder * createregexback (const string & str)
{ return new RegexBackFinder (str, false); }

Finder * createregexnocase (const string & str)
{ return new RegexFinder (str, true); }

Finder * createregexbacknocase (const string & str)
{ return new RegexBackFinder (str, true); }


// The SearchOptions member 'function' is not used here
// because it does not affect the type of search performed.

// Less operation used in the map.

struct lessOptions {
	bool operator ()
		(const SearchOptions & opt1, const SearchOptions & opt2) const
	{
		if (opt1.nocase < opt2.nocase) return true;
		if (opt1.nocase > opt2.nocase) return false;
		if (opt1.backwards < opt2.backwards) return true;
		if (opt1.backwards > opt2.backwards) return false;
		if (opt1.wholeword < opt2.wholeword) return true;
		if (opt1.wholeword > opt2.wholeword) return false;
		if (opt1.regex < opt2.regex) return true;
		return false;
	}
};


// The map.

typedef map <SearchOptions, createfinderfunc, lessOptions> mapcreatefinder_t;
mapcreatefinder_t mapcreatefinder;


// This simple singleton takes care of the map initialization.

class Initmapcreate {
	// All private, so nothing can build another instance.
	Initmapcreate ();
	Initmapcreate (const Initmapcreate &); // Unimplemented.
	~Initmapcreate ();
	static Initmapcreate instance;

	// Avoid gcc warning.

	class NoWarning { };
	friend class NoWarning;
};

Initmapcreate Initmapcreate::instance;

// Function to simplify the building of the map.

SearchOptions mkopts (bool backwards, bool nocase, bool wholeword, bool regex)
{
	SearchOptions opts;
	opts.backwards= backwards;
	opts.nocase= nocase;
	opts.wholeword= wholeword;
	opts.regex= regex;
	return opts;
}

Initmapcreate::Initmapcreate ()
{
	TRF;

	mapcreatefinder_t & m= mapcreatefinder;
	m [mkopts (false, false, false, false)]= createnormal;
	m [mkopts (true,  false, false, false)]= createback;
	m [mkopts (false, true,  false, false)]= createnocase;
	m [mkopts (true,  true,  false, false)]= createbacknocase;

	m [mkopts (false, false, true,  false)]= createword;
	m [mkopts (true,  false, true,  false)]= createwordback;
	m [mkopts (false, true,  true,  false)]= createwordnocase;
	m [mkopts (true,  true,  true , false)]= createwordbacknocase;

	m [mkopts (false, false, false, true) ]= createregex;
	m [mkopts (true,  false, false, true) ]= createregexback;
	m [mkopts (false, true,  false, true) ]= createregexnocase;
	m [mkopts (true,  true,  false, true) ]= createregexbacknocase;

	// regex & word is not allowed.
}

Initmapcreate::~Initmapcreate ()
{
	TRF;
}

} // namespace


// Extern function to create a finder, using all previous stuff

Finder * createFinder (const SearchOptions & options,
	const std::string & strFind)
{
	TRF;

	mapcreatefinder_t::iterator it= mapcreatefinder.find (options);
	if (it == mapcreatefinder.end () )
	{
		// This must not happen, the options must be checked
		// before calling this.
		throw logic_error ("Invalid option combination");
	}
	Finder * pfinder= (* it->second) (strFind);
	ASSERT (pfinder != 0);
	return pfinder;
}

//***********************************************
//	Finder derived classes implementation.
//***********************************************


//***********************************************
//		StringFinder
//***********************************************

bool StringFinder::adjustfrom (const string & str, size_type & from)
{
	if (backwards () )
	{
		if (from <= searched.size () )
			return false;
		from-= searched.size () + 1;
	}
	else
	{
		++from;
		if (from >= str.size () )
			return false;
	}
	return true;
}

Finder::string StringFinder::replace (const string & str, const string &)
{
	return str;
}

Finder::string StringFinder::replace (const string & function,
	const string & line, size_type pos, void * data)
{
	TRF;

	string arg (line, pos, searched.size () );
	const char * const argv [1]= { arg.c_str () };
	char * result;
	qperl_function_args_r (function.c_str (), data,
		util::dim_array (argv), argv, & result);
	std::string r (result ? result : "");

	TRDEB ("Returned: " + r);

	qperl_free_str (result);
	return r;
}

//***********************************************
//		NormalFinder
//***********************************************

Finder::size_type NormalFinder::find (const string & str)
{
	return str.find (searched);
}

Finder::size_type NormalFinder::find (const string & str, size_type from)
{
	if (! adjustfrom (str, from) )
		return npos;
	return str.find (searched, from);
}

//***********************************************
//		BackFinder
//***********************************************

Finder::size_type BackFinder::find (const string & str)
{
	return str.rfind (searched);
}

Finder::size_type BackFinder::find (const string & str, size_type from)
{
	if (! adjustfrom (str, from) )
		return npos;
	return str.rfind (searched, from);
}

//***********************************************
//		NocaseFinder
//***********************************************

Finder::size_type NocaseFinder::find (const string & str)
{
	string::const_iterator it=
		std::search (str.begin (), str.end (),
			searched.begin (), searched.end (),
			equal_nocase () );
	return (it == str.end () ) ?
		npos :
		std::distance (str.begin (), it);
}

Finder::size_type NocaseFinder::find (const string & str, size_type from)
{
	if (! adjustfrom (str, from) )
		return npos;
	string::const_iterator it=
		std::search (str.begin () + from, str.end (),
			searched.begin (), searched.end (),
			equal_nocase () );
	return (it == str.end () ) ?
		npos :
		std::distance (str.begin (), it);				
}

//***********************************************
//		BackNocaseFinder
//***********************************************

bool BackNocaseFinder::adjustfrom (const string & str, size_type & from)
{
	if (backwards () )
	{
		if (from <= searched.size () )
			return false;
		--from;
	}
	else
	{
		++from;
		if (from >= str.size () )
			return false;
	}
	return true;
}

Finder::size_type BackNocaseFinder::find (const string & str)
{
	string::const_iterator it=
		std::find_end (str.begin (), str.end (),
			searched.begin (), searched.end (),
			equal_nocase () );
	return (it == str.end () ) ?
		npos :
		std::distance (str.begin (), it);
}
Finder::size_type BackNocaseFinder::find (const string & str, size_type from)
{
	if (! adjustfrom (str, from) )
		return npos;
	string::const_iterator it=
		std::find_end (str.begin (), str.begin () + from,
			searched.begin (), searched.end (),
			equal_nocase () );
	return (it == str.begin () + from) ?
		npos :
		std::distance (str.begin (), it);
}

//***********************************************
//		WordBaseFinder
//***********************************************

namespace {

bool iswordchar (char c)
{
	return isalnum (c) || c == '_';
}

} // namespace

WordBaseFinder::WordBaseFinder (const string & searched) :
	StringFinder (searched),
	ssize (searched.size () ),
	alnumfirst (! searched.empty () && iswordchar (searched [0] ) ),
	alnumlast (! searched.empty () &&
		iswordchar (searched [ssize - 1] ) )
{
}

bool WordBaseFinder::checkword (const string & str, size_type r)
{
	size_type end= r + ssize;
	return (! alnumfirst ||
		(r == 0 || ! iswordchar (str [r - 1] ) ) ) &&
		(! alnumlast || 
		(end >= str.size () || ! iswordchar (str [end] )
		) );
}

Finder::size_type WordBaseFinder::find (const string & str)
{
	return do_find (str, backwards () ? str.size () : 0);
}

Finder::size_type WordBaseFinder::find (const string & str, size_type from)
{
	if (! adjustfrom (str, from) )
		return npos;
	return do_find (str, from);
}

//***********************************************
//		WordFinder
//***********************************************

Finder::size_type WordFinder::do_find (const string & str, size_type from)
{
	size_type r;
	for (;;)
	{
		r= str.find (searched, from);
		if (r == npos)
			break;
		if (checkword (str, r) )
			break;
		from= r + 1;
	}
	return r;
}

//***********************************************
//		WordBackFinder
//***********************************************

Finder::size_type WordBackFinder::do_find (const string & str, size_type from)
{
	size_type r;
	for (;;)
	{
		r= str.rfind (searched, from);
		if (r == npos)
			break;
		if (checkword (str, r) )
			break;
		if (r == 0)
		{
			r= npos;
			break;
		}
		from= r - 1;
	}
	return r;
}

//***********************************************
//		WordNocaseFinder
//***********************************************

Finder::size_type WordNocaseFinder::do_find
	(const string & str, size_type from)
{
	size_type r;
	for (;;)
	{
		string::const_iterator it=
			std::search (str.begin () + from, str.end (),
				searched.begin (), searched.end (),
				equal_nocase () );
		r= (it == str.end () ) ?
			npos :
			std::distance (str.begin (), it);
		if (r == npos)
			break;
		if (checkword (str, r) )
			break;
		from= r + 1;
	}
	return r;
}

//***********************************************
//		WordBackNocaseFinder
//***********************************************

bool WordBackNocaseFinder::adjustfrom (const string & str, size_type & from)
{
	if (backwards () )
	{
		if (from <= searched.size () )
			return false;
		--from;
	}
	else
	{
		++from;
		if (from >= str.size () )
			return false;
	}
	return true;
}

Finder::size_type WordBackNocaseFinder::do_find
	(const string & str, size_type from)
{
	size_type r;
	for (;;)
	{
		string::const_iterator it=
			std::find_end (str.begin (),
				str.begin () + from,
				searched.begin (), searched.end (),
				equal_nocase () );
		r= (it == str.begin () + from) ?
			npos :
			std::distance (str.begin (), it);
		if (r == npos)
			break;
		if (checkword (str, r) )
			break;
		if (r == 0)
		{
			r= npos;
			break;
		}
		from= r - 1;
	}
	return r;
}


//***********************************************
//	RegexBaseFinder::Internal
//***********************************************

#if 0
class RegexBaseFinder::Internal {
public:
	Internal (const string & searched, bool nocase);
	~Internal ();
	size_type find (const string & str);
	size_type find (const string & str, size_type from);
	size_type find_back (const string & str);
	size_type find_back (const string & str, size_type from);
	string replace (const string & str, const string & line);
	string replace (const string & function, const string & line,
		size_type pos, void * data);
	size_type length () const;
private:
	regex_t reg;

	typedef std::vector <regmatch_t> vregmatch;
	vregmatch pmatch;

	// This does not need to be a member, but that way
	// avoids creating and destroying it in each search.
	vregmatch auxmatch;

	size_t pmlen;
	size_type desp;

	size_type szlength;
	void setlength ();
	size_type do_find (const string & str, size_type from);
	void checkresult (int r);
};
#endif

//RegexBaseFinder::Internal::Internal (const string & searched, bool nocase) :
RegexBaseFinder::RegexBaseFinder (const string & searched, bool nocase)
	//szlength (0)
{
	int cflags= REG_EXTENDED;
	if (nocase)
		cflags|= REG_ICASE;
	int r= regcomp (& reg, searched.c_str (), cflags);
	checkresult (r);
	pmatch.resize (reg.re_nsub + 1);
	//auxmatch.resize (pmatch.size () );
	pmlen= pmatch.size ();
}

//RegexBaseFinder::Internal::~Internal ()
RegexBaseFinder::~RegexBaseFinder ()
{
	regfree (& reg);
}

#if 0
//void RegexBaseFinder::Internal::setlength ()
void RegexBaseFinder::setlength ()
{
	szlength= pmatch [0].rm_eo - pmatch [0].rm_so;
}
#endif

//Finder::size_type RegexBaseFinder::Internal::find
Finder::size_type RegexFinder::find
	(const string & str)
{
	return do_find (str, 0);
}

//Finder::size_type RegexBaseFinder::Internal::find
Finder::size_type RegexFinder::find
	(const string & str, size_type from)
{
	++from;
	if (from >= str.size () )
		return npos;
	return do_find (str, from);
}

//Finder::size_type RegexBaseFinder::Internal::do_find
Finder::size_type RegexFinder::do_find
	(const string & str, size_type from)
{
	int eflags= 0;
	if (from != 0)
		eflags|= REG_NOTBOL;
	int r= regexec (& reg, str.c_str () + from,
		pmlen, & pmatch [0], eflags);
	switch (r)
	{
	case 0:
		//desp= 0;
		desp= from;
		//setlength ();
		return pmatch [0].rm_so + from;
	case REG_NOMATCH:
		//return Finder::npos;
		return npos;
	default:
		checkresult (r);
		return npos; // Make the compiler happy.
	}
}

//Finder::size_type RegexBaseFinder::Internal::find_back
Finder::size_type RegexBackFinder::find
	(const string & str)
{
	TRF;

	int eflags= 0;
	const char * cstr= str.c_str ();
	int r= regexec (& reg, cstr, pmlen, & pmatch [0], eflags);
	if (r == REG_NOMATCH)
		return npos;
	checkresult (r);
	size_type res= pmatch [0].rm_so;
	eflags|= REG_NOTBOL;
	desp= 0;
	//setlength ();
	while ( (r= regexec (& reg, cstr + res + 1,
		pmlen, & pmatch [0], eflags) ) == 0)
	{
		desp= res + 1;
		res+= pmatch [0].rm_so + 1;
		//setlength ();
	}
	return res;
}

//Finder::size_type RegexBaseFinder::Internal::find_back
Finder::size_type RegexBackFinder::find
	(const string & str, size_type from)
{
	TRF;

	int eflags= 0;
	const char * cstr= str.c_str ();
	int r= regexec (& reg, cstr, pmlen, & pmatch [0], eflags);
	if (r == REG_NOMATCH)
		return npos;
	checkresult (r);
	if (static_cast <size_t> (pmatch [0].rm_eo) >= from)
		return npos;
	size_type res= pmatch [0].rm_so;
	eflags|= REG_NOTBOL;
	desp= 0;
	//setlength ();

	//vregmatch auxmatch (pmatch.size () );
	ASSERT (auxmatch.size () == pmatch.size () );

	while ( (r= regexec (& reg, cstr + res + 1,
		pmlen, & auxmatch [0], eflags) ) == 0)
	{
		//desp= res + 1;
		size_t endres= res + auxmatch [0].rm_eo + 1;
		if (endres >= from)
			break;
		desp= res + 1;
		pmatch.swap (auxmatch);
		res+= pmatch [0].rm_so + 1;
		//setlength ();
	}
	return res;
}

//Finder::string RegexBaseFinder::Internal::replace
Finder::string RegexBaseFinder::replace
	(const string & str, const string & line)
{
	TRFDEBS ("to: " << str << " line: " + line);

	size_type pos= 0;
	string nstr= str;
	while ( (pos= nstr.find ('$', pos) ) != npos)
	{
		// Ooops, bug. Fixed.
		//if (pos >= str.size () - 1)
		if (pos >= nstr.size () - 1)
			break;
		char c= nstr [pos + 1];
		if (c == '$')
		{
			nstr.erase (pos, 1);
			++pos;
			continue;
		}
		if (! isdigit (c) )
		{
			pos+= 2;
			continue;
		}
		size_t n= c - '0';
		if (n > pmlen)
			nstr.erase (pos, 2);
		else
		{
			regoff_t ini= pmatch [n].rm_so;
			if (ini == regoff_t (-1) )
				nstr.erase (pos, 2);
			else
			{
				regoff_t l= pmatch [n].rm_eo - ini;
				nstr.replace (pos, 2, line, ini + desp, l);
				pos-= 2;
				pos+= l;
			}
		}
	}
	return nstr;
}

//Finder::string RegexBaseFinder::Internal::replace (const string & function,
Finder::string RegexBaseFinder::replace (const string & function,
	const string & line, size_type, void * data)
{
	TRF;

	std::vector <string> vargs;
	for (size_t n= 0; n < pmlen; ++n)
	{
		regoff_t ini= pmatch [n].rm_so;
		if (ini == regoff_t (-1) )
			vargs.push_back (string () );
		else
		{
			regoff_t l= pmatch [n].rm_eo - ini;
			vargs.push_back (line.substr (ini + desp, l) );
		}
	}
	const char * * argv= new const char * [pmlen];
	for (size_t n= 0; n < pmlen; ++n)
	{
		argv [n]= vargs [n].c_str ();
		TRDEB (vargs [n] );
	}

	char * result;
	qpfunctionresult qpr= qperl_function_args_r
		(function.c_str (), data, pmlen, argv, & result);
	switch (qpr)
	{
	case qpfunction_notfound:
		throw "Function: " + function + " does not exist";
	case qpfunction_failed:
		throw "Function: " + function + " has failed";
	case qpfunction_ok:
		break;
	}
	std::string r (result ? result : "");

	TRDEBS ("Returned: " << r);

	qperl_free_str (result);
	delete [] argv;
	return r;
}

//Finder::size_type RegexBaseFinder::Internal::length () const
Finder::size_type RegexBaseFinder::length () const
{
	return pmatch [0].rm_eo - pmatch [0].rm_so;
	//return szlength;
}

//void RegexBaseFinder::Internal::checkresult (int r)
void RegexBaseFinder::checkresult (int r)
{
	if (r != 0)
	{
		char buffer [256];
		regerror (r, & reg, buffer, sizeof (buffer) );
		throw std::string (buffer);
	}
}

//***********************************************
//		RegexBaseFinder
//***********************************************

#if 0
RegexBaseFinder::RegexBaseFinder (const string & searched, bool nocase) :
	pin (new Internal (searched, nocase) )

{
}

RegexBaseFinder::~RegexBaseFinder ()
{
	delete pin;
}

Finder::size_type RegexBaseFinder::find (const string & str)
{
	return pin->find (str);
}

Finder::size_type RegexBaseFinder::find (const string & str, size_type from)
{
	return pin->find (str, from);
}

Finder::size_type RegexBaseFinder::length () const
{
	return pin->length ();
}

Finder::string RegexBaseFinder::replace
	(const string & str, const string & line)
{
	return pin->replace (str, line);
}

Finder::string RegexBaseFinder::replace (const string & function,
	const string & line, size_type pos, void * data)
{
	return pin->replace (function, line, pos, data);
}
#endif

//***********************************************
//		RegexFinder
//***********************************************

RegexFinder::RegexFinder (const string & searched, bool nocase) :
	RegexBaseFinder (searched, nocase)
{
}

#if 0
Finder::size_type RegexFinder::find (const string & str)
{
	return pin->find (str);
}

Finder::size_type RegexFinder::find (const string & str, size_type from)
{
	return pin->find (str, from);
}
#endif

//***********************************************
//		RegexBackFinder
//***********************************************


RegexBackFinder::RegexBackFinder (const string & searched, bool nocase) :
	RegexBaseFinder (searched, nocase)
{
	auxmatch.resize (pmatch.size () );
}

#if 0
Finder::size_type RegexBackFinder::find (const string & str)
{
	return pin->find_back (str);
}

Finder::size_type RegexBackFinder::find (const string & str, size_type from)
{
	return pin->find_back (str, from);
}
#endif

// End of finder.cpp

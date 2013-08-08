#ifndef INCLUDE_LINKPERL_H
#define INCLUDE_LINKPERL_H

// linkperl.h
// Revision 4-oct-2004

#include <string>
#include <vector>
#include <stdexcept>

class ScreenImpl;

class LinkPerl {
public:
	LinkPerl (char * * env, ScreenImpl * pscreen);
	~LinkPerl ();
	void perlinitdone ();

	void callperlfunc (const std::string & func);
	void callperlfunc (const std::string & func,
		const std::vector <std::string> vargs);
	int callperleval (const std::string & expression);

	class Error : public std::runtime_error {
	public:
		Error (const std::string & str);
	};
};

#endif

// End of linkperl.h

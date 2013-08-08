// linkperl.cpp
// Revision 27-jan-2006

#include "linkperl.h"

#include "qperl.h"

#include "trace.h"

using std::string;

LinkPerl::Error::Error (const std::string & str) :
	std::runtime_error (str)
{
}

LinkPerl::LinkPerl (char * * env, ScreenImpl * pscreen)
{
	TRF;

	if (qperl_init (env, pscreen) )
		throw Error ("Failed perl initialization");
}

LinkPerl::~LinkPerl ()
{
	TRF;

	qperl_destroy ();
}

void LinkPerl::perlinitdone ()
{
	qperl_init_done ();
}

void LinkPerl::callperlfunc (const std::string & func)
{
	switch (qperl_function (func.c_str (), this) )
	{
	case qpfunction_ok:
		break;
	case qpfunction_notfound:
		throw Error (string ("Undefined function ") + func);
		break;
	case qpfunction_failed:
		throw Error (string ("Error in function ") + func);
		break;
	}
}

void LinkPerl::callperlfunc (const std::string & func,
	const std::vector <std::string> vargs)
{
	const size_t nargs= vargs.size ();
	const char * * argv= new const char * [nargs];
	for (size_t i= 0; i < nargs; ++i)
		argv [i]= vargs [i].c_str ();
	qperl_function_args (func.c_str (), this, nargs, argv);
}

int LinkPerl::callperleval (const std::string & expression)
{
	return qperl_eval (expression.c_str (), this);
}

// End of linkperl.cpp

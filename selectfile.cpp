// selectfile.cpp
// Revision 29-jan-2006

#include "selectfile.h"

#include "select.h"
#include "dirutil.h"

#include <algorithm>

RedrawerSelectFile::~RedrawerSelectFile ()
{
}

std::string selectfile (otermstream & ot, const std::string & title,
	RedrawerSelectFile & redrawer)
{
	using namespace dirutil;

	std::vector <std::string> v;
	std::string dir= ".";

	std::string result;
	for (;;)
	{
		directory (dir, v);
		std::sort (v.begin (), v.end () );
	
		size_t r= selectlist (ot, title, v);
		if (r == size_t (-1) )
			break;
		std::string sel= enter_dir (dir, v [r] );
		if (! is_directory (sel) )
		{
			result= sel;
			break;
		}
		dir= sel;
		redrawer.doit ();
	}
	return result;
}

// End of selectfile.cpp

#ifndef INCLUDE_SELECTFILE_H_
#define INCLUDE_SELECTFILE_H_

// selectfile.h
// Revision 17-oct-2005

#include <string>

#include "termstream.h"

class RedrawerSelectFile {
public:
	virtual void doit ()= 0;
	virtual ~RedrawerSelectFile ();
};

std::string selectfile (otermstream & ot, const std::string & title,
	RedrawerSelectFile & redrawer);

#endif

// End of selectfile.h

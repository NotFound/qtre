#ifndef INCLUDE_SELECT_H_
#define INCLUDE_SELECT_H_

// select.h
// Revision 29-jan-2006

#include "termstream.h"

#include <string>
#include <vector>

size_t selectlist (otermstream & ot, const std::string & title,
	const std::vector <std::string> & vstr);

#endif

// End of select.h

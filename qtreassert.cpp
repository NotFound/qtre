// qtreassert.cpp
// Revision 19-sep-2003

#include <stdexcept>
#include <sstream>

#ifndef NDEBUG

extern "C"
void qtre_assert_failed	(const char * a, const char * file, size_t line)
{
	std::ostringstream oss;
	oss << "ASSERTION FAILED: " <<	a << " in file " << file <<
		" line " << line;
	throw std::logic_error (oss.str () );
}

#endif

// End of qtreassert.cpp

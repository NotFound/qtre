#ifndef INCLUDE_UTIL_H_
#define INCLUDE_UTIL_H_

// util.h
// Revision 27-nov-2003

#include <sstream>

namespace util {

template
	<class DEST, class ORG, class EX>
DEST checked_cast (ORG org, EX ex)
{
	DEST dest= static_cast <DEST> (org);
	if (static_cast <ORG> (dest) != org)
		throw ex;
	return dest;
}

template
	<class C, size_t N>
inline size_t dim_array (C (&) [N])
{ return N; }

template
	<class C, size_t N>
inline size_t dim_array (const C (&) [N])
{ return N; }

template <class C>
std::string to_string (const C & c)
{
	std::ostringstream oss;
	oss << c;
	return oss.str ();
}

template
	<class C>
class auto_buffer
{
public:
	auto_buffer (size_t s) :
		buffer (new C [s])
	{
	}
	~auto_buffer ()
	{
		delete [] buffer;
	}
	operator C * () { return buffer; }
private:
	auto_buffer (const auto_buffer &); // Forbidden
	auto_buffer & operator = (const auto_buffer &); // Forbidden
	C * buffer;
};

class bool_saver {
public:
	bool_saver (bool & save, bool value) :
		save (save),
		old (save)
	{
		save= value;
	}
	~bool_saver ()
	{
		save= old;
	}
private:
	bool & save;
	bool old;
};

} // namespace util

#endif

// End of util.h

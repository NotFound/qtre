#ifndef INCLUDE_STATUS_H_
#define INCLUDE_STATUS_H_

// status.h
// Revision 2-oct-2004

#include "termstream.h"

#include <string>

class StatusLine
{
public:
	StatusLine (int ancho, int y);
	~StatusLine ();
	void invalidate ();
	void refresh ();
	void update ();
	void settext (std::string str);
	int getkey (const std::string & str);
	void clear ();
	void lincol (int y, int x);
	char question (const std::string & q, const std::string & values);
private:
	void plincol ();
	void puttext ();
	otermstream ot;
	size_t max;
	int y;
	int x;
	std::string str;
	bool invalid;
};

#endif

// Fin de status.h

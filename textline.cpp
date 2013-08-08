// textline.cpp
// Revision 29-jul-2009

#include "textline.h"

#include "util.h"

#include "trace.h"
#undef ASSERT
#include "qtreassert.h"

#include <sstream>
#include <algorithm>
#include <memory>

using std::string;
using std::ostringstream;
using std::auto_ptr;

using util::to_string;

//*********************************************************
//		escape chars functions
//*********************************************************


unsigned char escapable (unsigned char c, bool useutf8)
{
	if (c < ' ' || c == '\x7F')
		return '^';
	else
	{
		const unsigned char cfrom ('\x7F');
		const unsigned char cto ('\xA0');
		if (useutf8)
		{
			#if 0
			// TODO: find a way to look what chars
			// are viewable in the terminal in utf8
			// mode.
			if (c > cfrom && c < cto)
				return '\\';
			else
				return '\0';
			#else
			return '\0';
			#endif
		}
		else
		{
			if (c > cfrom && c < cto)
				return '\\';
			else
				return '\0';
		}
	}
}

unsigned char escaped (unsigned char c)
{
	if (c == '\x7F') return '?';
	if (c > (unsigned char) '\x7F' && c < (unsigned char) '\xA0')
		return c - '\x80' + '@';
	return c + '@';
}

//*********************************************************
//			Col
//*********************************************************

const size_t size_max= size_t (0) - 1;
Col Col::colmax= Col (size_max);

//*********************************************************
//			TextLine::AuxImpl
//*********************************************************

class TextLine::AuxImpl {
public:
	AuxImpl (bool useutf8);
	void tracecache ();
	void clearcache (size_t from= 0);
	void clearcachefrompos (size_t pos);
	void cache (Col col, size_t pos);
	size_t col2pos (Col col, const std::string & s);
	Col pos2col (size_t pos, const std::string & s);
private:
	bool flagutf8;
	// Arbitrary values, must be tested.
	static const size_t ccsize= 32;
	static const size_t ccsep= 2048;

	Col colcache [ccsize];
	size_t poscache [ccsize];

	void cacheinsert (size_t n, Col col, size_t pos);
	void cacheappend (Col col, size_t pos);
};

TextLine::AuxImpl::AuxImpl (bool useutf8) :
	flagutf8 (useutf8)
{
	clearcache ();
}

void TextLine::AuxImpl::tracecache ()
{
	TRF;

	ostringstream opos, ocol;
	for (size_t i= 0; i < ccsize; ++i)
	{
		if (poscache [i] == size_t (-1) )
			break;
		opos << '\t' << poscache [i];
		ocol << '\t' << colcache [i];
	}
	TRDEBS (opos.str () << '-' << ocol.str () );
}

void TextLine::AuxImpl::clearcache (size_t from)
{
	ASSERT (from < ccsize);
	std::fill (colcache + from, colcache + ccsize, Col::max () );
	std::fill (poscache + from, poscache + ccsize, size_t (-1) );
}

void TextLine::AuxImpl::clearcachefrompos (size_t pos)
{
	int i;
	for (i= ccsize - 1; i > 0 && poscache [i] > pos; --i)
	{ }
	if (static_cast <size_t> (i) < ccsize - 1)
		clearcache (i + 1);
}

void TextLine::AuxImpl::cacheinsert (size_t n, Col col, size_t pos)
{
	TRF;

	ASSERT (n < ccsize);

	ASSERT (poscache [n] >= pos);
	if (pos - poscache [n] <= ccsep)
		return;

	if (n < ccsize / 2 || poscache [ccsize -1] == size_t (-1) )
	{
		for (size_t i= ccsize - 1; i > n; --i)
		{
			poscache [i]= poscache [i - 1];
			colcache [i]= colcache [i - 1];
		}
	}
	else
	{
		for (size_t i= 0; i < n; ++i)
		{
			poscache [i]= poscache [i + 1];
			colcache [i]= colcache [i + 1];
		}
	}
	poscache [n]= pos;
	colcache [n]= col;
	tracecache ();
}

void TextLine::AuxImpl::cacheappend (Col col, size_t pos)
{
	TRF;

	size_t posmax= poscache [ccsize -1];
	ASSERT (pos >= posmax);
	if (pos - posmax <= ccsep)
		return;

	for (size_t i= 0; i < ccsize - 1; ++i)
	{
		poscache [i]= poscache [i + 1];
		colcache [i]= colcache [i + 1];
	}
	poscache [ccsize - 1]= pos;
	colcache [ccsize - 1]= col;
	tracecache ();
}

void TextLine::AuxImpl::cache (Col col, size_t pos)
{
	TRFDEBS ("Col: " << col << " Pos: " << pos);

	if (pos < ccsep)
		return;

	size_t i= 0;
	for ( ; i < ccsize && pos >= poscache [i]; ++i)
		continue;
	switch (i)
	{
	case 0:
		if (poscache [0] > pos)
		{
			cacheinsert (0, col, pos);
		}
		break;
	case ccsize:
		ASSERT (pos >= poscache [ccsize - 1] );
		cacheappend (col, pos);
		break;
	default:
		ASSERT (pos >= poscache [i - 1] );
		ASSERT (pos < poscache [i] );
		if ( (pos - poscache [i - 1] > ccsep) &&
			(i == ccsize -1 || poscache [i + 1] != pos) )
		{
			cacheinsert (i, col, pos);
		}
		else
		{
			TRDEB ("Not cached");
		}
	}
}

size_t TextLine::AuxImpl::col2pos (Col col, const std::string & s)
{
	size_t pos= 0;
	Col auxcol;
	unsigned char c;
	size_t l= s.size ();

	for (size_t i= 0; i <= ccsize; ++i)
	{
		if (i == ccsize || colcache [i] > col)
		{
			if (i > 0)
			{
				ASSERT (colcache [i - 1] <= col);
				auxcol= colcache [i - 1];
				pos= poscache [i - 1];
			}
			break;
		}
	}

	while (auxcol < col)
	{
		if (pos >= l)
			return size_max;
		c= s [pos];
		if (c == '\t')
			auxcol.nexttab ();
		else
			if (escapable (c, flagutf8) ) auxcol+= 2;
			else ++auxcol;
		++pos;
		if ( (pos % ccsep) == 0)
			cache (auxcol, pos);
	}
	return pos;
}

Col TextLine::AuxImpl::pos2col (size_t pos, const std::string & s)
{
	Col col;
	unsigned char c;
	size_t l= s.size ();

	size_t auxpos= 0;
	for (size_t i= 0; i <= ccsize; ++i)
	{
		if (i == ccsize || poscache [i] > pos)
		{
			if (i > 0)
			{
				ASSERT (poscache [i - 1] <= pos);
				auxpos= poscache [i - 1];
				col= colcache [i - 1];
			}
			break;
		}
	}

	for ( ; auxpos < pos; ++auxpos)
	{
		if ( (auxpos % ccsep) == 0)
			cache (col, auxpos);
		if (auxpos >= l)
			return Col::max ();
		c= s [auxpos];
		if (c == '\t')
			col.nexttab ();
		else
			if (escapable (c, flagutf8) ) col+= 2;
			else ++col;
	}
	return col;
}

//*********************************************************
//			TextLine
//*********************************************************

namespace {

bool iswordchar (char c)
{
	return isalnum (c) || c == '_';
}

#ifndef NDEBUG

class TextLineInstanceCounter {
public:
	TextLineInstanceCounter ();
	~TextLineInstanceCounter ();
	void operator ++ () { ++count; }
	void operator -- ();
private:
	size_t count;
};

TextLineInstanceCounter::TextLineInstanceCounter () : count (0)
{
	TRF;
}

TextLineInstanceCounter::~TextLineInstanceCounter ()
{
	TRF;

	if (count != 0)
	{
		TRDEBS ("ERROR: TextLine instance counter is not 0, is " <<
			count);
	}
}

void TextLineInstanceCounter::operator -- ()
{
	ASSERT (count > 0);
	--count;
}

TextLineInstanceCounter textlinecount;

#endif
// NDEBUG

} // namespace

TextLine::TextLine (bool useutf8) :
	aimpl (new AuxImpl (useutf8) )
{
	#ifndef NDEBUG
	++textlinecount;
	#endif
}

TextLine::TextLine (bool useutf8, const std::string & snue) :
	s (snue),
	aimpl (new AuxImpl (useutf8) )
{
	#ifndef NDEBUG
	++textlinecount;
	#endif
}

TextLine::~TextLine ()
{
	#ifndef NDEBUG
	--textlinecount;
	#endif
}

bool TextLine::empty () const
{
	return s.empty ();
}

size_t TextLine::size () const
{
	return s.size ();
}

const std::string & TextLine::str () const
{
	return s;
}

char TextLine::operator [] (size_t n) const
{
	ASSERT (n < s.size () );

	return s [n];
}

std::string TextLine::substr (size_t n, size_t fin) const
{
	return s.substr (n, fin);
}

std::string TextLine::wordat (Col col) const
{
	size_t pos= col2pos (col);
	while (pos > 0 && iswordchar (s [pos - 1] ) )
		--pos;
	size_t end= pos + 1;
	while (end < s.size () && iswordchar (s [end] ) )
		++end;
	return s.substr (pos, end - pos);
}

void TextLine::operator = (std::string snue)
{
	s= snue;
	aimpl->clearcache ();
}

void TextLine::operator += (char c)
{
	const size_t pos= s.size ();
	s+= c;
	//aimpl->clearcache ();
	aimpl->clearcachefrompos (pos);
}

void TextLine::operator += (const std::string & str)
{
	const size_t pos= s.size ();
	s+= str;
	aimpl->clearcachefrompos (pos);
}

void TextLine::operator += (const TextLine & tnue)
{
	const size_t pos= s.size ();
	s+= tnue.s;
	//aimpl->clearcache ();
	aimpl->clearcachefrompos (pos);
}

size_t TextLine::col2pos (Col col) const
{
	#if 0
	size_t pos= 0;
	Col i;
	unsigned char c;
	size_t l= size ();
	while (i < col)
	{
		if (pos >= l)
			return size_max;
		c= s [pos];
		if (c == '\t')
			//i= Text::nexttab (i);
			i.nexttab ();
		else
			if (escapable (c) ) i+= 2;
			else ++i;
		++pos;
	}
	return pos;
	#endif

	return aimpl->col2pos (col, s);
}

Col TextLine::pos2col (size_t pos) const
{
	#if 0
	Col col;
	unsigned char c;
	size_t l= size ();
	for (size_t i= 0; i < pos; ++i)
	{
		if (i >= l)
			return Col::max ();
		c= s [i];
		if (c == '\t')
			//col= Text::nexttab (col);
			col.nexttab ();
		else
			if (escapable (c) ) col+= 2;
			else ++col;
	}
	return col;
	#endif

	return aimpl->pos2col (pos, s);
}

Col TextLine::colmax () const
{
	return pos2col (size () );
}

void TextLine::deletechar (Col col)
{
	const size_t pos= col2pos (col);
	ASSERT (pos < s.size () );
	s.erase (pos, 1);
	//aimpl->clearcache ();
	aimpl->clearcachefrompos (pos);
}

void TextLine::deletecharpos (size_t pos)
{
	ASSERT (pos < s.size () );
	s.erase (pos, 1);
	//aimpl->clearcache ();
	aimpl->clearcachefrompos (pos);
}

void TextLine::insertchar (Col col, char c)
{
	size_t pos= col2pos (col);
	if (s.size () <= pos)
	{
		if (s.size () < pos)
			// TODO: this is actually unused, and need a revision.
			s+= string (pos - s.size (), ' ');
		s+= c;
	}
	else s.insert (pos, 1, c);
	//aimpl->clearcache ();
	aimpl->clearcachefrompos (pos);
}

void TextLine::insertcharpos (size_t pos, char c)
{
	if (s.size () <= pos)
	{
		// TODO: this is actually unused, and need a revision.
		if (s.size () < pos)
			s+= string (pos - s.size (), ' ');
		s+= c;
	}
	else s.insert (pos, 1, c);
	//aimpl->clearcache ();
	aimpl->clearcachefrompos (pos);
}

void TextLine::insert (const std::string & str)
{
	s.insert (0, str);
	aimpl->clearcache ();
}

void TextLine::replace (size_t pos, std::string::size_type l,
	const std::string & str)
{
	s.replace (pos, l, str);
	//aimpl->clearcache ();
	aimpl->clearcachefrompos (pos);
}

std::ostream & operator << (std::ostream &os, const TextLine & tl)
{
	os << tl.s;
	return os;
}

//*********************************************************
//			VecTextLine
//*********************************************************

VecTextLine::VecTextLine (bool useutf8) :
	flagutf8 (useutf8)
{
	TRF;
}

VecTextLine::~VecTextLine ()
{
	TRF;

	for (size_t i= 0; i < size (); ++i)
		delete vtl [i];
}

VecTextLine::VecTextLine (const VecTextLine & other) :
	vtl (other.vtl.size () ),
	flagutf8 (other.flagutf8)
{
	TRF;

	size_t i= 0;
	try
	{
		for (; i < size (); ++i)
		{
			//vtl [i]= new TextLine (other.vtl [i]->str () );
			vtl [i]= new TextLine (other.vtl [i] );
		}
	}
	catch (...)
	{
		for (size_t j= 0; j < i; ++j)
			delete vtl [j];
		throw;
	}
}

void VecTextLine::swap (VecTextLine & other)
{
	vtl.swap (other.vtl);
}

bool VecTextLine::empty () const
{
	return vtl.empty ();
}

size_t VecTextLine::size () const
{
	return vtl.size ();
}

void VecTextLine::clear ()
{
	for (size_t i= 0; i < size (); ++i)
		delete vtl [i];
	vtl.clear ();
}

void VecTextLine::push_back ()
{
	auto_ptr <TextLine> ptr (new TextLine (flagutf8) );
	vtl.push_back (ptr.get () );
	ptr.release ();
}

void VecTextLine::push_back (const std::string & str)
{
	auto_ptr <TextLine> ptr (new TextLine (flagutf8, str) );
	vtl.push_back (ptr.get () );
	ptr.release ();
}

void VecTextLine::insert_at (size_t n)
{
	auto_ptr <TextLine> ptr (new TextLine (flagutf8) );
	vtl.insert (vtl.begin () + n, ptr.get () );
	ptr.release ();
}

void VecTextLine::insert_at (size_t n, const std::string & str)
{
	auto_ptr <TextLine> ptr (new TextLine (flagutf8, str) );
	vtl.insert (vtl.begin () + n, ptr.get () );
	ptr.release ();
}

void VecTextLine::erase (size_t n)
{
	if (n >= vtl.size () )
		throw range ("in erase");
	delete vtl [n];
	vtl.erase (vtl.begin () + n);
}

VecTextLine::const_reference VecTextLine::operator [] (size_t n) const
{
	if (n >= vtl.size () )
		throw range ("in operator [ ] const");
	return * vtl [n];
}

VecTextLine::reference VecTextLine::operator [] (size_t n)
{
	if (n >= vtl.size () )
		throw range ("in operator [ ]");
	return * vtl [n];
}

// End of textline.cpp

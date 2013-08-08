// text.cpp
// Revision 22-sep-2006

#include "text.h"

#include "dirutil.h"

#include "trace.h"
#undef ASSERT
#include "qtreassert.h"

#include "config_filebuf.h"


#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <strstream>
#include <vector>
#include <algorithm>
#include <new>
#include <stdexcept>

#include <string.h>
#include <errno.h>

//#if __GNUC__ > 2
#if HAVE_EXT_STDIO_FILEBUF_H

#include <ext/stdio_filebuf.h>
typedef __gnu_cxx::stdio_filebuf <char> stdio_stream;

#endif

using std::string;
using std::vector;
using std::swap;
using std::for_each;
using std::runtime_error;

using dirutil::delete_file;
using dirutil::create_temp_file;
using dirutil::copy_remote_file;

//*********************************************************
//			Text::Error
//*********************************************************

Text::Error::Error (const string & str) :
	runtime_error (str)
{ }

Text::ErrorSaving::ErrorSaving (const string & filename) :
	Error (string ("Error saving ") + filename +
		": " + strerror (errno) )
{ }

Text::ErrorReadOnly::ErrorReadOnly () :
	Error ("Cannot write, read only file")
{ }

//*********************************************************
//			Text
//*********************************************************

namespace {

bool do_load (FILE * fin, Text::VecLine & t, bool useutf8);
bool do_load (const string & filename, Text::VecLine & t, bool useutf8);

bool set_text (const char * str, Text::VecLine & t, bool useutf8);

void do_save (const string & filename, const Text::VecLine & t, bool useutf8);

} // namespace

Text::Text (bool useutf8) :
	flagutf8 (useutf8),
	t (useutf8)
{
	ASSERT (t.empty () );
	//t.push_back (TextLine () );
	t.push_back ();
}

Text::~Text ()
{ }

void Text::operator = (const Text & other)
{
	VecLine newt (other.t);
	t.swap (newt);
}

void Text::swap (Text & other)
{
	t.swap (other.t);
}

bool Text::empty () const
{
	return t.size () == 1 && t [0].empty ();
}

size_t Text::size () const
{
	size_t l= t.size ();
	size_t r= l - 1;
	for (size_t i= 0; i < l; ++i)
		r+= t [i].size ();
	return r;
}

size_t Text::lines () const
{
	return t.size ();
}

bool Text::getutf8 () const
{
	return flagutf8;
}

void Text::setutf8 (bool f)
{
	flagutf8= f;
}

const TextLine & Text::operator [] (size_t n) const
{
	if (n >= t.size () )
		throw runtime_error ("Line does not exist");
	return t [n];
}

std::string Text::wordat (size_t lin, Col col) const
{
	return t [lin].wordat (col);
}

void Text::copyselection (const Selection & sel, Text & copyto) const
{
	TRF;

	copyto.clear ();
	size_t linfirst= sel.begin ().lin;
	size_t linlast= sel.end ().lin;
	size_t posfirst= sel.begin ().pos;
	size_t poslast= sel.end ().pos;
	string s;
	if (linfirst == linlast)
	{
		s= t [linfirst].substr (posfirst, poslast - posfirst);
		copyto.setline (0, s);
	}
	else
	{
		s= t [linfirst].substr (posfirst);
		copyto.setline (0, s);
		for (size_t i= linfirst + 1; i < linlast; ++i)
			copyto.push_back (t [i].str () );
		s= t [linlast].substr (0, poslast);
		copyto.push_back (s);
	}
}

std::string Text::str () const
{
	TRF;

	std::string aux;
	for (size_t i= 0, nlines= t.size (); i < nlines; ++i)
	{
		aux+= t [i].str ();
		if (i != nlines - 1)
			aux+= '\n';
	}
	return aux;
}

void Text::clear ()
{
	TRF;

	t.clear ();
	//t.push_back (TextLine () );
	t.push_back ();
}

bool Text::load (FILE * fin)
{
	TRF;

	return do_load (fin, t, flagutf8);
}

bool Text::load (const std::string & strFile)
{
	TRF;

	return do_load (strFile, t, flagutf8);
}

void Text::save (const std::string & strFile)
{
	TRF;

	do_save (strFile, t, flagutf8);
}

bool Text::set (const char * str)
{
	return set_text (str, t, flagutf8);
}

void Text::setline (size_t lin, const std::string & str)
{
	ASSERT (lin < t.size () );
	t [lin]= str;
}

void Text::deletecharpos (size_t lin, size_t pos)
{
	ASSERT (lin < t.size () );
	t [lin].deletecharpos (pos);
}

void Text::insertchar (size_t lin, Col col, unsigned char c)
{
	ASSERT (lin < t.size () );
	t [lin].insertchar (col, c);
}

void Text::insertcharpos (size_t lin, size_t pos, unsigned char c)
{
	ASSERT (lin < t.size () );
	t [lin].insertcharpos (pos, c);
}

void Text::append (size_t lin, const std::string & str)
{
	ASSERT (lin < t.size () );
	t [lin]+= str;
}

void Text::appendchar (size_t lin, unsigned char c)
{
	ASSERT (lin < t.size () );
	t [lin]+= c;
}

size_t Text::insertline (size_t lin, Col col, bool autoindent)
{
	//t.insert (t.begin () + lin + 1, TextLine () );
	t.insert_at (lin + 1);

	//TextLine & ant= t [lin];
	//TextLine & sigue= t [lin + 1];

	VecLine::reference ant= t [lin];
	VecLine::reference sigue= t [lin + 1];

	size_t pos= ant.col2pos (col);
	if (pos < ant.size () )
	{
		sigue= ant.substr (pos);
		ant= ant.substr (0, pos);
	}

	// Autoindent.

	size_t ipos= 0;
	if (autoindent)
	{
		size_t l= ant.size ();
		size_t i= 0;
		Col icol;
		//while (i < l && isspace (ant [i]) && ant [i] != '\r')
		for ( ; i < l && (ant [i] == ' ' || ant [i] == '\t'); ++i)
		{
			if (ant [i] == ' ')
				++icol;
			else
				//icol= (icol / 8 + 1) * 8;
				icol.nexttab ();
		}
		if (i > 0)
		{
			//sigue= ant.substr (0, i) + sigue.str ();
			std::string pre;
			size_t ntab= icol.getn () / 8;
			if (ntab)
			{
				pre= std::string (ntab, '\t');
				ipos= ntab;
			}
			size_t nspace= icol.getn () % 8;
			if (nspace)
			{
				pre+= std::string (nspace, ' ');
				ipos+= nspace;
			}
			sigue.insert (pre);
		}
	}
	return ipos;
}

void Text::insertline (size_t lin, const std::string & str)
{
	//t.insert (t.begin () + lin, TextLine (str) );
	t.insert_at (lin, str);
}

void Text::deleteline (size_t lin)
{
	if (lin < t.size () - 1)
	{
		//t.erase (t.begin () + lin);
		t.erase (lin);
	}
	else
	{
		ASSERT (lin == t.size () - 1);
		t [lin]= "";
	}
}

void Text::joinline (size_t lin)
{
	t [lin]+= t [lin + 1];
	//t.erase (t.begin () + lin + 1);
	t.erase (lin + 1);
}

void Text::replace (size_t lin, size_t pos, std::string::size_type l,
		const std::string & str)
{
	t [lin].replace (pos, l, str);
}

void Text::push_back (const std::string & s)
{
	t.push_back (s);
}

//*********************************************************
//		TextFile::FileId
//*********************************************************

TextFile::FileId::FileId ()
{
}

TextFile::FileId::FileId (const std::string & newname)
{
	set (newname);
}

TextFile::FileId::~FileId ()
{
}

void TextFile::FileId::operator = (const FileId & other)
{
	if (& other != this)
	{
		string newfilename= other.strFilename;
		string newremotename= other.strRemotename;
		erase ();
		std::swap (newfilename, strFilename);
		std::swap (newremotename, strRemotename);
	}
}

void TextFile::FileId::erase ()
{
	TRF;

	if (isremote () )
		delete_file (strFilename);
	strFilename.erase ();
	strRemotename.erase ();
}

void TextFile::FileId::set (const std::string & newname)
{
	TRF;

	// Check for remote file.

	string newremotename;
	string newfilename;
	const string::size_type possep= newname.find (':');
	if (possep != string::npos && possep != 0)
	{
		string remotefilename= newname.substr (possep + 1);
		if (remotefilename.empty () )
			throw runtime_error ("Invalid empty remote file name");
		TRDEBS ("Host: '" << newname.substr (0, possep) <<
			"' File: '" << remotefilename << '\'');
		newfilename= create_temp_file ();
		newremotename= newname;
	}
	else
	{
		if (possep == 0)
			newfilename= newname.substr (1);
		else
			newfilename= newname;
	}

	// Done that way for better exception safety.
	std::swap (strFilename, newfilename);
	std::swap (strRemotename, newremotename);
}

bool TextFile::FileId::empty () const
{
	return strFilename.empty ();
}

bool TextFile::FileId::isremote () const
{
	return ! strRemotename.empty ();
}

std::string TextFile::FileId::filename () const
{
	return strFilename;
}

std::string TextFile::FileId::remotename () const
{
	return strRemotename;
}

//*********************************************************
//			TextFile
//*********************************************************

TextFile::TextFile (bool useutf8) :
	Text (useutf8),
	isSaved (true),
	isReadOnly (false)
{
}

TextFile::~TextFile ()
{
	fileid.erase ();
}

bool TextFile::saved () const
{
	return isSaved;
}

bool TextFile::modified () const
{
	return ! isSaved;
}

void TextFile::modified (bool f)
{
	isSaved= ! f;
}

bool TextFile::readonly () const
{
	return isReadOnly;
}

void TextFile::readonly (bool f)
{
	isReadOnly= f;
}

bool TextFile::getutf8 () const
{
	return Text::getutf8 ();
}

void TextFile::setutf8 (bool f)
{
	Text::setutf8 (f);
}

void TextFile::checkreadonly ()
{
	if (isReadOnly)
		throw ErrorReadOnly ();
}

bool TextFile::hasname () const
{
	return ! fileid.empty ();
}

std::string TextFile::name () const
{
	if (fileid.isremote () )
		return fileid.remotename ();
	else
		return fileid.filename ();
}

void TextFile::clear ()
{
	TRF;

	checkreadonly ();

	Text::clear ();

	isSaved= true;
	fileid.erase ();
}

bool TextFile::load (FILE * fin)
{
	TRF;

	checkreadonly ();
	bool r= Text::load (fin);

	fileid.erase ();

	isSaved= false;
	return r;
}

bool TextFile::load (const std::string & strFile)
{
	TRFDEBS ("Loading '" << strFile << '\'' );

	checkreadonly ();

	FileId newid (strFile);
	if (newid.isremote () )
	{
		copy_remote_file (newid.remotename (), newid.filename () );
	}
	bool r= Text::load (newid.filename () );
	fileid= newid;

	isSaved= true;

	return r;
}

void TextFile::save ()
{
	if (fileid.empty () )
		throw Error ("Can't save without filename");

	Text::save (fileid.filename () );

	if (fileid.isremote () )
	{
		copy_remote_file (fileid.filename (), fileid.remotename () );
	}

	isSaved= true;
}

void TextFile::save (const std::string & strFile)
{
	FileId newid (strFile);
	Text::save (newid.filename () );
	if (newid.isremote () )
	{
		copy_remote_file (newid.filename (), newid.remotename () );
	}
	fileid= newid;

	isReadOnly= false;
	isSaved= true;
}

bool TextFile::set (const char * str)
{
	checkreadonly ();

	bool r= Text::set (str);

	fileid.erase ();

	isSaved= false;
	return r;
}

void TextFile::setline (size_t lin, const std::string & str)
{
	checkreadonly ();
	Text::setline (lin, str);
	isSaved= false;
}

void TextFile::deletecharpos (size_t lin, size_t pos)
{
	checkreadonly ();
	Text::deletecharpos (lin, pos);
	isSaved= false;
}

void TextFile::insertchar (size_t lin, Col col, unsigned char c)
{
	checkreadonly ();
	Text::insertchar (lin, col, c);
	isSaved= false;
}

void TextFile::insertcharpos (size_t lin, size_t pos, unsigned char c)
{
	checkreadonly ();
	Text::insertcharpos (lin, pos, c);
	isSaved= false;
}

void TextFile::append (size_t lin, const std::string & str)
{
	checkreadonly ();
	Text::append (lin, str);
	isSaved= false;
}

void TextFile::appendchar (size_t lin, unsigned char c)
{
	checkreadonly ();
	Text::appendchar (lin, c);
	isSaved= false;
}

size_t TextFile::insertline (size_t lin, Col col, bool autoindent)
{
	checkreadonly ();

	size_t ipos= Text::insertline (lin, col, autoindent);

	isSaved= false;
	return ipos;
}

void TextFile::insertline (size_t lin, const std::string & str)
{
	checkreadonly ();
	Text::insertline (lin, str);
	isSaved= false;
}

void TextFile::deleteline (size_t lin)
{
	checkreadonly ();
	Text::deleteline (lin);
	isSaved= false;
}

void TextFile::joinline (size_t lin)
{
	checkreadonly ();
	Text::joinline (lin);
	isSaved= false;
}

void TextFile::replace (size_t lin, size_t pos, std::string::size_type l,
		const std::string & str)
{
	checkreadonly ();
	Text::replace (lin, pos, l, str);
	isSaved= false;
}

void TextFile::push_back (const std::string & s)
{
	checkreadonly ();
	Text::push_back (s);
	isSaved= false;
}

//*********************************************************
//			Auxiliares
//*********************************************************

namespace {

bool load_from_stream (std::istream & in, Text::VecLine & t, bool useutf8)
{
	TRF;

	t.clear ();
	char c;
	string s;
	in.unsetf (std::ios::skipws);
	while (in >> c)
	{
		if (c == '\n')
		{
			t.push_back (s);
			s.erase ();
		}
		else
		{
			if (useutf8)
			{
				#if 0
				unsigned char uc (c);
				if ( (uc & 0xFC) == 0xC0)
				{
					char c2 (0);
					in >> c2;
					unsigned char uc2 (c2);
					uc= (uc << 6) | (uc2 & 0x3F);
					c= uc;
				}
				#else
				mbtowc (0, 0, 0);
				wchar_t wc;
				if (mbtowc (& wc, & c, 1) != 1)
				{
					std::cerr << "HO!\n";
					static const size_t maxchar= 8;
					char aux [maxchar];
					aux [0]= c;
					in >> c;
					if (in)
					{
						std::cerr << "HI 1!\n";
						aux [1]= c;
						mbtowc (0, 0, 0);
						if (mbtowc (& wc, aux, 2) == 2)
						{
							std::cerr << "HI 2!\n";
							c= static_cast <unsigned char> (wc);
						}
						else
						{
							in >> c;
							if (in)
							{
								std::cerr << "HI 3!\n";
								aux [2]= c;
								mbtowc (0, 0, 0);
								if (mbtowc (& wc, aux, 3) == 3)
								{
									std::cerr << "HI 4!\n";
									c= static_cast <unsigned char> (static_cast <unsigned int> (wc) );
									std::cerr << static_cast <unsigned long> (wc) << ", " << static_cast <unsigned int> (c) << '\n';
								}
							}
						}
					}
				}
				#endif
			}
			s+= c;
		}
	}
	t.push_back (s);
	if (! in.eof () )
		throw runtime_error ("Error reading file");
	return true;
}

#if 0
bool do_load (FILE * fin, Text::VecLine & t, bool useutf8)
{
	#if __GNUC__ > 2

	#if __GNUC__ > 3

	__gnu_cxx::stdio_filebuf <char> bufin
		(fileno (fin), std::ios::in, size_t (4096) );

	#else

	stdio_stream bufin
		(fileno (fin), std::ios::in, false, 4096);
	//if (! bufin.is_open () )
	//	return false;
	//std::istream sin (& bufin);

	#endif

	if (! bufin.is_open () )
		return false;
	std::istream sin (& bufin);

	#else

	std::ifstream sin (fileno (fin) );

	#endif

	return load_from_stream (sin, t, useutf8);
}
#endif

bool do_load (FILE * fin, Text::VecLine & t, bool useutf8)
{
	#if FILEBUF_PARAMS >= 3

	#if FILEBUF_PARAMS == 3
	__gnu_cxx::stdio_filebuf <char> bufin
		(fileno (fin), std::ios::in, size_t (4096) );
	#elif FILEBUF_PARAMS == 4
	stdio_stream bufin (fileno (fin), std::ios::in, false, 4096);
	#else
	#error unexpected value of FILEBUF_PARAMS
	#endif

	if (! bufin.is_open () )
		return false;
	std::istream sin (& bufin);

	#else

	std::ifstream sin (fileno (fin) );

	#endif

	return load_from_stream (sin, t, useutf8);
}

bool do_load (const string & filename, Text::VecLine & t, bool useutf8)
{
	TRFDEBS (std::string ("loading ") << filename);

	std::ifstream in;
	in.open (filename.c_str () );
	if (! in.is_open () )
	{
		if (errno != ENOENT)
		{
			std::ostringstream oss;
			oss << "al abrir " << filename << ": (" <<
				errno << ") " << strerror (errno);
			throw runtime_error (oss.str () );
		}
		else
		{
			t.clear ();
			//t.push_back (TextLine () );
			t.push_back ();
			return false;
		}
	}
	return load_from_stream (in, t, useutf8);
}

bool set_text (const char * str, Text::VecLine & t, bool useutf8)
{
	TRF;

	std::istrstream in (str);
	return load_from_stream (in, t, useutf8);
}

#if 0

class savefunc {
public:
	savefunc (std::ostream & nos) : os (nos), flag (false) { }
	//void operator () (const TextLine & tl)
	void operator () (Text::VecLine::const_reference tl)
	{
		if (flag)
			os << '\n';
		else
			flag= true;
		os << tl;
	}
private:
	std::ostream & os;
	bool flag;
};

void do_save (const string & filename, const Text::VecLine & t)
{
	TRACEFUNC (tr, "do_save");
	TRMESSAGE (tr, std::string ("saving ") + filename);

	std::ofstream out (filename.c_str () );
	if (! out.is_open () )
		throw Text::ErrorSaving (filename);
	for_each (t.begin (), t.end (), savefunc (out) );
	out.close ();
	if (! out.good () )
		throw Text::ErrorSaving (filename);
}

#else

// Testing a simpler way.

class PutUtf8 {
public:
	PutUtf8 (std::ostream & os_n) : os (os_n) { }
	void operator () (char c);
private:
	std::ostream & os;
};

void PutUtf8::operator () (char c)
{
	unsigned char uc (static_cast <unsigned char> (c) );
	if (uc & '\x80')
	{
		unsigned char uc1= '\xC0' | (uc >> 6);
		unsigned char uc2= (uc & '\x3F') | '\x80';
		os << uc1 << uc2;
	}
	else
	{
		os << c;
	}
}

class savefunc {
public:
	savefunc (std::ostream & nos, bool useutf8) :
		os (nos), firstline (true),
		flagutf8 (useutf8), pututf8 (nos)
	{ }
	void operator () (Text::VecLine::const_reference tl);
private:
	std::ostream & os;
	bool firstline;
	bool flagutf8;
	PutUtf8 pututf8;
};

void savefunc::operator () (Text::VecLine::const_reference tl)
{
	if (firstline)
		firstline= false;
	else
		os << '\n';
	if (flagutf8)
	{
		const string & s (tl.str () );
		for_each (s.begin (), s.end (), pututf8);
	}
	else
	{
		os << tl;
	}
}

void do_save (const string & filename, const Text::VecLine & t, bool useutf8)
{
	TRFDEBS (std::string ("saving '") << filename <<
		"' using utf8: " << (useutf8 ? "Yes" : "No") );

	std::ofstream out (filename.c_str () );
	if (! out.is_open () )
		throw Text::ErrorSaving (filename);

	ASSERT (! t.empty () );

	for_each (t.begin (), t.end (), savefunc (out, useutf8) );

	out.close ();
	if (! out.good () )
		throw Text::ErrorSaving (filename);
}

#endif

} // namespace

// Fin de text.cpp

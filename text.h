#ifndef INCLUDE_TEXT_H_
#define INCLUDE_TEXT_H_

// text.h
// Revision 29-jan-2006

#include "textline.h"

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

#include <stdio.h>


class PosText {
public:
	PosText () : lin (0), pos (0) { }
	PosText (size_t nlin, size_t npos) : lin (nlin), pos (npos) { }
	size_t lin, pos;
	bool operator < (const PosText &pt) const
	{
		if (lin < pt.lin)
			return true;
		if (lin == pt.lin && pos < pt.pos)
			return true;
		return false;
	}
	bool operator == (const PosText &pt) const
		{ return lin == pt.lin && pos == pt.pos; }
	bool operator != (const PosText & pt) const
		{ return lin != pt.lin || pos != pt.pos; }
	void set (size_t nlin, size_t npos)
		{ lin= nlin; pos= npos; }
};

class Selection {
public:
	void init (int lin, int pos)
	{
		ptBegin.set (lin, pos);
		ptEnd.set (lin, pos);
	}
	const PosText & begin () const { return ptBegin; }
	const PosText & end () const { return ptEnd; }
	bool empty () const { return ptBegin == ptEnd; }
	void movebegin (const PosText & npt) 
	{
		ptBegin= npt;
		adjust ();
	}
	void movebegin (int lin, int pos) { movebegin (PosText (lin, pos) ); }
	void moveend (const PosText & npt)
	{
		ptEnd= npt;
		adjust ();
	}
	void moveend (int lin, int pos) { moveend (PosText (lin, pos) ); }
private:
	PosText ptBegin, ptEnd;
	inline void adjust ()
		{ if (ptEnd < ptBegin) std::swap (ptBegin, ptEnd); }
};

class Text {
public:
	class Error : public std::runtime_error {
	public:
		Error (const std::string & str);
	};
	class ErrorSaving : public Error {
	public:
		ErrorSaving (const std::string & filename);
	};
	class ErrorReadOnly : public Error {
	public:
		ErrorReadOnly ();
	};

	//typedef std::vector <TextLine> VecLine;
	typedef VecTextLine VecLine;

	Text (bool useutf8);
	virtual ~Text ();
private:
	Text (const Text & other);
public:
	void operator = (const Text & other);
	void swap (Text & other);

	bool empty () const;
	size_t size () const;
	size_t lines () const;
	bool getutf8 () const;
	void setutf8 (bool f);

	const TextLine & operator [] (size_t n) const;
	std::string wordat (size_t lin, Col col) const;

	void copyselection (const Selection & sel, Text & copyto) const;

	std::string str () const;

	void clear ();
	bool set (const char * str);
	void setline (size_t lin, const std::string & str);
	void deletecharpos (size_t lin, size_t pos);
	void insertchar (size_t lin, Col col, unsigned char c);
	void insertcharpos (size_t lin, size_t pos, unsigned char c);
	void append (size_t lin, const std::string & str);
	void appendchar (size_t lin, unsigned char c);
	size_t insertline (size_t lin, Col col, bool autoindent);
	void insertline (size_t lin, const std::string & str);
	void deleteline (size_t lin);
	void deletechar (size_t lin, size_t col);
	void joinline (size_t lin);
	void replace (size_t lin, size_t pos, std::string::size_type l,
			const std::string & str);
	void push_back (const std::string & s);
protected:
	bool load (FILE * fin);
	bool load (const std::string & filename);
	void save (const std::string & strFile);
private:
	bool flagutf8;
	VecLine t;
};

namespace std {

template <> inline void swap (Text & one, Text & two)
{
	one.swap (two);
}

} // namespace std

class TextFile : public Text {
public:
	TextFile (bool useutf8);
	~TextFile ();
	bool saved () const;
	bool modified () const;
	void modified (bool f);
	bool readonly () const;
	void readonly (bool f);
	bool getutf8 () const;
	void setutf8 (bool f);
	bool hasname () const;
	std::string name () const;
	void clear ();

	bool load (FILE * fin);
	bool load (const std::string & strFile);
	void save ();
	void save (const std::string & strFile);

	bool set (const char * str);
	void setline (size_t lin, const std::string & str);
	void deletecharpos (size_t lin, size_t pos);
	void insertchar (size_t lin, Col col, unsigned char c);
	void insertcharpos (size_t lin, size_t pos, unsigned char c);
	void append (size_t lin, const std::string & str);
	void appendchar (size_t lin, unsigned char c);
	size_t insertline (size_t lin, Col col, bool autoindent);
	void insertline (size_t lin, const std::string & str);
	void deleteline (size_t lin);
	void joinline (size_t lin);
	void replace (size_t lin, size_t pos, std::string::size_type l,
			const std::string & str);
	void push_back (const std::string & s);
private:
	class FileId {
	public:
		FileId ();
		FileId (const std::string & newname);
		~FileId ();
		void operator = (const FileId & other);
		void erase ();
		void set (const std::string & newname);
		bool empty () const;
		bool isremote () const;
		std::string filename () const;
		std::string remotename () const;
	private:
		FileId (const FileId &); // Forbidden.
		std::string strFilename;
		std::string strRemotename;
	};
	FileId fileid;
	bool isSaved;
	bool isReadOnly;

	void checkreadonly ();
};

#endif

// Fin de text.h

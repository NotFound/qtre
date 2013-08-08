#ifndef INCLUDE_TEXTLINE_H_
#define INCLUDE_TEXTLINE_H_

// textline.h
// Revision 25-jan-2006

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>

unsigned char escapable (unsigned char c, bool useutf8);
unsigned char escaped (unsigned char c);

class Col {
public:
	class Bad : public std::exception {
		const char * what () { return "Bad column"; }
	};

	Col () : n (0) { }
	explicit Col (size_t nn) : n (nn) { }
	size_t getn () const { return n; }
	Col & operator += (size_t inc)
	{
		if (n > colmax.n - inc)
			throw Bad ();
		n+= inc;
		return * this;
	}
	Col operator + (size_t nn) const
	{
		if (n > colmax.n - nn)
			throw Bad ();
		return Col (n + nn);
	}
	Col operator + (const Col & col) const
		{ return Col (n + col.n); }
	Col & operator ++ ()
	{
		if (n == colmax.n)
			throw Bad ();
		++n;
		return * this;
	}
	Col operator - (const Col & col) const
	{
		if (col.n > n)
			throw Bad ();
		return Col (n - col.n);
	}
	bool operator == (const Col & col) const
		{ return n == col.n; }
	bool operator != (const Col & col) const
		{ return n != col.n; }
	bool operator < (const Col & col) const
		{ return n < col.n; }
	bool operator <= (const Col & col) const
		{ return n <= col.n; }
	bool operator > (const Col & col) const
		{ return n > col.n; }
	static Col & max () { return colmax; }
	/*
	Col nexttab () const
		{ return Col ( (n / 8 + 1) * 8); }
	Col previoustab () const
		{ return Col ( (n / 8) * 8); }
	*/
	void nexttab ()
		{ n= (n / 8 + 1) * 8; }
	void previoustab ()
		{ n= (n / 8) * 8; }
	bool odd () const
		{ return n & 1; }
	friend std::ostream & operator << (std::ostream & os, const Col & col);
private:
	static Col colmax;
	size_t n;
};

inline std::ostream & operator << (std::ostream & os, const Col & col)
{
	os << col.n;
	return os;
}

class TextLine {
	std::string s;
public:
	TextLine (bool useutf8);
	TextLine (bool useutf8, const std::string & snue);
	~TextLine ();
private:
	TextLine (const TextLine &); // Forbidden
	void operator = (const TextLine &); // Forbidden
public:
	bool empty () const;
	size_t size () const;
	const std::string & str () const;
	char operator [] (size_t n) const;
	std::string substr (size_t n, size_t fin= std::string::npos) const;
	std::string wordat (Col col) const;

	void operator = (std::string snue);
	void operator += (char c);
	void operator += (const std::string & str);
	void operator += (const TextLine & tnue);

	size_t col2pos (Col col) const;
	Col pos2col (size_t pos) const;
	Col colmax () const;

	void deletechar (Col col);
	void deletecharpos (size_t pos);
	void insertchar (Col col, char c);
	void insertcharpos (size_t pos, char c);
	void insert (const std::string & str);
	void replace (size_t pos, std::string::size_type l,
			const std::string & str);
	friend std::ostream & operator <<
		(std::ostream & os, const TextLine & tl);
private:
	class AuxImpl;
	AuxImpl *aimpl;
};

class TextLineRef {
public:
	TextLineRef (TextLine & tl) : tl (tl)
	{ }
	TextLineRef (const TextLineRef & tlf) : tl (tlf.tl)
	{ }
	size_t size () const { return tl.size (); }
	char operator [] (size_t n) const { return tl [n]; }
	std::string substr (size_t n, size_t fin= std::string::npos) const
	{ return tl.substr (n, fin); }

	void operator = (std::string snue) { tl= snue; }
	void operator += (char c) { tl+= c; }
	void operator += (const std::string & str) { tl+= str; }
	void operator += (const TextLine & tnue) { tl+= tnue; }
	void operator += (const TextLineRef & tlnue) { tl+= tlnue.tl; }

	size_t col2pos (Col col) const { return tl.col2pos (col); }

	void deletecharpos (size_t pos) { tl.deletecharpos (pos); }
	void insertchar (Col col, char c) { tl.insertchar (col, c); }
	void insertcharpos (size_t pos, char c) { tl.insertcharpos (pos, c); }
	void insert (const std::string & str) { tl.insert (str); }
	void replace (size_t pos, std::string::size_type l,
			const std::string & str)
	{ tl.replace (pos, l, str); }

	friend std::ostream & operator <<
		(std::ostream & os, const TextLineRef & tlr);
private:
	TextLine & tl;
};

inline std::ostream & operator <<
	(std::ostream & os, const TextLineRef & tlr)
{
	os << tlr.tl;
	return os;
}

class VecTextLine {
public:
	typedef TextLineRef reference;
	typedef const TextLine & const_reference;

	VecTextLine (bool useutf8);
	~VecTextLine ();
	VecTextLine (const VecTextLine &);
private:
	void operator = (const VecTextLine &);
public:
	void swap (VecTextLine & other);

	class range : public std::out_of_range {
	public:
		range (const std::string & str) :
			out_of_range ("VecTextLine index out of range: " +
				str)
		{ }
	};

	bool empty () const;
	size_t size () const;

	void clear ();
	void push_back ();
	void push_back (const std::string & str);
	void insert_at (size_t n);
	void insert_at (size_t n, const std::string & str);
	void erase (size_t n);

	const_reference operator [] (size_t n) const;
	reference operator [] (size_t n);

	class iterator {
		friend class VecTextLine;
		iterator (const VecTextLine & vtl, size_t pos) :
			vtl (vtl), pos (pos)
		{ }
	public:
		const TextLine & operator * () const { return vtl [pos]; }
		iterator & operator ++ ()
		{
			if (pos == size_t (-1) || pos >= vtl.size () )
				throw range ("in operator ++");
			++pos;
			return * this;
		}
		iterator operator ++ (int)
		{
			if (pos == size_t (-1) || pos >= vtl.size () )
				throw range ("in operator ++");
			iterator it (* this);
			++pos;
			return it;
		}
		bool operator != (const iterator it) const
		{
			return & vtl != & it.vtl || pos != it.pos;
		}
	private:
		const VecTextLine & vtl;
		size_t pos;
	};
	typedef const iterator const_iterator;

	iterator begin () { return iterator (* this, 0); }
	const_iterator begin () const { return iterator (* this, 0); }
	const_iterator end () const
		{ return iterator (* this, vtl.size () ); }
private:
	std::vector <TextLine *> vtl;
	bool flagutf8;
};

#endif

// Fin de textline.h

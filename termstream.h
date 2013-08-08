#ifndef INCLUDE_TERMSTREAM_H_
#define INCLUDE_TERMSTREAM_H_

// termstream.h
// Revision 7-feb-2007

#include "config_streambuf.h"

#include <stdio.h>

#include <iostream>

#if HAVE_STREAMBUF
#include <streambuf>
#elif HAVE_STREAMBUF_H
#include <streambuf.h>
#else
#error No streambuf
#endif

class Color
{
public:
	typedef short value;
	static const value
		Black= 0,
		Red= 1,
		Green= 2,
		Yellow= 3,
		Blue= 4,
		Magenta= 5,
		Cyan= 6,
		White= 7;
};

class termbuf : public std::streambuf
{
public:
	explicit termbuf (std::ios::openmode);
	void setWindow (void * window);
	int getkey ();
	int checkkey ();
	virtual int sync ();
protected:
	virtual int overflow (int ch= EOF);
	virtual int underflow ();
private:
	static const std::streamsize bufsize= 64;
	char defbuf [bufsize];
	void *w;
	class Control
	{
	public:
		Control ();
		~Control ();
	};
	Control control;
};


void termstream_set_terminal (FILE * termtouse);


class termstreambase : virtual public std::ios
{
protected:
	termbuf __my_tb;
public:
	termbuf * rdbuf ();
	static void shellmode ();
	static void progmode ();
	static void clr_all ();
	static void redraw_all ();
	void sync ();
	int getkey ();
	int checkkey ();
	int lines () const;
	int columns () const;
	int wherex () const;
	int wherey () const;
protected:
	termstreambase (std::ios::openmode which);
	void setWindow (void *window);
	void *w;
};


class ts_bool
{
public:
	std::ostream & (* f) (std::ostream &, bool);
	bool b;
	ts_bool (std::ostream & (*ff) (std::ostream &, bool), bool bb)
		: f (ff), b (bb) {}
};

class ts_2int
{
public:
	std::ostream & (* f) (std::ostream &, int, int);
	int i1, i2;
	ts_2int (std::ostream & (*ff) (std::ostream &, int, int),
			int ii1, int ii2)
		: f (ff), i1 (ii1), i2 (ii2)
	{}
};

class ts_short
{
public:
	std::ostream & (* f) (std::ostream &, short);
	short s;
	ts_short (std::ostream & (*ff) (std::ostream &, short), short ss)
		: f (ff), s (ss) {}
};


std::ostream & operator << (std::ostream & os, const ts_bool & tb);
std::ostream & operator << (std::ostream & os, const ts_2int & t2i);
std::ostream & operator << (std::ostream & os, const ts_short & ts);


class otermstream : public termstreambase, public std::ostream
{
public:
	otermstream (std::ios::openmode which= std::ios::out);
	otermstream (int nlines, int ncols, int y, int x);
	otermstream (otermstream & ot, int nlines, int ncols, int y, int x);
	~otermstream ();
	void update ();
	void border ();
	void clrscr ();
	void clreol ();
	void insertline ();
	void deleteline ();
	void at (int y, int x);
	inline void gotoxy (int x, int y) { at (y, x); }
	void inverse (bool flag);
	void delay (bool flag);
	void cursor (bool flag);
	int getBegX () const;
	int getBegY () const;
	int getWidth () const;
	int getHeight () const;
	void setcolor (short n, Color::value f, Color::value b);
	void color (short c);
	void background (short c);

	friend std::ostream & operator <<
		(std::ostream & os, const ts_bool & tb);
	friend std::ostream & operator <<
		(std::ostream & os, const ts_2int & t2i);
	friend std::ostream & operator <<
		(std::ostream & os, const ts_short & ts);
private:
	bool isStd;
};


std::ostream & inverse (std::ostream &os, bool b);

std::ostream & delay (std::ostream &os, bool b);

std::ostream & cursor (std::ostream &os, bool b);

std::ostream & gotoxy (std::ostream &os, int x, int y);

std::ostream & at (std::ostream & os, int y, int x);

std::ostream & color (std::ostream &os, short c);

ts_bool inverse (bool b);

ts_bool delay (bool b);

ts_bool cursor (bool b);

ts_2int gotoxy (int x, int y);

ts_2int at (int y, int x);

ts_short color (short c);

std::ostream & update (std::ostream & os);

std::ostream & clrscr (std::ostream &os);

std::ostream & clreol (std::ostream &os);

std::ostream & delline (std::ostream &os);

std::ostream & insline (std::ostream &os);

namespace key
{
const int
	Ctl_A=     0x01,
	Ctl_B=     0x02,
	Ctl_C=     0x03,
	Ctl_D=     0x04,
	Ctl_E=     0x05,
	Ctl_F=     0x06,
	Ctl_G=     0x07,
	Ctl_H=     0x08,
	Ctl_I=     0x09,
	Ctl_J=     0x0A,
	Ctl_K=     0x0B,
	Ctl_L=     0x0C,
	Ctl_M=     0x0D,
	Ctl_N=     0x0E,
	Ctl_O=     0x0F,
	Ctl_P=     0x10,
	Ctl_Q=     0x11,
	Ctl_R=     0x12,
	Ctl_S=     0x13,
	Ctl_T=     0x14,
	Ctl_U=     0x15,
	Ctl_V=     0x16,
	Ctl_W=     0x17,
	Ctl_X=     0x18,
	Ctl_Y=     0x19,
	Ctl_Z=     0x1A,

	Esc=       0x1B,

	Left=      0x0100,
	Right=     0x0101,
	Up=        0x0102,
	Down=      0x0103,
	PageUp=    0x0104,
	PageDown=  0x0105,
	Backspace= 0x0106,
	Delete=    0x0107,
	Home=      0x0108,
	End=       0x0109,

	Select=    0x010A,

	F1=  0x0201,
	F2=  0x0202,
	F3=  0x0203,
	F4=  0x0204,
	F5=  0x0205,
	F6=  0x0206,
	F7=  0x0207,
	F8=  0x0208,
	F9=  0x0209,
	F10= 0x020A,
	F11= 0x020B,
	F12= 0x020C;
}

#endif

// End of termstream.h

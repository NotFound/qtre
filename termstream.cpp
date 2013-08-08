// termstream.cpp
// Revision 7-feb-2007

#include "termstream.h"

#include "trace.h"

#include "config_curses.h"

#include <sstream>
#include <stdexcept>


#ifdef HAVE_NCURSESW_NCURSES_H

#include <ncursesw/ncurses.h>

#else

#ifdef HAVE_NCURSES_H
#include <ncurses.h>
#else
#error ncurses required
#endif

#endif

#undef border

using std::logic_error;
using std::runtime_error;


namespace
{

size_t instancesControl= 0;

FILE * usethisterm= NULL;

SCREEN * newscreen= NULL;

short color_curses (Color::value c)
{
	short v;
	switch (c) {
	case Color::Black:   v= COLOR_BLACK;   break;
	case Color::Red:     v= COLOR_RED;     break;
	case Color::Green:   v= COLOR_GREEN;   break;
	case Color::Yellow:  v= COLOR_YELLOW;  break;
	case Color::Blue:    v= COLOR_BLUE;    break;
	case Color::Magenta: v= COLOR_MAGENTA; break;
	case Color::Cyan:    v= COLOR_CYAN;    break;
	case Color::White:   v= COLOR_WHITE;   break;
	default:             v= COLOR_WHITE;    break;
	}
	return v;
}

} // namespace


//**************************************************************


void termstream_set_terminal (FILE * termtouse)
{
	TRF;

	if (termtouse == NULL)
		throw logic_error ("Null FILE * invalid");

	if (instancesControl != 0)
		throw logic_error ("Can't set terminal now");
	usethisterm= termtouse;
}


//**************************************************************


termbuf::termbuf (std::ios::openmode)
{
	setp (defbuf, defbuf + bufsize);
}

void termbuf::setWindow (void * window)
{
	w= window;
}


termbuf::Control::Control ()
{
	TRF;

	if (instancesControl == 0)
	{
		if (usethisterm == NULL)
		{
			initscr ();
		}
		else
		{
			newscreen= newterm (NULL, usethisterm, usethisterm);
			if (newscreen == NULL)
				throw runtime_error ("newterm failed");
			set_term (newscreen);
		}
		noecho ();
		raw ();
		//keypad (stdscr, TRUE); // It's done in setWindow
		start_color ();
	}
	++instancesControl;
}

termbuf::Control::~Control ()
{
	TRF;

	if (--instancesControl == 0)
	{
		endwin ();
		if (newscreen != NULL)
		{
			delscreen (newscreen);
			newscreen= NULL;
		}
	}
}


int termbuf::getkey ()
{
	using namespace key;
	sync ();
	int c= wgetch ( (WINDOW *) w);
	if (c == ERR)
		return 0;
	if (c & ~ 0xFF)
		switch (c)
		{
		case KEY_LEFT:      c= Left;      break;
		case KEY_RIGHT:     c= Right;     break;
		case KEY_UP:        c= Up;        break;
		case KEY_DOWN:      c= Down;      break;
		case KEY_PPAGE:     c= PageUp;    break;
		case KEY_NPAGE:     c= PageDown;  break;
		case KEY_BACKSPACE: c= Backspace; break;
		case KEY_DC:        c= Delete;    break;
		case KEY_HOME:      c= Home;      break;
		case KEY_FIND:      c= Home;      break;
		case KEY_END:       c= End;       break;
		case KEY_SELECT:    c= Select;    break;
		// Need this assign of KEY_NEXT when TERM is vt320
		case KEY_NEXT:      c= '\t';      break;
		case KEY_F(1):  c= F1;  break;
		case KEY_F(2):  c= F2;  break;
		case KEY_F(3):  c= F3;  break;
		case KEY_F(4):  c= F4;  break;
		case KEY_F(5):  c= F5;  break;
		case KEY_F(6):  c= F6;  break;
		case KEY_F(7):  c= F7;  break;
		case KEY_F(8):  c= F8;  break;
		case KEY_F(9):  c= F9;  break;
		case KEY_F(10): c= F10; break;
		case KEY_F(11): c= F11; break;
		case KEY_F(12): c= F12; break;
		default: c= 0;
		}
	return c;
}

int termbuf::checkkey ()
{
	nodelay ( (WINDOW *) w, TRUE);
	int c= getkey ();
	nodelay ( (WINDOW *) w, FALSE);
	return c;
}

int termbuf::sync ()
{
	//TRF;

	std::streamsize n= pptr () - pbase ();
	for (std::streamsize i= 0; i < n; ++i)
	{
		unsigned char c
			(* reinterpret_cast <unsigned char *> (pbase () + i) );

		//TRDEBS (std::hex << static_cast <int> (c) );

		waddch ( (WINDOW *) w, c);
	}
	wrefresh ( (WINDOW *) w);
	pbump (-n);
	gbump (egptr () - gptr () );
	return 0;
}

int termbuf::overflow (int ch)
{
	sync ();
	if (ch != EOF)
	{
		unsigned char c (static_cast <unsigned char>
			(static_cast <unsigned int> (ch) ) );
		waddch ( (WINDOW *) w, c);
		wrefresh ( (WINDOW *) w);
	}
	return 0;
}

int termbuf::underflow ()
{
	return 0;
}


//**************************************************************


termstreambase::termstreambase (std::ios::openmode which) :
	__my_tb (which)
{
	init (&__my_tb);
}

termbuf * termstreambase::rdbuf ()
{
	return & __my_tb;
}

void termstreambase::sync ()
{
	__my_tb.sync ();
}

int termstreambase::getkey ()
{
	return __my_tb.getkey ();
}

int termstreambase::checkkey ()
{
	return __my_tb.checkkey ();
}

void termstreambase::shellmode ()
{
	reset_shell_mode ();
}

void termstreambase::progmode ()
{
	reset_prog_mode ();
}

void termstreambase::clr_all ()
{
	wclear (stdscr);
	wrefresh (stdscr);
}

void termstreambase::redraw_all ()
{
	redrawwin (stdscr);
	doupdate ();
}

void termstreambase::setWindow (void *window)
{
	w= window;
	__my_tb.setWindow (w);
	keypad ( (WINDOW *) window, TRUE);
}

int termstreambase::lines () const
{
	int r, nousado;
	getmaxyx ( (WINDOW *) w, r, nousado);
	return r;
}

int termstreambase::columns () const
{
	int r, nousado;
	getmaxyx ( (WINDOW *) w, nousado, r);
	return r;
}

int termstreambase::wherex () const
{
	int x, y;
	const_cast <termstreambase *> (this)->sync ();
	getyx ( (WINDOW *) w, y, x);
	return x;
}

int termstreambase::wherey () const
{
	int x, y;
	const_cast <termstreambase *> (this)->sync ();
	getyx ( (WINDOW *) w, y, x);
	return y;
}


//**************************************************************


otermstream::otermstream (std::ios::openmode which) :
	termstreambase (which),
	std::ostream (& __my_tb),
	isStd (true)
{
	setWindow (stdscr);
}

otermstream::otermstream (int nlines, int ncols, int y, int x) :
	termstreambase (std::ios::out),
	std::ostream (& __my_tb),
	isStd (false)
{
	TRF;

	WINDOW * sw= newwin (nlines, ncols, y, x);
	if (! sw)
		throw "Al diablo";
	setWindow (sw);
}

otermstream::otermstream (otermstream & ot, int nlines, int ncols,
	int y, int x) :
	termstreambase (std::ios::out),
	std::ostream (& __my_tb),
	isStd (false)
{
	TRF;

	//WINDOW * sw= subwin ( (WINDOW *) ot.w, nlines, ncols, y, x);
	WINDOW * sw= derwin ( (WINDOW *) ot.w, nlines, ncols, y, x);
	if (! sw)
		throw "Al diablo";
	setWindow (sw);
}

otermstream::~otermstream ()
{
	TRF;

	if (! isStd)
		delwin ( (WINDOW *) w);
}

void otermstream::update ()
{
	//wnoutrefresh ( (WINDOW *) w);
	//doupdate ();
	wrefresh ( (WINDOW *) w);
}

void otermstream::border ()
{
	wborder ( (WINDOW *) w, 0, 0, 0, 0, 0, 0, 0, 0);
}

void otermstream::clrscr ()
{
	sync ();
	wclear ( (WINDOW *) w);
	wrefresh ( (WINDOW *) w);
}

void otermstream::clreol ()
{
	sync ();
	wclrtoeol ( (WINDOW *) w);
	wrefresh ( (WINDOW *) w);
}

void otermstream::insertline ()
{
	sync ();
	winsertln ( (WINDOW *) w);
}

void otermstream::deleteline ()
{
	sync ();
	wdeleteln ( (WINDOW *) w);
}

void otermstream::at (int y, int x)
{
	sync ();
	wmove ( (WINDOW *) w, y, x);
}

void otermstream::inverse (bool flag)
{
	sync ();
	if (flag) wattron ( (WINDOW *) w, A_REVERSE);
	else wattroff ( (WINDOW *) w, A_REVERSE);
}

void otermstream::delay (bool flag)
{
	nodelay ( (WINDOW *) w, flag ? false : true);
}

void otermstream::cursor (bool flag)
{
	curs_set (flag ? 1 : 0);
}

int otermstream::getBegX () const
{
	WINDOW * win= (WINDOW *) w;
	int y, x;
	getbegyx (win, y, x);
	return x;
}

int otermstream::getBegY () const
{
	WINDOW * win= (WINDOW *) w;
	int y, x;
	getbegyx (win, y, x);
	return y;
}

int otermstream::getWidth () const
{
	WINDOW * win= (WINDOW *) w;
	int y, x;
	getmaxyx (win, y, x);
	return x;
}

int otermstream::getHeight () const
{
	WINDOW * win= (WINDOW *) w;
	int y, x;
	getmaxyx (win, y, x);
	return y;
}

void otermstream::setcolor (short n, Color::value f, Color::value b)
{
	init_pair (n, color_curses (f), color_curses (b) );
}

void otermstream::color (short c)
{
	sync ();
	wattrset ( (WINDOW *) w, COLOR_PAIR (c) );
}

void otermstream::background (short c)
{
	sync ();
	wbkgd ( (WINDOW *) w, COLOR_PAIR (c) );
}


//**************************************************************


std::ostream & operator << (std::ostream &os, const ts_bool & tb)
{
	return tb.f (os, tb.b);
}

std::ostream & operator << (std::ostream &os, const ts_2int & t2i)
{
	return t2i.f (os, t2i.i1, t2i.i2);
}

std::ostream & operator << (std::ostream & os, const ts_short & ts)
{
	return ts.f (os, ts.s);
}


//**************************************************************


std::ostream & inverse (std::ostream &os, bool b)
{
	otermstream & o= dynamic_cast<otermstream &> (os);
	o.inverse (b);
	return os;
}

std::ostream & delay (std::ostream &os, bool b)
{
	otermstream & o= dynamic_cast<otermstream &> (os);
	o.delay (b);
	return os;
}

std::ostream & cursor (std::ostream &os, bool b)
{
	otermstream & o= dynamic_cast<otermstream &> (os);
	o.cursor (b);
	return os;
}

std::ostream & gotoxy (std::ostream &os, int x, int y)
{
	otermstream & o= dynamic_cast<otermstream &> (os);
	o.gotoxy (x, y);
	return os;
}

std::ostream & at (std::ostream & os, int y, int x)
{
	otermstream & o= dynamic_cast<otermstream &> (os);
	o.at (y, x);
	return os;
}

std::ostream & color (std::ostream &os, short c)
{
	otermstream & o= dynamic_cast<otermstream &> (os);
	o.color (c);
	return os;
}

ts_bool inverse (bool b)
{
	return ts_bool (inverse, b);
}

ts_bool delay (bool b)
{
	return ts_bool (delay, b);
}

ts_bool cursor (bool b)
{
	return ts_bool (cursor, b);
}

ts_2int gotoxy (int x, int y)
{
	return ts_2int (gotoxy, x, y);
}

ts_2int at (int y, int x)
{
	return ts_2int (at, y, x);
}

ts_short color (short c)
{
	return ts_short (color, c);
}

std::ostream & update (std::ostream & os)
{
	otermstream & o= dynamic_cast<otermstream &> (os);
	o.update ();
	return os;
}

std::ostream & clrscr (std::ostream &os)
{
	otermstream & o= dynamic_cast<otermstream &> (os);
	o.clrscr ();
	return os;
}

std::ostream & clreol (std::ostream &os)
{
	otermstream & o = dynamic_cast<otermstream &> (os);
	o.clreol ();
	return os;
}

std::ostream & delline (std::ostream &os)
{
	otermstream & o= dynamic_cast<otermstream &> (os);
	o.deleteline ();
	return os;
}

std::ostream & insline (std::ostream &os)
{
	otermstream & o= dynamic_cast<otermstream &> (os);
	o.insertline ();
	return os;
}

// End of termstream.cpp

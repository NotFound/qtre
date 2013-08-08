// dialog.cpp
// Revision 6-feb-2007

#include "dialog.h"

#include "trace.h"
#undef ASSERT
#include "qtreassert.h"

#include <vector>
#include <deque>
#include <sstream>

//#include <unistd.h> // Debug only, sleep.

void centertext (otermstream & ot, int lin, int width, std::string str)
{
	using std::string;
	int pos;
	string::size_type l= str.size ();
	const string::size_type max= width;
	if (l < max)
	{
		pos= (max - l) / 2;
	}
	else
	{
		pos= 0;
		str.erase (max);
	}
	ot << at (lin, pos) << str;
}


namespace {

struct Dims {
	size_t h, w, y, x;
};

#if 0
std::deque <std::string> history;

size_t getmaxhistory ()
{
	// Provisional
	return 10;
}
#endif

class GetString {
public:
	GetString (otermstream & ots,
		int & keyend, const std::set <int> & keyender,
		const std::string & ntitle, const std::string & nprompt,
		std::string & str, History & history,
		size_t width, std::string::size_type nmaxlen);
	bool interact ();
private:
	typedef std::string::size_type size_type;
	int & keyend;
	const std::set <int> & keyender;
	const std::string & title;
	const std::string & prompt;
	std::string & str;
	const Dims d;
	otermstream ts;
	const size_t postitle;
	const size_type maxlen;
	static const size_t xini= 2, yini= 3;
	const size_type w;
	size_type inipos;
	size_type pos;
	History & history;
	size_t histsize;
	size_t histpos;
	std::string savestr;

	void sendkey (int key);
	Dims calcdims (otermstream & ots, size_t width);
	size_type calcinipos ();
	void putfrom (std::string::size_type n);
	void clearrest ();
	void showagain ();
	void up ();
	void down ();
};

GetString::GetString (otermstream & ots,
		int & keyend, const std::set <int> & keyender,
		const std::string & ntitle, const std::string & nprompt,
		std::string & nstr, History & history,
		size_t width, std::string::size_type nmaxlen) :
	keyend (keyend),
	keyender (keyender),
	title (ntitle),
	prompt (nprompt),
	str (nstr),
	// Testing: test to avoid scrolling in short fixed length.
	//d (calcdims (ots, width) ),
	d (calcdims (ots, width + 1) ),
	ts (ots, d.h, d.w, d.y, d.x),
	postitle ( (d.w - 2 - title.size () ) / 2 + 1),
	maxlen (nmaxlen),
	w (d.w - 4),
	inipos (calcinipos () ),
	history (history),
	histsize (history.size () ),
	histpos (histsize)
{
	pos= str.size ();
}

GetString::size_type GetString::calcinipos ()
{
	size_type l= str.size ();
	return l < w ? 0 : ( (l -w + 7) / 8) * 8;
}

void GetString::putfrom (std::string::size_type n)
{
	ASSERT (n >= inipos);
	size_type skip= n - inipos;
	ts << at (yini, xini + skip) << str.substr (n, w - skip);
	//ts << at (yini, xini + skip) << str.substr (n, n + w);
}

void GetString::clearrest ()
{
	TRF;

	size_type l= str.size ();
	TRDEBS (l << ' ' << inipos << ' ' << w);
	if (l - inipos >= w)
		return;
	size_type r= w - (l - inipos);
	ts << std::string (r, ' ');
}

void GetString::showagain ()
{
	TRF;

	//pos= 0;
	//inipos= 0;
	const size_type l= str.size ();
	if (l > maxlen)
		str.erase (maxlen);
	pos= str.size ();
	for (inipos= 0; pos - inipos >= w; inipos+= 8)
		continue;

	TRDEBS ("Pos: " << pos << ", w: " << w << ", inipos: " << inipos);

	ts.flush ();
	ts << at (yini, xini) << std::string (w, ' ');
	ts.flush ();
	//sleep (2);
	putfrom (inipos);
	//clearrest ();
}

void GetString::up ()
{
	if (histpos > 0)
	{
		if (histpos == histsize)
			savestr= str;
		--histpos;
		str= history [histpos];
		showagain ();
	}
}

void GetString::down ()
{
	if (histpos < histsize)
	{
		++histpos;
		if (histpos < histsize)
			str= history [histpos];
		else
			str= savestr;
		showagain ();
	}
}

bool GetString::interact ()
{
	TRF;

	ts << clrscr;
	ts.border ();
	ts << at (0, postitle) << title;
	ts << at (2, 2) << prompt;
	putfrom (inipos);
	for (;;)
	{
		ASSERT (pos >= inipos);
		ts << at (yini, xini + pos - inipos);
		int key;
		switch (key= ts.getkey () )
		{
		case key::Esc:
		case key::F8:
			keyend= key;
			return false;
		case '\r': case '\n':
			{
				#if 0
				size_t maxhist= getmaxhistory ();
				if (maxhist == 0)
					maxhist= 1;
				if (histsize >= maxhist)
				{
					history.erase (history.begin (),
						history.begin () + histsize -
						maxhist + 1);
				}
				if (history.empty () || 
						str != history.back () )
					history.push_back (str);
				#else
				history.push_back (str);
				#endif
			}
			keyend= '\n';
			return true;
		default:
			if (keyender.find (key) != keyender.end () )
			{
				keyend= key;
				return false;
			}
			sendkey (key);
			;
		}
	}
}

void GetString::sendkey (int key)
{
	using namespace key;

	switch (key)
	{
	case Home:
		pos= 0;
		if (inipos > 0)
		{
			inipos= 0;
			putfrom (0);
		}
		break;
	case End:
		pos= str.size ();
		{
			size_type newinipos= calcinipos ();
			if (inipos < newinipos)
			{
				inipos= newinipos;
				putfrom (inipos);
				clearrest ();
			}
		}
		break;
	case Left:
		if (pos > 0)
		{
			--pos;
			if (pos < inipos)
			{
				ASSERT (inipos >= 8);
				inipos-= 8;
				putfrom (inipos);
			}
		}
		break;
	case Right:
		if (pos < str.size () )
		{
			++pos;
			if (pos - inipos >= w)
			{
				inipos+= 8;
				putfrom (inipos);
				clearrest ();
			}
		}
		break;
	case Up:
		up ();
		break;
	case Down:
		down ();
		break;
	case Backspace:
	case Ctl_H:
		if (pos > 0)
		{
			--pos;
			str.erase (pos, 1);
			if (pos < inipos)
			{
				ASSERT (inipos >= 8);
				inipos-= 8;
				putfrom (inipos);
			}
			else
			{
				putfrom (pos);
				ts << ' ';
			}
		}
		break;
	case Ctl_Y:
		ts << at (yini, xini) << std::string (w, ' ');
		str.erase ();
		pos= 0;
		inipos= 0;
		break;
	case Delete:
		if (pos < str.size () )
		{
			str.erase (pos, 1);
			putfrom (pos);
			ts << ' ';
		}
		break;
	default:
		if (key <= 0xff && str.size () < maxlen)
		{
			str.insert (pos, 1, key);
			++pos;
			if (pos - inipos >= w)
			{
				inipos+= 8;
				putfrom (inipos);
				clearrest ();
			}
			else
				putfrom (pos - 1);
		}
	} // switch
}

Dims GetString::calcdims (otermstream & ots, size_t width)
{
	TRF;

	size_t w= title.size ();
	size_t w2= prompt.size ();
	w= std::max (w, w2);
	w= std::max (w, width);
	w+= 4;
	w2= ots.getWidth ();
	w= std::min (w, w2);
	Dims d;
	const size_t Height= 6;
	d.h= Height;
	d.w= w;
	d.x= (w2 - w) / 2;
	d.y= (ots.getHeight () - Height) / 2;
	return d;
}

} // namespace

bool dialogGetString (otermstream &ots,
	int & keyend, const std::set <int> & keyender,
	const std::string & title, const std::string & prompt,
	std::string & str, History & history,
	size_t width, std::string::size_type maxlen)
{
	TRF;

	std::string strnue= str;
	//std::string strnue;
	GetString gs (ots, keyend, keyender, title, prompt, 
		strnue, history, width, maxlen);
	if (gs.interact () )
	{
		str= strnue;
		return true;
	}
	else return false;
}

bool dialogGetString (otermstream &ots,
	const std::string & title, const std::string & prompt,
	std::string & str, History & history,
	size_t width, std::string::size_type maxlength)
{
	int key;
	return dialogGetString (ots, key, std::set <int> (),
		title, prompt, str, history, width, maxlength);
}

namespace {

using std::string;
using std::vector;

class ShowText {
public:
	ShowText (otermstream & otsn, const char * title, const char * str,
			int height, int width) :
		strTitle (title),
		d (calcdims (otsn, height, width, text) ),
		w (d.w - 3),
		text (parsetext (otsn, str, w) ),
		inilin (0),
		max (d.h - 2),
		scroll (max - 1),
		postitle ( (w - strTitle.size () ) / 2 + 1),
		ots (otsn, d.h, d.w, d.y, d.x)
	{
	}
	void show ();
private:
	string strTitle;
	typedef vector <string> Vtext;
	Dims d;
	const size_t w;
	Vtext text;
	size_t inilin;
	const size_t max, scroll;
	const size_t postitle;
	otermstream ots;
	// To clear a empty line, space does not work always.
	static const char cWhite= '\xA0';
	static string strtab;

	void putlines ();
	static string nextword (const char * & paux);
	static void addline (Vtext & text, string str, size_t w);
	Vtext parsetext (otermstream & otsn, const char * str, int w);
	static Dims calcdims (otermstream & ot, int height, int width,
		const Vtext & text);
};

string ShowText::strtab (8, ' ');

void ShowText::show ()
{
	TRF;

	using namespace key;

	ots.cursor (false);
	ots << clrscr;
	ots.border ();
	ots << at (0, postitle) << strTitle;
	putlines ();
	int key;
	while ( (key= ots.getkey () ) != Esc && key != F8)
	{
		size_t oldini= inilin;
		switch (key)
		{
		case Down:
			if (inilin < text.size () - 1)
				++inilin;
			break;
		case Up:
			if (inilin > 0)
				--inilin;
			break;
		case PageUp:
			if (inilin > max)
				inilin-= scroll;
			else
				inilin= 0;
			break;
		case PageDown:
			inilin+= scroll;
			if (inilin >= text.size () )
				inilin= text.size () - 1;
			break;
		} // switch
		if (inilin != oldini)
			putlines ();
	}
	
	ots.cursor (true);
}

void ShowText::putlines ()
{
	TRF;

	for (size_t i= 0; i < max; ++i)
	{
		size_t l= i + inilin;
		ots << at (i + 1, 2);
		if (l < text.size () )
		{
			string & str= text [l];
			if (! str.empty () )
				ots << str;
			else
				ots << string (w, cWhite);
		}
		else
			ots << string (w, cWhite);
		ots << std::flush;
	}
}

string ShowText::nextword (const char * & aux)
{
	TRF;

	string str;
	char c;
	while ( (c= * aux) != ' ' && c != '\t' && c != '\n' && c != '\0')
	{
		str+= c;
		++aux;
	}
	if (c != '\0' && c != '\n') ++aux;
	return str;
}

void ShowText::addline (Vtext & text, string str, size_t w)
{
	TRFDEBS ('"' << str << "\" w= " << w);

	string::size_type len= str.size ();
	if (len < w)
		str+= string (w - len, ' ');
	ASSERT (str.size () == w);
	text.push_back (str);
}

inline bool fit (const string & str, const string & word, size_t w)
{
	return str.size () + 1 + word.size () < w;
}

ShowText::Vtext ShowText::parsetext (otermstream & otsn,
	const char * str, int w)
{
	TRFDEB (str);

	Vtext text;
	string strAux;

	string word;
	char c;
	const size_t ww= static_cast <size_t> (w);
	while ( (c= * str) == ' ' || c == '\t')
	{
		if (c == '\t')
			strAux+= strtab;
		else
			strAux+= ' ';
		++str;
	}
	while (* str)
	{
		word= nextword (str);
		TRDEB (word);
		if (strAux.empty () || fit (strAux, word, w) )
		{
			if (strAux.empty () )
			{
				// Is possible that the first word not fit
				// into a line.
				while (word.size () >= ww)
				{
					addline (text,
						word.substr (0, w - 1), w);
					word.erase (0, w - 1);
				}
			}
			else 
				if (strAux [strAux.size () - 1] != ' ' )
					strAux+= ' ';
			strAux+= word;
		}
		else
		{
			addline (text, strAux, w);
			TRDEB (string ("'") + text.back () + '\'');
			while (word.size () >= ww)
			{
				addline (text, word.substr (0, w - 1), w);
				word.erase (0, w - 1);
			}
			strAux= word;
		}
		if (* str == '\n')
		{
			++str;
			addline (text, strAux, w);
			TRDEB ( string ("'") + text.back () + '\'');
			strAux.erase ();
			while ( (c= * str) == ' ' || c == '\t')
			{
				if (c == '\t')
					strAux+= strtab;
				else
					strAux+= ' ';
				++str;
			}
		}
	}
	if (! strAux.empty () )
	{
		addline (text, strAux, w);
		TRDEB (string ("'") + text.back () + '\'');
	}
	size_t htext= text.size ();
	if (htext < d.h)
	{
		d.h= htext + 2;
		d.y= (otsn.getHeight () - d.h) / 2;
	}
	return text;
}

Dims ShowText::calcdims (otermstream & ot, int height, int width,
	const Vtext & /*text*/)
{
	Dims d;
	size_t otw= ot.getWidth ();
	if (width == 0)
		d.w= otw / 2;
	else
		d.w= width;
	d.x= (otw - d.w) / 2;

	size_t oth= ot.getHeight ();
	d.h= height == 0 ? oth / 2 : height;
	if (d.h > oth)
		d.h= oth;
	d.y= (oth - d.h) / 2;
	return d;
}

} // namespace

void dialogShowText (otermstream & ots, const char * title, const char *str,
	int height, int width)
{
	TRFDEBS ("Title: \"" << title << "\" str= \"" << str <<
			"\" height= " << height << " width= " << width);

	ShowText st (ots, title, str, height, width);
	st.show ();
}

dialogResult dialogQuestion (otermstream & ots, const std::string & str)
{
	TRF;

	size_t l= str.size () + 4;
	size_t w= ots.getWidth () - 6;
	if (l > w)
		l= w;
	otermstream otq (ots, 6, l, 10, 15);
	otq.clrscr ();
	otq.border ();
	otq.cursor (false);

	otq << at (2, 2) <<
		str << at (4, 6) << "Y/N/esc";

	dialogResult result= dialogCancel;
	for (bool sigue= true; sigue;)
	{
		switch (otq.getkey () )
		{
		case 'Y': case 'y':
			sigue= false;
			result= dialogYes;
			break;
		case 'N': case 'n':
			sigue= false;
			result= dialogNo;
			break;
		case key::F8:
		case key::Esc:
			sigue= false;
			break;
		}
	}
	otq.cursor (true);
	return result;
}

// *************************************************************
// *              class DialogOptions::Internal                *
// *************************************************************

class DialogOptions::Internal {
public:
	Internal (otermstream & ots, const std::string & title);
	~Internal ();
	void add (const std::string & description, bool & option);
	bool do_it ();
private:
	otermstream & ots;
	const std::string title;
	std::vector <std::string> desc;
	std::vector <bool *> opt;
	size_t nopts;
	otermstream * pots;
	size_t markpos;

	size_t posoption (size_t i) const { return i * 2 + 2; }
	void putmark (size_t i) const;
};

DialogOptions::Internal::Internal
		(otermstream & ots, const std::string & title) :
	ots (ots),
	title (title),
	nopts (0),
	pots (NULL)
{
}

DialogOptions::Internal::~Internal ()
{
	delete pots;
}

void DialogOptions::Internal::add (const std::string & description, bool & option)
{
	desc.push_back (description);
	opt.push_back (& option);
	++nopts;
}

void DialogOptions::Internal::putmark (size_t i) const
{
	* pots << at (posoption (i), markpos) << (* opt [i] ? 'X' : ' ');
}

bool DialogOptions::Internal::do_it ()
{
	using namespace key;

	size_t w= title.size () + 2;
	size_t maxl= desc [0].size ();
	for (size_t i= 1; i < nopts; ++i)
		if (desc [i].size () > maxl)
			maxl= desc [i].size ();
	w= std::max (w, maxl + 8);
	size_t h= desc.size () * 2 + 3;
	markpos= w - 4;

	size_t parentw= ots.getWidth ();
	size_t parenth= ots.getHeight ();
	if (parentw < w || parenth < h)
		return false;
	size_t x= (parentw - w) / 2;
	size_t y= (parenth - h) / 2;
	

	pots= new otermstream (ots, h, w, y, x);
	* pots << clrscr;
	pots->border ();
	std::string t= title.substr (0, w - 2);
	* pots << at (0, (w - t.size () ) / 2) << t;

	for (size_t i= 0; i < nopts; ++i)
	{
		* pots << at (posoption (i), 2) << desc [i] <<
			at (posoption (i), markpos - 1) << "[ ]";
	}
	for (size_t i= 0; i < nopts; ++i)
		putmark (i);

	size_t current= 0;
	bool confirm= false;
	for (bool looping= true; looping; )
	{
		* pots << at (posoption (current), markpos);
		switch (pots->getkey () )
		{
		case '\n': case '\r':
			confirm= true;
			looping= false;
			break;
		case Esc:
		case F8:
			looping= false;
			break;
		case Up:
			if (current > 0)
				--current;
			break;
		case Down:
			if (current < nopts - 1)
				++current;
			break;
		case Home: case PageUp:
			current= 0;
			break;
		case End: case PageDown:
			current= nopts - 1;
			break;
		case ' ':
			* opt [current]= ! * opt [current];
			putmark (current);
			break;
		}
	}
	return confirm;
}

// *************************************************************
// *                    class DialogOptions                    *
// *************************************************************

DialogOptions::DialogOptions (otermstream & ots, const std::string & title) :
	pin (new Internal (ots, title) )
{
}

DialogOptions::~DialogOptions ()
{
	delete pin;
}

void DialogOptions::add (const std::string & description, bool & option)
{
	pin->add (description, option);
}

bool DialogOptions::do_it ()
{
	return pin->do_it ();
}

// Fin de dialog.cpp

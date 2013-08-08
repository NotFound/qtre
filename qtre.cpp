// qtre.cpp
// Revision 29-jan-2006

#include "termstream.h"
#include "screen.h"

#include "trace.h"
#undef ASSERT
#include "qtreassert.h"

#include "config_locale.h"

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include <algorithm>

#include <signal.h>

using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::string;
using std::vector;
using std::exception;
using std::runtime_error;
using std::auto_ptr;

extern const string strVersion;
extern const string strAbout1;
extern const string strAbout2;
extern const string strAbout3;

// Used to end the program when received a SIGTERM signal.
bool fExit= false;

namespace {

//*********************************************************
//		Initialization.
//*********************************************************

class FileLoader {
public:
	FileLoader (Screen & screen);
	~FileLoader ();
	size_t size () const;
	void load (const string & filename);
	void load (FILE * fin);
private:
	size_t numfiles;
	Screen & screen;
};

FileLoader::FileLoader (Screen & screen) :
	numfiles (0),
	screen (screen)
{
	TRF;
}

FileLoader::~FileLoader ()
{
	TRF;
}

size_t FileLoader::size () const
{
	return numfiles;
}

void FileLoader::load (const string & filename)
{
	TRFDEBS ("Loading: " << filename);

	if (numfiles > 0)
		screen.windownew ();
	screen.loadtext (filename);
	++numfiles;
}

void FileLoader::load (FILE * fin)
{
	TRF;

	if (numfiles > 0)
		screen.windownew ();
	screen.loadtext (fin);
	++numfiles;
}


class Options {
public:
	Options (int argc, char * * argv);
	~Options ();
	bool use_stderr () const;
	bool has_line () const;
	size_t line () const;
	size_t numfiles () const;
	string filename (size_t n) const;
private:
	bool stdin_used;
	bool flagline;
	size_t numline;
	vector <string> files;
};

Options::Options (int argc, char * * argv) :
	stdin_used (false),
	flagline (false),
	numline (0)
{
	TRF;

	bool allowoptions= true;
	for (int i= 1; i < argc; ++i)
	{
		string param (argv [i] );
		if (allowoptions)
		{
			if (param == "--")
			{
				allowoptions= false;
				continue;
			}
			if (param.substr (0, 1) == "+")
			{
				const char * aux= param.c_str () + 1;
				char * auxres;
				numline= strtoul (aux, & auxres, 10);
				if (* auxres != '\0')
					throw runtime_error ("Invalid option");
				flagline= true;
				continue;
			}
		}
		if (param == "-")
			stdin_used= true;
		files.push_back (param);
	}

}

Options::~Options ()
{
	TRF;
}

bool Options::use_stderr () const
{
	return stdin_used;
}

bool Options::has_line () const
{
	return flagline;
}

size_t Options::line () const
{
	return numline;
}

size_t Options::numfiles () const
{
	return files.size ();
}

string Options::filename (size_t n) const
{
	ASSERT (n < files.size () );
	return files [n];
}

int qtre_main (int argc, char * * argv, char * * env)
{
	TRF;

	bool useutf8= false;

	// Use locale and langinfo, if available, to check utf8 use.
	#if HAVE_LOCALE_H
		setlocale (LC_ALL, "");
		#if HAVE_LANGINFO_H
			const std::string codeset (nl_langinfo (CODESET) );
			useutf8= codeset == "UTF-8";
		#endif
	#endif

	TRDEBS (codeset << " is utf8: " << (useutf8 ? "Yes" : "No") );

	Options options (argc, argv);

	if (options.use_stderr () )
		termstream_set_terminal (stderr);

	// Screen is the main object of the application.

	auto_ptr <Screen> pscreen (createScreen (useutf8, env) );
	Screen & screen= * pscreen.get ();

	FileLoader fl (screen);
	const size_t numfiles= options.numfiles ();
	for (size_t i= 0; i < numfiles; ++i)
	{
		const string filename= options.filename (i);
		if (filename == "-")
		{
			fl.load (stdin);
		}
		else
			fl.load (filename);
	}

	if (numfiles > 0)
		screen.changewindow (0);

	if (options.has_line () )
	{
		const size_t optline= options.line ();
		const size_t maxline= screen.textlines ();
		const size_t numline= optline > 0 ?
			std::min (optline, maxline) :
			maxline;
		screen.gotoline (numline - 1);
	}

	screen.interact ();

	return 0;
}

void handleSigTerm (int)
{
	fExit= true;
}

void init_signals ()
{
	TRF;

	#ifndef NDEBUG
	signal (SIGUSR1, TraceFunc::show);
	#endif

	#if 0
	struct sigaction action;
	action.sa_handler= handleSigTerm;
	action.sa_flags= SA_RESTART;
	sigaction (SIGTERM, & action, NULL);
	#endif
}

} // namespace

int main (int argc, char * * argv, char * * env)
{
	TRF;

	init_signals ();

	int r= 0;
	try
	{
		r= qtre_main (argc, argv, env);
	}
	catch (exception &e)
	{
		cout << endl << "Error: " << e.what ();
		r= 127;
	}
	catch (...)
	{
		cout << endl << "Unexpected error";
		r= 127;
	}

	cout << endl;
	return r;
} // main

// End of qtre.cpp

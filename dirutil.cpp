// dirutil.cpp
// Revision 29-jul-2009

#include "dirutil.h"

#include "termstream.h"

#include "trace.h"

#include "config_types.h"
#include "config_dirent.h"
#include "config_fork.h"
#include "config_wait.h"


#include <stdexcept>

#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>

using std::string;
using std::runtime_error;

namespace {

class AutoCString {
public:
	AutoCString (const char * str) : str (str)
	{ }
	~AutoCString ()
	{
		free (const_cast <char *> (str) );
	}
private:
	const char * str;
};

class AutoDir {
public:
	AutoDir (DIR * dir) : dir (dir)
	{ }
	~AutoDir ();
private:
	DIR * dir;
};

AutoDir::~AutoDir ()
{
	TRF;

	#if CLOSEDIR_VOID
	closedir (dir);
	#else
	int r= closedir (dir);
	if (r != 0)
	{
		TRDEB ("Failed closedir");
	}
	#endif
}


bool has_leading_slash (const std::string & str)
{
	if (str.empty () )
		return false;
	return str [str.size () - 1] == '/';
}

} // namespace

bool dirutil::is_directory (const std::string & str)
{
	struct stat st;
	if (stat (str.c_str (), & st) != 0)
		return false;
	if (! S_ISDIR (st.st_mode) )
		return false;
	return true;
}

std::string dirutil::base_name (const std::string & str)
{
	// basename requires a writable c string.
	char * aux= strdup (str.c_str () );
	if (! aux)
		throw std::bad_alloc ();
	AutoCString aaux (aux);
	return basename (aux);
}

std::string dirutil::dir_name (const std::string & str)
{
	// basename requires a writable c string.
	char * aux= strdup (str.c_str () );
	if (! aux)
		throw std::bad_alloc ();
	AutoCString aaux (aux);
	return dirname (aux);
}

std::string dirutil::extension (const std::string & str)
{
	//TraceFunc tr ("file::extension");

	const string file= base_name (str);
	//tr.message (file);
	string::size_type l= file.rfind ('.');
	if (l == string::npos)
		return string ();
	if (l == file.size () )
		return string ();
	else return file.substr (l + 1);	
}

std::string dirutil::enter_dir (const std::string & old,
	const std::string & enter)
{
	if (enter == ".")
		return old;
	if (enter == "..")
	{
		string base= base_name (old);
		if (base == "/")
			return old;
		if (base != ".." && base != ".")
			return dir_name (old);
	}
	return old + (has_leading_slash (old) ? "" : "/") + enter;
}

void dirutil::directory (const std::string & dirname,
	std::vector <std::string> & v)
{
	v.clear ();
	DIR * dir= opendir (dirname.c_str () );
	if (! dir)
		return;
	AutoDir adir (dir);
	struct dirent * ent;
	while ( (ent= readdir (dir) ) )
		v.push_back (ent->d_name);
}

void dirutil::delete_file (const std::string & filename)
{
	if (unlink (filename.c_str () ) != 0)
		throw runtime_error ("Error deleting " + filename +
			": " + strerror (errno) );
}

std::string dirutil::create_temp_file ()
{
	TRF;

	char buffer [PATH_MAX];

	// Generate temporary file template.

	const char * tmp_path= getenv ("TMP");
	if (tmp_path != NULL)
	{
		strncpy (buffer, tmp_path, PATH_MAX - 2);
		buffer [sizeof (buffer) - 2]= '\0';
		size_t l= strlen (buffer);
		if (l > 0 && buffer [l - 1] != '/')
		{
			buffer [l]= '/';
			buffer [l + 1]= '\0';
		}
	}
	else
		strcpy (buffer, "/tmp/");
	strcat (buffer, "qtre_XXXXXX");

	// Create temporary file with the template.

	int r= mkstemp (buffer);
	if (r == -1)
	{
		throw runtime_error
			(string ("Error creating temporay file: ") +
				strerror (errno) );
	}
	close (r);
	return string (buffer);
}

namespace {

class EnterShellMode {
public:
	EnterShellMode ()
	{
		otermstream::clr_all ();
		otermstream::shellmode ();
	}
	~EnterShellMode ()
	{
		otermstream::progmode ();
	}
};

int do_copy_remote_file (const std::string & source, const std::string & dest)
{
	TRF;

	//int r= system ( ("scp " + source + " " + dest).c_str () );

	int r= -1;

	pid_t pid= vfork ();
	switch (pid)
	{
	case pid_t (-1):
		// Failed fork.
		r= -1;
		break;
	case pid_t (0):
		// Child process.
		execlp ("scp", "scp",
			source.c_str (), dest.c_str (),
			static_cast <char *> (0) );
		TRDEB (	string ("execlp failed: ") + strerror (errno) );
		exit (127);
	default:
		// Parent process.
		{
			int status;
			if (waitpid (pid, & status, 0) != pid)
				r= -1;
			else
			{
				if (WIFEXITED (status) )
					r= WEXITSTATUS (status);
				else
					r= -1;
			}
		}
	}

	return r;
}

} // namespace

void dirutil::copy_remote_file
	(const std::string & source, const std::string & dest)
{
	TRF;

	EnterShellMode esm;

	std::cerr << "Remote copying..." << std::endl;

	//int r= system ( ("scp " + source + " " + dest).c_str () );
	int r= do_copy_remote_file (source, dest);

	if (r != 0)
		throw runtime_error
			(string ("Error copying remote file: ") +
				strerror (errno) );
}

// End of dirutil.cpp

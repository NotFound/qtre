// exec.cpp
// Revision 29-jan-2006

#include "exec.h"

#include "trace.h"

#include "config_types.h"
#include "config_fork.h"
#include "config_wait.h"


#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>


//***********************************************
//		Exec::Impl
//***********************************************

class Exec::Impl
{
public:
	Impl (Exec & exec);
	~Impl ();
	void Command (const std::string & strCommand);
private:
	Impl (const Impl &);            // Forbidden
	void operator = (const Impl &); // Forbidden

	void killchild ();
	bool createchild (const std::string & strCommand);
	void readfromchild ();

	Exec & exec;
	int hchild;
	pid_t pidchild;

	static const int h_none= -1;
	static const pid_t pid_none= static_cast <pid_t> (-1); 
};

Exec::Impl::Impl (Exec & exec) :
	exec (exec),
	hchild (h_none),
	pidchild (pid_none)
{
}

Exec::Impl::~Impl ()
{
	killchild ();
}

void Exec::Impl::killchild ()
{
	if (pidchild != pid_none)
	{
		kill (pidchild, SIGTERM);
		pidchild= pid_none;
	}
	if (hchild != h_none)
	{
		close (hchild);
		hchild= h_none;
	}
}

bool Exec::Impl::createchild (const std::string & strCommand)
{
	TRF;

	// If any previous child exist, kill it.
	killchild ();

	int handle [2];
	if (pipe (handle) != 0)
	{
		TRDEB ("Error en pipe");
		return false;
	}

	pid_t child= vfork ();
	switch (child)
	{
	case pid_t (0):
		// Child
		close (handle [0] );
		dup2 (handle [1], STDOUT_FILENO);
		dup2 (handle [1], STDERR_FILENO);
		close (handle [1] );
		{
			const char * strShell;
			strShell= getenv ("SHELL");
			if (! strShell)
				strShell= "/bin/sh";
			execlp (strShell, strShell,
				"-c", strCommand.c_str (),
				static_cast <const char *> (0) );
		}
		TRDEB ("Error en execlp");
		exit (1);
		return false;
	case pid_t (-1):
		// Error
		TRDEB ("Error en fork");
		close (handle [0] );
		close (handle [1] );
		return false;
	default:
		hchild= handle [0];
		close (handle [1] );
		pidchild= child;
		return true;
	}
}

void Exec::Impl::readfromchild ()
{
	TRF;

	// Put the handle in non blocking mode.
	{
		int r= fcntl
			(hchild, F_SETFL, static_cast <long> (O_NONBLOCK) );
		if (r != 0)
		{
			TRDEB (strerror (errno) );
		}
	}

	const size_t bufsize= 1024;
	char buffer [bufsize];
	bool aborted= false;

	for (;;)
	{
		TRDEB ("Reading from child");
		size_t r= read (hchild, buffer, bufsize);
		if (r == 0)
		{
			TRDEB ("End of child");
			break;
		}
		TRDEB ("Read done");

		if (r == size_t (-1) )
		{
			if (errno == EAGAIN)
			{
				r= 0;
				sleep (1);
			}
			else
			{
				TRDEB ("Error en read");
				break;
			}
		}

		if (r > 0)
		{
			// Try to send block, if rejected
			// send char by char.
			if (! exec.ExecSendBlock (buffer, r) )
			{
				for (size_t i= 0; i < r; ++i)
					exec.ExecSendChar (buffer [i] );
			}
		}

		if (exec.ExecQueryAbort () )
		{
			aborted= true;
			break;
		}
	}

	close (hchild);
	hchild= h_none;

	if (aborted)
	{
		TRDEB ("Killing child");
		kill (pidchild, SIGTERM);
	}

	TRDEB ("Waiting for child");
	waitpid (pidchild, 0, 0);

	pidchild= pid_none;
}

void Exec::Impl::Command (const std::string & strCommand)
{
	TRF;

	if (createchild (strCommand) )
		readfromchild ();
}

//***********************************************
//		Exec
//***********************************************

Exec::Exec () :
	pi (new Impl (* this) )
{
}

Exec::~Exec ()
{
	delete pi;
}

bool Exec::ExecSendBlock (const char * /*str*/, size_t /*size*/)
{
	return false;
}

bool Exec::ExecQueryAbort ()
{
	return false;
}

void Exec::Command (const std::string & strCommand)
{
	TRF;

	pi->Command (strCommand);
}

// Fin de exec.cpp

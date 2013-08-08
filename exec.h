#ifndef INCLUDE_EXEC_H_
#define INCLUDE_EXEC_H_

// exec.h
// Revision 18-sep-2004

#include <string>

class Exec {
protected:
	Exec ();
	virtual ~Exec ();
	virtual void ExecSendChar (char c)= 0;
	virtual bool ExecSendBlock (const char * str, size_t size);
	virtual bool ExecQueryAbort ();
	void Command (const std::string & strCommand);
private:
	class Impl;
	Impl * pi;
	friend class Impl;
};

#endif

// Fin de exec.h

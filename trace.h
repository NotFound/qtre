#ifndef INCLUDE_TRACE_H
#define INCLUDE_TRACE_H

// trace.h
// Revision 30-jan-2006

#include "config_debug.h"
#include "config_function.h"

#include <cstddef>
#include <string>


class TraceFunc {
public:
	TraceFunc (const char * filename, size_t linenum,
		const char * funcname_n= 0, const char * shortname_n= 0);
	~TraceFunc ();
	void message (const std::string & text);
	static void show (int);
private:
	const char * funcname;
	const char * shortname;
	TraceFunc * * previous;
	TraceFunc * next;
};


#ifndef NDEBUG

#include <sstream>

#define TRFUNC(tr,name) \
	TraceFunc tr (__FILE__, __LINE__, name)
#define TRMESSAGE(tr,text) \
	tr.message (text)
#define TRSTREAM(tr,text) \
	{ \
		std::ostringstream oss; \
		oss << text; \
		tr.message (oss.str () ); \
	}

#ifdef HAVE_FUNCTION

#ifdef HAVE_PRETTY_FUNCTION
#define TRF \
	TraceFunc tracefunc_obj (__FILE__, __LINE__, \
		__PRETTY_FUNCTION__, __FUNCTION__)
#define TRFDEB(text) \
	TraceFunc tracefunc_obj (__FILE__, __LINE__, \
		__PRETTY_FUNCTION__, __FUNCTION__); \
	tracefunc_obj.message (text)
#define TRFDEBS(tex) \
	TraceFunc tracefunc_obj (__FILE__, __LINE__, \
		__PRETTY_FUNCTION__, __FUNCTION__); \
	{ \
		std::ostringstream oss; \
		oss << tex; \
		tracefunc_obj.message (oss.str () ); \
	}
#else
#define TRF \
	TraceFunc tracefunc_obj (__FILE, __LINE__, __FUNCTION__)
#define TRFDEB(text) \
	TraceFunc tracefunc_obj (__FILE__, __LINE__, __FUNCTION__); \
	tracefunc_obj.message (text)
#define TRFDEBS(tex) \
	TraceFunc tracefunc_obj (__FILE__, __LINE__, __FUNCTION__); \
	{ \
		std::ostringstream oss; \
		oss << tex; \
		tracefunc_obj.message (oss.str () ); \
	}
#endif

#else

#define TRF \
	TraceFunc tracefunc_obj (__FILE__, __LINE__)
#define TRFDEB(text) \
	TraceFunc tracefunc_obj (__FILE__, __LINE__); \
	tracefunc_obj.message (text)
#define TRFDEBS(tex) \
	TraceFunc tracefunc_obj (__FILE__, __LINE__); \
	{ \
		std::ostringstream oss; \
		oss << tex; \
		tracefunc_obj.message (oss.str () ); \
	}

#endif

#define TRDEB(text) \
	tracefunc_obj.message (text)

#define TRDEBS(tex) \
	{ \
		std::ostringstream oss; \
		oss << tex; \
		tracefunc_obj.message (oss.str () ); \
	}


void trace_assertion_failed (const char * a, const char * file, size_t line);

#define ASSERT(a) \
	if (a) ; \
	else trace_assertion_failed (#a, __FILE__, __LINE__)


#else


#define TRFUNC(tr,name)
#define TRMESSAGE(tr,text)
#define TRSTREAM(tr,text)

#define TRF
#define TRFDEB(text)
#define TRFDEBS(tex)
#define TRDEB(text)
#define TRDEBS(tex)

#define ASSERT(a)


#endif


#endif

// Fin de trace.h

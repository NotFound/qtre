#ifndef INCLUDE_QPERL_H_
#define INCLUDE_QPERL_H_

/*
	qperl.h
	Revision 29-jan-2005
*/

#ifdef __cplusplus
extern "C" {
#endif

int qperl_init (char * * env, void * data);
void qperl_init_done ();
void qperl_destroy (void);

typedef enum {
	qpMode,
	qpIsEmpty, qpGetModified, qpSetModified,
	qpUp, qpDown, qpLeft, qpRight,
	qpHome, qpEnd, qpPageUp, qpPageDown,
	qpGotoLine,
	qpGetChar, qpGetWord,
	qpDeleteChar, qpBackspace,
	qpDeleteLine, qpReplaceLine,
	qpInsertChar, qpInsert,

	qpGetSelection, qpSelectionLimits,
	qpSelect, qpBeginSelect, qpEndSelect,
	qpSelectAll, qpDeleteSelection,

	qpGetClipboard,
	qpSetClipboard,
	qpCut, qpCopy, qpPaste,

	qpLine, qpNumLines, qpCharLine,
	qpGetLine, qpGetPos, qpGetCol,
	qpGetFileExt, qpGetFileName,
	qpStatus, qpStatusQuestion,
	qpDialogGetString, qpDialogShowText,
	qpDefineControl,
	qpRedraw, qpQuiet, qpUpdate,
	qpGetKey,
	qpSetAutoindent, qpGetAutoindent, qpSetAutocr, qpGetAutocr,

	qpMenuAdd, qpMenuAddOption, qpMenuRename,

	/* qpWindowNew, qpWindowNext, qpWindowClose, */

	qpFileLoad, qpFileNew, qpFileSave, qpFileSaveAs,
	qpFileNext, qpFileClose, qpFileQuit,
	qpHelpAddFunc,
	qpMacro,
	qpFreeString,
	qpCopyNewFile,

	qpHelp, qpHelpPerl, qpHelpAbout,

	qpCompile, qpMake, qpCompileOptions, qpMan, qpManOf,

	/* Do not insert new function codes after here */

	qpFailTest, qpLastFuncCode= qpFailTest
} qpFuncCode;

typedef enum {
	qpRetOk= 0,
	qpRetFailed,
	qpRetInvalidFunc,
	qpRetInvalidValue,
	qpRetTrue,
	qpRetFalse,
	qpRetUnexpected= -1
} qpRetCode;

const char * nameqpFunc (qpFuncCode n);

typedef struct {
	char * str;
	size_t len;
} qpStringParam;

typedef struct {
	int beginline;
	int beginpos;
	int endline;
	int endpos;
} qpSelectionLimitsParam;

typedef struct {
	const char * question;
	const char * values;
	char result;
} qpStatusQuestionParam;

typedef struct {
	const char * title;
	const char * prompt;
	char * str;
	unsigned int width;
	unsigned int maxlength;
} qpDialogGetStringParam;

typedef struct {
	const char * title;
	const char * str;
	unsigned int height;
	unsigned int width;
} qpDialogShowTextParam;

typedef struct {
	char key;
	const char * function;
} qpDefineControlParam;

typedef struct {
	int position;
	const char * menuname;
} qpMenuAddParam;

typedef struct {
	const char * menuname;
	int position;
	const char * optionname;
	const char * perlfunc;
} qpMenuAddOptionParam;

typedef struct {
	const char * from;
	const char * to;
} qpMenuRenameParam;

typedef struct {
	const char * func;
	const char * desc;
} qpHelpAddFuncParam;

qpRetCode qpCallback (void *, qpFuncCode, int n, void *);

typedef enum { qpfunction_notfound, qpfunction_failed, qpfunction_ok }
	qpfunctionresult;

qpfunctionresult qperl_function (const char * function, void * data);
qpfunctionresult qperl_function_args (const char * function, void * data,
	int argc, const char * const * argv);
qpfunctionresult qperl_function_args_r (const char * function, void * data,
	int argc, const char * const * argv, char * * result);

void qperl_free_str (char * str);

int qperl_eval (const char * str, void * data);

#ifdef __cplusplus
}
#endif

#endif

/* Fin de qperl.h */

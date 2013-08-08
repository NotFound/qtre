#ifndef INCLUDE_ACTION_H
#define INCLUDE_ACTION_H

// action.h
// Revision 27-sep-2004

#include <map>

class Action {
public:
	Action ();
	virtual ~Action ();
	virtual void doit ()= 0;
	//virtual Action * clone ()= 0;
private:
	Action (const Action &);
	void operator= (const Action &);
};

class MapKeyAction {
public:
	virtual ~MapKeyAction ();
	void addKeyAction (int key, Action * action);
	Action * getKeyAction (int key);

	typedef std::map <int, Action *> mapka;
private:
	mapka m;
};

#endif

// End of action.h

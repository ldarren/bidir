#pragma once
#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include "./page.h"
#include "./script_obj.h"
#include <map>

class Application
{
	typedef std::map<int, Page*> PagesType;

public:
	static const char * const sType;

	Application();
	virtual ~Application();

	bool init();
	bool update();
	bool deinit();

	static void installScript(Script *s);
	static void uninstallScript(Script *s);

	Page* addPage(int id, Page *page);
	void setContent(int id);
	Page* getPage(int id);

	int avg(int argc, int argv[]);
	int sum(int argc, int argv[]);

protected:
	Page		*pCurrPage;
	PagesType	mPages_;

private:
	char		mTestStr[32];
	int			mTestInt;
	double		mTestFloat;

private:
	// script functions and variables
	static int sfCreate(lua_State *L);
	static int sfDestroy(lua_State *L);
	static int sfSetContent(lua_State *L);

	// globals function and varables
	static int sfAverage(lua_State *L);
	static int		svDragConst;
	static double	svFresnelConst;
	static char		svName[64];

	// bare minimum requirement for script binding
	static ScriptObj<Application> mS__;
};

#endif // _APPLICATION_H_
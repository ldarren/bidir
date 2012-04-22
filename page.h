#pragma once
#ifndef _PAGE_H_
#define _PAGE_H_

#include "./dialog.h"
#include "./script_obj.h"

#include <stdlib.h>
#include <string.h>

struct Page : public Dialog
{
	static const char * const sType;

	Page(const char *title, int x, int y, int w, int h, int id);
	virtual ~Page();

	static void installScript(Script *s);
	static void uninstallScript(Script *s);

	bool onWake();
	bool update(int choice);
	bool onSleep();

	int getId() const { return nId_; }

	virtual bool onEvent(int evt);

protected:
	int nId_;

private:
	// script functions and variables
	static int sfCreate(lua_State *L);
	static int sfDestroy(lua_State *L);

	static ScriptObj<Page> mS__;
};

#endif // _PAGE_H_
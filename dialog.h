#pragma once
#ifndef _DIALOG_H_
#define _DIALOG_H_

#include "./rectangle.h"
#include "./script_obj.h"

#include <stdlib.h>
#include <string.h>

struct Dialog : public Rectangle
{
	static const char * const sType;

	Dialog(const char *title, int x, int y, int w, int h)
		: Rectangle(x, y, w, h)
	{
		sTitle_ = (char*)malloc(strlen(title)+16);
		setTitle(title);
	}

	virtual ~Dialog()
	{
		free(sTitle_);
	}

	static void installScript(Script *s)
	{
		Rectangle::installScript(s);

		s->startClass(sType, sfCreate, Rectangle::sType);
			s->defVar("title", Script::ESDT_PtrStr, offsetof(Dialog, sTitle_));
		s->endClass(sfDestroy);
	}

	static void uninstallScript(Script *s)
	{
		s->removeClass(sType);
	}

	virtual bool onEvent(int evt)
	{
		return true;
	}

	void setTitle(const char *title) { strcpy(sTitle_, title); }
	const char* getTitle() const { return sTitle_; }

protected:
	char *sTitle_;

private:
	static int sfCreate(lua_State *L)
	{
		return 0;
	}

	static int sfDestroy(lua_State *L)
	{
		return 0;
	}
};

#endif // _DIALOG_H_
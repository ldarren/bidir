#pragma once
#ifndef _RECTANGLE_H_
#define _RECTANGLE_H_

#include "./script_obj.h"

struct Rectangle
{
	static const char * const sType;

	Rectangle(int x, int y, int w, int h)
		: nX_(x), nY_(y), nWidth_(w), nHeight_(h)
	{
	}

	virtual ~Rectangle()
	{
	}

	static void installScript(Script *s)
	{
		s->startClass(sType, sfCreate);
			s->defVar("x", Script::ESDT_Int, offsetof(Rectangle, nX_));
			s->defVar("y", Script::ESDT_Int, offsetof(Rectangle, nY_));
			s->defVar("width", Script::ESDT_Int, offsetof(Rectangle, nWidth_));
			s->defVar("height", Script::ESDT_Int, offsetof(Rectangle, nHeight_));
		s->endClass(sfDestroy);
	}

	static void uninstallScript(Script *s)
	{
		s->removeClass(sType);
	}

	void setPosition(int x, int y) { nX_=x; nY_=y; }
	void setSize(int w, int h) { nWidth_=w; nHeight_=h; }
	int getX() const { return nX_; }
	int getY() const { return nY_; }
	int getWidth() const { return nWidth_; }
	int getHeight() const { return nHeight_; }

protected:
	int nX_;
	int nY_;
	int nWidth_;
	int nHeight_;

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

#endif // _RECTANGLE_H_
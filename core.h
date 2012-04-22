#pragma once
#ifndef _CORE_H_
#define _CORE_H_

#include "./app.h"

// all "singleton" objects should place here

struct Core 
{
	void init();
	bool update();
	void deinit();

	static Application& getApplication();

protected:
	static Application	mApp_;
};

#endif // _CORE_H_
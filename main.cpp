#include "./script_mgr.h"
#include "./script.h"
#include "./core.h"

Application Core::mApp_;

void Core::init()
{
	mApp_.init();
}

bool Core::update()
{
	bool ret = true;
	ret &=mApp_.update();

	return ret;
}

void Core::deinit()
{
	mApp_.deinit();
}

Application& Core::getApplication() { return mApp_; }

int main(int argc, char **argv)
{
	ScriptMgr::init();

	Core core;
	core.init();

	while (core.update())
	{
	}

	core.deinit();

	ScriptMgr::deinit();

	return 0;
}

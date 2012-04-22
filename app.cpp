#include "./app.h"
#include "./page_id.h"
#include "./core.h"

const char * const Application::sType = "Application";
ScriptObj<Application> Application::mS__;

int		Application::svDragConst = 0;
double	Application::svFresnelConst = 67.89;
char	Application::svName[64] = "Application1";

Application::Application()
: pCurrPage(0), mTestInt(99), mTestFloat(-99.99)
{
	strcpy(mTestStr, "testStrWorks");
}

Application::~Application()
{
}

bool Application::init()
{
	addPage(EPID_Menu, new Page("MenuPage", 0, 0, 1024, 768, EPID_Menu));
	addPage(EPID_Game, new Page("GamePage", 0, 0, 1024, 768, EPID_Game));
	setContent(EPID_Menu);

	int width;
	double height;
	char path[256];
	mS__.setVar("nWidth", 800.5);
	mS__.getVar("nWidth", &width);
	mS__.getVar("nHeight", &height);
	mS__.setVar("sPath", "is//here//right//here");
	mS__.getVar("sPath", path);
	printf("From Lua to C++: Width[%d] Height[%f] Path[%s]\n", width, height, path);

	char i1[8], i2[8], i3[8], i4[8];
	mS__.call("multiCall", "%s %s %s %s", "%s %s %s %s", "is", "a", "good", "man", i1, i2, i3, i4);
	printf("multicall output: %s %s %s %s\n", i1, i2, i3, i4);

	mS__.invoke(this, "uselessMethod", "", "");

	mS__.setVar(this, "testStr", "Hello");
	char sResult[64];
	mS__.getVar(this, "testStr", sResult);

	mS__.setVar(this, "testFloat", 700.7);
	double fResult;
	mS__.getVar(this, "testFloat", &fResult);
	printf("C++, set get object property sResult[%s] fResult[%f]\n", sResult, fResult);

	return true;
}

bool Application::update()
{
	int choice;
	printf("Select your next page: \n");
	for (int i=0; i<EPID_Max; ++i)
	{
		printf("%d) %s\n", i, getPage(i)->getTitle());
	}
	printf("%d) %s\n", EPID_Max, "Quit");
	scanf("%d", &choice);

	if (choice==EPID_Max) return false;

	pCurrPage->update(choice);

	return true;
}

bool Application::deinit()
{
	if (pCurrPage) pCurrPage->onSleep();

	PagesType::iterator iter;
	for (iter=mPages_.begin(); iter != mPages_.end(); ++iter)
	{
		delete iter->second;
	}
	mPages_.clear();

	return true;
}

void Application::installScript(Script *s)
{
	s->defFunc("Average", &sfAverage);
	s->defVar("gnDrag", &svDragConst);
	s->defVar("gfFresnel", &svFresnelConst);
	s->defVar("gsName", svName);

	s->startClass(sType, &sfCreate);
		s->defFunc("SetContent", &sfSetContent);
		s->defVar("testStr", Script::ESDT_Str, offsetof(Application, mTestStr));
		s->defVar("testInt", Script::ESDT_Int, offsetof(Application, mTestInt));
		s->defVar("testFloat", Script::ESDT_Num, offsetof(Application, mTestFloat));
	s->endClass(&sfDestroy);
}

void Application::uninstallScript(Script *s)
{
	s->undefFunc("Average");
	s->undefVar("gnDrag");
	s->undefVar("gfFresnel");
	s->undefVar("gsName");

	s->removeClass(sType);
}

Page* Application::addPage(int id, Page *page)
{
	Page *p = getPage(id);
	if (p) return p;

	mPages_[id] = page;

	return 0;
}

void Application::setContent(int id)
{
	Page *p = getPage(id);
	if (!p) return;
	if (pCurrPage) pCurrPage->onSleep();
	pCurrPage = getPage(id);
	pCurrPage->onWake();

	printf("In C++, Application mTestInt:%d mTestFloat:%f mTestStr:%s\n", mTestInt, mTestFloat, mTestStr);
}

Page* Application::getPage(int id)
{
	PagesType::iterator iter = mPages_.find(id);
	if (iter == mPages_.end()) return 0;
	return mPages_[id];
}

int Application::avg(int argc, int argv[])
{
	return sum(argc, argv)/argc;
}

int Application::sum(int argc, int argv[])
{
	int total = 0;
	for (int i=0; i<argc; ++i)
	{
		total += argv[i];
	}

	return total;
}

// argc: -1
// rets: 2
int Application::sfAverage(lua_State *L)
{
	int n = lua_gettop(L); // get number of arguments
	int *argv = new int[n];
	for (int i=0; i<n; ++i)
		argv[i] = (int)lua_tointeger(L, i+1);

	lua_pushinteger(L, Core::getApplication().sum(n, argv));
	lua_pushinteger(L, Core::getApplication().avg(n, argv));

	delete [] argv;

	return 2;
}

int Application::sfCreate(lua_State *L)
{
	Application *a = &Core::getApplication();

	mS__.addObject(a, sType);

	return 1;
}
int Application::sfDestroy(lua_State *L)
{
	Application *a = reinterpret_cast<Application*>(mS__.getObject(1, sType));
	mS__.removeObject(a);

	return 0;
}

int Application::sfSetContent(lua_State *L)
{
	Application *a = reinterpret_cast<Application*>(mS__.getObject(1, sType));

	int id = mS__.popInt(2);
	if (id == a->pCurrPage->getId())
	{
		mS__.pushBool(false);
	}
	else
	{
		a->setContent(id);
		mS__.pushBool(true);
	}

	return 1;
}

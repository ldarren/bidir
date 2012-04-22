#include "./page.h"
#include "./page_id.h"
#include "./core.h"

const char * const Rectangle::sType = "Rectangle";
const char * const Dialog::sType = "Dialog";
const char * const Page::sType = "Page";
ScriptObj<Page> Page::mS__;

Page::Page(const char *title, int x, int y, int w, int h, int id)
	: Dialog(title, x, y, w, h), nId_(id)
{
}

Page::~Page()
{
}

void Page::installScript(Script *s)
{
	Dialog::installScript(s);

	s->startClass(sType, sfCreate, Dialog::sType);
	s->defVar("id", Script::ESDT_Int, offsetof(Page, nId_));
	s->endClass(sfDestroy);
}

void Page::uninstallScript(Script *s)
{
	s->removeClass(sType);
}

bool Page::onWake()
{
	int len = printf("Welcome to %s page\n", sTitle_);
	for (int i=0; i<len; ++i) printf("=");
	printf("\n\n");
	return true;
}

bool Page::update(int choice)
{
	if (choice == nId_) 
	{
		printf("Thanks for choosing me again\n");
		return true;
	}
	printf("Your choice is %s\n", Core::getApplication().getPage(choice)->getTitle());
	Core::getApplication().setContent(choice);
	return true;
}

bool Page::onSleep()
{
	printf("Shutting down %s...\n", sTitle_);
	return true;
}

bool Page::onEvent(int evt)
{
	return true;
}

int Page::sfCreate(lua_State *L)
{
	Page *p = new Page(mS__.popStr(1), mS__.popInt(2), mS__.popInt(3), mS__.popInt(4), mS__.popInt(5), mS__.popInt(6));

	mS__.addObject(p, sType);

	return 1;
}

int Page::sfDestroy(lua_State *L)
{
	Page *p = (Page*)mS__.getObject(1, sType);
	mS__.removeObject(p);
	delete p;

	return 0;
}

#include <cage-core/logger.h>

#include "main.h"

uint32 CageTestCase::counter = 0;

void testBasics();

int main()
{
	Holder<Logger> log = newLogger();
	log->format.bind<logFormatConsole>();
	log->output.bind<logOutputStdOut>();

	testBasics();

	{
		CAGE_TESTCASE("all tests done ok");
	}

	return 0;
}

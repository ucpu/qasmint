#include <cage-core/logger.h>

#include "main.h"

uint32 CageTestCase::counter = 0;

void testCompilation();
void testArithmetics();
void testStructures();
void testFlow();
void testInputOutput();
void testDebugging();

int main()
{
	Holder<Logger> log = newLogger();
	log->format.bind<logFormatConsole>();
	log->output.bind<logOutputStdOut>();

	testCompilation();
	testArithmetics();
	testStructures();
	testFlow();
	testInputOutput();
	testDebugging();

	{
		CAGE_TESTCASE("all tests done ok");
	}

	return 0;
}

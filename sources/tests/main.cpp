#include <cage-core/logger.h>

using namespace cage;

void testBasics();

int main()
{
	Holder<Logger> log = newLogger();
	log->format.bind<logFormatConsole>();
	log->output.bind<logOutputStdOut>();

	testBasics();

	CAGE_LOG(SeverityEnum::Info, "tests", "all tests done ok");
	return 0;
}

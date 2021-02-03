#include <cage-core/logger.h>

#include <qasm/qasm.h>

using namespace qasm;

int main(int argc, const char *args[])
{
	try
	{
		Holder<Logger> log = newLogger();
		log->format.bind<logFormatConsole>();
		log->output.bind<logOutputStdOut>();

		Holder<QasmCompiler> compiler = newQasmCompiler();

		Holder<QasmCpu> cpu = newQasmCpu();

		// todo

		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}

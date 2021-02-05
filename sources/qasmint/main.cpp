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

		Holder<Compiler> compiler = newCompiler();
		Holder<BinaryProgram> program = compiler->compile({});
		Holder<Cpu> cpu = newCpu({});

		// todo

		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}

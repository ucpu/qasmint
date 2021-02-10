#include <cage-core/logger.h>
#include <cage-core/ini.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/memoryBuffer.h>

#include <qasm/qasm.h>

#include "io.h"

using namespace qasm;

int main(int argc, const char *args[])
{
	try
	{
		Holder<Logger> logger = newLogger();
		logger->format.bind<logFormatConsole>();
		logger->output.bind<logOutputStdOut>();

		ConfigString programPath("qasmint/path/program", "source.qasm");
		ConfigString limitsPath("qasmint/path/limits");
		ConfigString inputPath("qasmint/path/input");
		ConfigString outputPath("qasmint/path/output");
		ConfigBool suppressConsoleLog("qasmint/log/suppressConsole");

		{
			Holder<Ini> ini = newIni();
			ini->parseCmd(argc, args);
			programPath = ini->cmdString('p', "program", programPath);
			limitsPath = ini->cmdString('l', "limits", limitsPath);
			inputPath = ini->cmdString('i', "input", inputPath);
			outputPath = ini->cmdString('o', "output", outputPath);
			suppressConsoleLog = ini->cmdBool('f', "filter", suppressConsoleLog);
			ini->checkUnusedWithHelp();
		}

		if (suppressConsoleLog)
			logger.clear();

		Holder<Program> program;
		{
			CAGE_LOG(SeverityEnum::Info, "qasmint", stringizer() + "loading program at path: '" + string(programPath) + "'");
			Holder<File> file = readFile(programPath);
			Holder<Compiler> compiler = newCompiler();
			program = compiler->compile(file->readAll());
			CAGE_LOG(SeverityEnum::Info, "qasmint", stringizer() + "program has: " + program->instructionsCount() + " instructions");
		}

		Holder<Input> input = newInput(inputPath);
		Holder<Output> output = newOutput(outputPath);
		Holder<Cpu> cpu;
		{
			CpuCreateConfig cfg;
			if (!string(limitsPath).empty())
			{
				CAGE_LOG(SeverityEnum::Info, "qasmint", stringizer() + "loading limits at path: '" + string(limitsPath) + "'");
				Holder<Ini> limits = newIni();
				limits->importFile(limitsPath);
				cfg.limits = qasm::limitsFromIni(+limits);
			}
			cfg.input.bind<Input, &Input::readLine>(+input);
			cfg.output.bind<Output, &Output::writeLine>(+output);
			cpu = newCpu(cfg);
			cpu->program(+program);
		}

		try
		{
			cpu->run();
			CAGE_LOG(SeverityEnum::Note, "qasmint", stringizer() + "finished in " + cpu->stepIndex() + " steps");
		}
		catch (...)
		{
			CAGE_LOG(SeverityEnum::Note, "qasmint", stringizer() + "function: " + program->functionName(cpu->functionIndex()));
			CAGE_LOG(SeverityEnum::Note, "qasmint", stringizer() + "source: " + program->sourceCodeLine(cpu->sourceLine()));
			CAGE_LOG(SeverityEnum::Note, "qasmint", stringizer() + "line: " + (cpu->sourceLine() + 1));
			CAGE_LOG(SeverityEnum::Note, "qasmint", stringizer() + "step: " + cpu->stepIndex());
			throw;
		}

		output->close();

		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}

#include <cage-core/logger.h>
#include <cage-core/ini.h>
#include <cage-core/config.h>
#include <cage-core/files.h>
#include <cage-core/image.h>

#include <qasm/qasm.h>

using namespace qasm;

int main(int argc, const char *args[])
{
	try
	{
		Holder<Logger> logger = newLogger();
		logger->format.bind<logFormatConsole>();
		logger->output.bind<logOutputStdOut>();

		ConfigString programPath("imgmod/path/program", "imgmod.qasm");
		ConfigString limitsPath("imgmod/path/limits");
		ConfigString inputPath("imgmod/path/input");
		ConfigString outputPath("imgmod/path/output");

		{
			Holder<Ini> ini = newIni();
			ini->parseCmd(argc, args);

			programPath = ini->cmdString('p', "program", programPath);
			limitsPath = ini->cmdString('l', "limits", limitsPath);
			inputPath = ini->cmdString('i', "input", inputPath);
			outputPath = ini->cmdString('o', "output", outputPath);

			ini->checkUnusedWithHelp();
		}

		if (string(inputPath).empty() || string(outputPath).empty())
			CAGE_THROW_ERROR(Exception, "no input or output path");

		Holder<Image> img = newImage();
		ImageFormatEnum originalFormat = ImageFormatEnum::Default;
		{
			CAGE_LOG(SeverityEnum::Info, "imgmod", stringizer() + "loading image at path: '" + string(inputPath) + "'");
			img->importFile(inputPath);
			CAGE_LOG(SeverityEnum::Info, "imgmod", stringizer() + "resolution: " + img->width() + "x" + img->height());
			CAGE_LOG(SeverityEnum::Info, "imgmod", stringizer() + "channels: " + img->channels());
			originalFormat = img->format();
			imageConvert(+img, ImageFormatEnum::Float);
		}

		Holder<Program> program;
		{
			CAGE_LOG(SeverityEnum::Info, "imgmod", stringizer() + "loading program at path: '" + string(programPath) + "'");
			Holder<File> file = readFile(programPath);
			Holder<Compiler> compiler = newCompiler();
			program = compiler->compile(file->readAll());
			CAGE_LOG(SeverityEnum::Info, "imgmod", stringizer() + "program has: " + program->instructionsCount() + " instructions");
		}

		Holder<Cpu> cpu;
		{
			CpuCreateConfig cfg;
			for (uint32 i = 0; i < 4; i++)
				cfg.limits.memoryCapacity[i] = img->width() * img->height() * img->channels();
			if (!string(limitsPath).empty())
			{
				CAGE_LOG(SeverityEnum::Info, "imgmod", stringizer() + "loading limits at path: '" + string(limitsPath) + "'");
				Holder<Ini> limits = newIni();
				limits->importFile(limitsPath);
				cfg.limits = qasm::limitsFromIni(+limits, cfg.limits);
			}
			cpu = newCpu(cfg);
			cpu->program(+program);
		}

		{
			auto fv = img->rawViewFloat();
			cpu->memory(0, { (const uint32 *)fv.begin(), (const uint32 *)fv.end() });
			uint32 regs[26];
			regs['W' - 'A'] = img->width();
			regs['H' - 'A'] = img->height();
			regs['C' - 'A'] = img->channels();
			cpu->registers(regs);
		}

		try
		{
			cpu->run();
			CAGE_LOG(SeverityEnum::Note, "imgmod", stringizer() + "finished in " + cpu->stepIndex() + " steps");
		}
		catch (...)
		{
			CAGE_LOG(SeverityEnum::Note, "imgmod", stringizer() + "function: " + program->functionName(cpu->functionIndex()));
			CAGE_LOG(SeverityEnum::Note, "imgmod", stringizer() + "source: " + program->sourceCodeLine(cpu->sourceLine()));
			CAGE_LOG(SeverityEnum::Note, "imgmod", stringizer() + "line: " + (cpu->sourceLine() + 1));
			CAGE_LOG(SeverityEnum::Note, "imgmod", stringizer() + "step: " + cpu->stepIndex());
			throw;
		}

		{
			Holder<Image> img = newImage();
			const auto mem = cpu->memory(0);
			const auto regs = cpu->registers();
			img->importRaw({ (const char *)mem.begin(), (const char *)mem.end() }, regs['W' - 'A'], regs['H' - 'A'], regs['C' - 'A'], ImageFormatEnum::Float);
			CAGE_LOG(SeverityEnum::Info, "imgmod", stringizer() + "resolution: " + img->width() + "x" + img->height());
			CAGE_LOG(SeverityEnum::Info, "imgmod", stringizer() + "channels: " + img->channels());
			imageConvert(+img, originalFormat);
			CAGE_LOG(SeverityEnum::Info, "imgmod", stringizer() + "saving image at path: '" + string(outputPath) + "'");
			img->exportFile(outputPath);
		}

		return 0;
	}
	catch (...)
	{
		detail::logCurrentCaughtException();
	}
	return 1;
}

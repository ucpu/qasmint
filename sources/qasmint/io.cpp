#include <cage-core/files.h>
#include <cage-core/lineReader.h>

#include "io.h"

#include <iostream>
#include <string>

struct InputImpl : public Input
{
	Holder<File> file;

	InputImpl(const string &path)
	{
		if (!path.empty())
		{
			CAGE_LOG(SeverityEnum::Info, "qasmint", stringizer() + "redirecting input from: '" + path + "'");
			file = readFile(path);
		}
	}

	bool readLine(string &line)
	{
		if (file)
		{
			return file->readLine(line);
		}
		else
		{
			std::string l;
			if (!std::getline(std::cin, l))
				return false;
			line = string(l);
			return true;
		}
	}
};

bool Input::readLine(string &line)
{
	InputImpl *impl = (InputImpl *)this;
	return impl->readLine(line);
}

Holder<Input> newInput(const string &path)
{
	return detail::systemArena().createImpl<Input, InputImpl>(path);
}

struct OutputImpl : public Output
{
	Holder<File> file;

	OutputImpl(const string &path)
	{
		if (!path.empty())
		{
			CAGE_LOG(SeverityEnum::Info, "qasmint", stringizer() + "redirecting output into: '" + path + "'");
			file = writeFile(path);
		}
	}

	bool writeLine(const string &line)
	{
		if (file)
		{
			file->writeLine(line);
			return true;
		}
		else
		{
			std::cout << line.data() << std::endl;
			return true;
		}
	}
};

bool Output::writeLine(const string &line)
{
	OutputImpl *impl = (OutputImpl *)this;
	return impl->writeLine(line);
}

void Output::close()
{
	OutputImpl *impl = (OutputImpl *)this;
	if (impl->file)
		impl->file->close();
}

Holder<Output> newOutput(const string &path)
{
	return detail::systemArena().createImpl<Output, OutputImpl>(path);
}

#ifndef io_h_s54gse85
#define io_h_s54gse85

#include <cage-core/core.h>

using namespace cage;

struct Input : private Immovable
{
	bool readLine(string &line);
};

Holder<Input> newInput(const string &path);

struct Output : private Immovable
{
	bool writeLine(const string &line);
	void close();
};

Holder<Output> newOutput(const string &path);

#endif // io_h_s54gse85

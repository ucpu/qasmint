#include <cage-core/math.h>
#include <cage-core/lineReader.h>
#include <cage-core/memoryBuffer.h>

#include "main.h"

namespace
{
	struct Output
	{
		uintPtr capacity = m;

		MemoryBuffer data;

		bool writeln(const string &line)
		{
			string l = line + string('\n');
			if (data.size() + l.length() > capacity)
				return false;
			uintPtr pos = data.size();
			data.resizeSmart(data.size() + l.length());
			detail::memcpy(data.data() + pos, l.data(), l.length());
			return true;
		}

		template<uint32 N>
		void test(const char(&expected)[N]) const
		{
			CAGE_TEST(data.size() + 1 == N);
			CAGE_TEST(detail::memcmp(expected, data.data(), data.size()) == 0);
		}
	};
}

void testInputOutput()
{
	CAGE_TESTCASE("input and output");

	{
		CAGE_TESTCASE("sum two numbers");
		constexpr const char source[] = R"asm(
readln
read A
readln
read B
add C A B
write C
writeln
)asm";
		constexpr const char input[] = R"text(42
13
)text";
		constexpr const char expected[] = R"text(55
)text";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<LineReader> reader = newLineReader(input);
		Output output;
		CpuCreateConfig cfg;
		cfg.input = Delegate<bool(string &)>().bind<LineReader, &LineReader::readLine>(+reader);
		cfg.output = Delegate<bool(const string &)>().bind<Output, &Output::writeln>(&output);
		Holder<Cpu> cpu = newCpu(cfg);
		cpu->program(+program);
		CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
		cpu->run();
		CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		output.test(expected);
	}

	{
		CAGE_TESTCASE("copy output to input");
		constexpr const char source[] = R"asm(
label Start
readln
inv z
condjmp End
rwswap
writeln
jump Start
label End
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<LineReader> reader = newLineReader(source);
		Output output;
		CpuCreateConfig cfg;
		cfg.input = Delegate<bool(string &)>().bind<LineReader, &LineReader::readLine>(+reader);
		cfg.output = Delegate<bool(const string &)>().bind<Output, &Output::writeln>(&output);
		Holder<Cpu> cpu = newCpu(cfg);
		cpu->program(+program);
		CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
		cpu->run();
		CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		output.test(source);
	}
}

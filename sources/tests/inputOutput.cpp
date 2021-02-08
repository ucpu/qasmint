#include <cage-core/math.h>
#include <cage-core/lineReader.h>
#include <cage-core/memoryBuffer.h>

#include "main.h"

#include <vector>
#include <algorithm> // is_sorted

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

	uint32 countLines(PointerRange<const char> str)
	{
		Holder<LineReader> reader = newLineReader(str);
		uint32 cnt = 0;
		string line;
		while (reader->readLine(line))
			cnt++;
		return cnt;
	}
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

	{
		CAGE_TESTCASE("count characters per line");
		constexpr const char source[] = R"asm(
label Start
readln
inv z
condjmp End
label Char
rstat
not z c
condjmp Line
cread D
inc C
jump Char
label Line
write C
writeln
set C 0
jump Start
label End
)asm";
		constexpr const char expected[] = R"text(0
11
6
5
11
10
5
7
12
7
5
9
10
7
7
7
10
9
)text";
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
		output.test(expected);
	}

	{
		CAGE_TESTCASE("reading beyond line end");
		constexpr const char source[] = R"asm(
readln
label Start
cread A
jump Start
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<LineReader> reader = newLineReader(source);
		CpuCreateConfig cfg;
		cfg.input = Delegate<bool(string &)>().bind<LineReader, &LineReader::readLine>(+reader);
		Holder<Cpu> cpu = newCpu(cfg);
		cpu->program(+program);
		CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
		CAGE_TEST_THROWN(cpu->run());
	}

	{
		CAGE_TESTCASE("random numbers");
		Output numbers;

		{
			CAGE_TESTCASE("generate 100 random numbers");
			constexpr const char source[] = R"asm(
set I 0       # count of generated numbers
set T 100     # count of numbers to generate
label Loop
rand J        # generate random number and store it in register J
write J       # write the number from register J to output buffer
writeln       # flush the output buffer to standard output
inc I         # increment the counter of generated numbers
lt z I T      # compare I < T and store it in z
condjmp Loop  # go generate another number if we are below the limit
)asm";
			Holder<Program> program = newCompiler()->compile(source);
			Holder<LineReader> reader = newLineReader(source);
			CpuCreateConfig cfg;
			cfg.input = Delegate<bool(string &)>().bind<LineReader, &LineReader::readLine>(+reader);
			cfg.output = Delegate<bool(const string &)>().bind<Output, &Output::writeln>(&numbers);
			Holder<Cpu> cpu = newCpu(cfg);
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
			CAGE_TEST(countLines(numbers.data) == 100);
		}

		{
			CAGE_TESTCASE("sort numbers");
			constexpr const char source[] = R"asm(
# read input
set C 0             # number of elements
label InputBegin
readln              # read one line from standard input into input buffer
inv z               # invert the flag whether we succeeded reading a line
condjmp SortBegin   # start sorting if there are no more numbers to read
label Input
rstat               # check what is in the input buffer
copy z u            # is it unsigned integer?
inv z               # is it NOT unsigned integer?
condjmp InvalidInput # handle invalid input
read V              # read the number into register V
store TA V          # store the number from the register V onto the current position on tape A
right TA            # move the position (the read/write head) on tape A one element to the right
inc C               # increment the counter of numbers
jump InputBegin     # go try read another number
label InvalidInput
terminate           # nothing useful to do with invalid input

# start a sorting pass over all elements
# this is the outer loop in bubble sort
label SortBegin
set M 0             # flag if anything has been modified?
center TA           # move the head on tape A to the initial position
right TA            # and one element to the right

# compare two elements and swap them if needed
# this is the inner loop in bubble sort
label Sorting
stat TA             # retrieve information about the tape A
gte z p C           # compare head position on tape A with value in register C and store it in register z
condjmp Ending      # jump to Ending if z evaluates true
left TA             # move the head one left
load L TA           # load value from tape A to register L
right TA            # move the head back
load R TA           # load value to register R
lte z L R           # compare values in registers L and R and store the result in register Z
condjmp NextPair    # skip some instructions if z is true
store TA L          # store value from register L onto the tape
left TA             # move head one left again
store TA R          # store value from R
right TA            # and move the head back again
set M 1             # mark that we made a change
label NextPair
right TA            # move head one right - this is the first instruction to execute after the skip
jump Sorting        # go try sort next pair of elements

# finished a pass over all elements
label Ending
copy z M            # copy value from register M to register z
condjmp SortBegin   # go start another sorting pass if we made any modifications

# write output
center TA           # move the head on tape A to the initial position
set Z 0             # count of outputted numbers
label Output
gte z Z C           # do we have more numbers to output?
condjmp Done        # no, we do not
load V TA           # load number from the tape A into register V
write V             # write value from register V into output buffer
writeln             # flush the output buffer to standard output
right TA            # move the head on tape A one element to the right
inc Z               # increment count of outputted numbers
jump Output         # go try output another number

label Done
)asm";
			Holder<Program> program = newCompiler()->compile(source);
			Holder<LineReader> reader = newLineReader(numbers.data);
			Output sorted;
			CpuCreateConfig cfg;
			cfg.input = Delegate<bool(string &)>().bind<LineReader, &LineReader::readLine>(+reader);
			cfg.output = Delegate<bool(const string &)>().bind<Output, &Output::writeln>(&sorted);
			Holder<Cpu> cpu = newCpu(cfg);
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
			{
				CAGE_TEST("verify sorted");
				Holder<LineReader> reader = newLineReader(sorted.data);
				string line;
				std::vector<uint32> vec;
				while (reader->readLine(line))
					vec.push_back(toUint32(line));
				CAGE_TEST(vec.size() == 100);
				CAGE_TEST(std::is_sorted(vec.begin(), vec.end()));
			}
		}
	}
}

#include <cage-core/math.h>

#include "main.h"

namespace
{
	void ftest(uint32 register_, real expected)
	{
		real value = *(real *)&register_;
		real diff = abs(value - expected);
		CAGE_TEST(diff < 1e-4);
	}
}

void testBasics()
{
	CAGE_TESTCASE("basics");
	Holder<Compiler> compiler = newCompiler();
	Holder<Cpu> cpu = newCpu({});

	{
		CAGE_TESTCASE("arithmetics");
		constexpr const char source[] = R"asm(
set A 42
set B 13
add C A B
sub D A B
mul E A B
div F A B

fset G 42.0
fset H 13.0
fadd I G H
fsub J G H
fmul K G H
fdiv L G H
)asm";
		Holder<BinaryProgram> program = compiler->compile(source);
		CAGE_TEST(cpu->state() == CpuStateEnum::None);
		{
			CAGE_TESTCASE("program");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
		}
		{
			CAGE_TESTCASE("run");
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
		{
			CAGE_TESTCASE("verify registers");
			const auto rs = cpu->explicitRegisters();
			CAGE_TEST(rs.size() == 26);
			CAGE_TEST(rs[0] == 42);
			CAGE_TEST(rs[1] == 13);
			CAGE_TEST(rs[2] == 42 + 13);
			CAGE_TEST(rs[3] == 42 - 13);
			CAGE_TEST(rs[4] == 42 * 13);
			CAGE_TEST(rs[5] == 42 / 13);
			ftest(rs[6], real(42));
			ftest(rs[7], real(13));
			ftest(rs[8], real(42) + real(13));
			ftest(rs[9], real(42) - real(13));
			ftest(rs[10], real(42) * real(13));
			ftest(rs[11], real(42) / real(13));
		}
		cpu->program(nullptr);
		CAGE_TEST(cpu->state() == CpuStateEnum::None);
	}

	/*
	{
		CAGE_TESTCASE("structures");
		constexpr const char source[] = R"asm(
set A 1
push SA A
set A 2
push SA A
set A 3
push SA A
set A 4
push SA A
pop S SA

set A 10
enqueue QA A
set A 11
enqueue QA A
set A 12
enqueue QA A
set A 13
enqueue QA A
dequeue Q QA

left TA
set A 21
store TA A
right TA
set A 22
store TA A
right TA
set A 23
store TA A
right TA
set A 24
store TA A
load T TA
)asm";
		Holder<BinaryProgram> program = compiler->compile(source);
		CAGE_TEST(cpu->state() == CpuStateEnum::None);
		{
			CAGE_TESTCASE("program");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
		}
		{
			CAGE_TESTCASE("run");
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
		{
			CAGE_TESTCASE("verify stack");
			const auto s = cpu->stack(0);
			CAGE_TEST(s.size() == 3);
			CAGE_TEST(cpu->explicitRegisters()['S' - 'A'] == 4);
		}
		{
			CAGE_TESTCASE("verify queue");
			const auto q = cpu->queue(0);
			CAGE_TEST(q.size() == 3);
			CAGE_TEST(cpu->explicitRegisters()['Q' - 'A'] == 10);
		}
		{
			CAGE_TESTCASE("verify tape");
			const auto t = cpu->tape(0);
			CAGE_TEST(t.size() == 4);
			CAGE_TEST(cpu->explicitRegisters()['T' - 'A'] == 24);
		}
		cpu->program(nullptr);
		CAGE_TEST(cpu->state() == CpuStateEnum::None);
	}
	*/
}

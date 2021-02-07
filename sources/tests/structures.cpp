#include <cage-core/math.h>

#include "main.h"

void testStructures()
{
	CAGE_TESTCASE("structures");
	Holder<Cpu> cpu = newCpu({});

	{
		CAGE_TESTCASE("basics");
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

set A 30
store MA@13 A
set A 31
store MA@42 A
load M MA@13
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		CAGE_TEST(cpu->state() == CpuStateEnum::None);
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
		{
			CAGE_TESTCASE("verify stack");
			const auto s = cpu->stack(0);
			CAGE_TEST(s.size() == 3);
			CAGE_TEST(cpu->registers()['S' - 'A'] == 4);
		}
		{
			CAGE_TESTCASE("verify queue");
			const auto q = cpu->queue(0);
			CAGE_TEST(q.size() == 3);
			CAGE_TEST(cpu->registers()['Q' - 'A'] == 10);
		}
		{
			CAGE_TESTCASE("verify tape");
			const auto t = cpu->tape(0);
			CAGE_TEST(t.size() == 4);
			CAGE_TEST(cpu->registers()['T' - 'A'] == 24);
		}
		{
			CAGE_TESTCASE("verify memory");
			const auto t = cpu->memory(0);
			CAGE_TEST(t.size() == CpuLimitsConfig().memoryCapacity[0]);
			CAGE_TEST(cpu->memory(0)[13] == 30);
			CAGE_TEST(cpu->memory(0)[42] == 31);
			CAGE_TEST(cpu->registers()['M' - 'A'] == 30);
		}
		cpu->program(nullptr);
		CAGE_TEST(cpu->state() == CpuStateEnum::None);
	}
}

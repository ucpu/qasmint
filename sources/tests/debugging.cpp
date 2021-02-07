#include <cage-core/math.h>

#include "main.h"

void testDebugging()
{
	CAGE_TESTCASE("debugging");

	{
		CAGE_TESTCASE("stepping");
		constexpr const char source[] = R"asm(
label Start
inc A
jump Start
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<Cpu> cpu = newCpu({});
		cpu->program(+program);
		CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
		cpu->step();
		CAGE_TEST(cpu->state() == CpuStateEnum::Running);
		while (cpu->registers()[0] < 100)
			cpu->step();
		CAGE_TEST(cpu->state() == CpuStateEnum::Running);
		CAGE_TEST(cpu->stepIndex() == 199);
		CAGE_TEST(cpu->registers()[0] == 100);
	}

	{
		CAGE_TESTCASE("periodic interrupt");
		constexpr const char source[] = R"asm(
label Start
inc A
jump Start
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		CpuCreateConfig cfg;
		cfg.interruptPeriod = 10;
		Holder<Cpu> cpu = newCpu(cfg);
		cpu->program(+program);
		CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
		uint32 interrupts = 0;
		while (cpu->registers()[0] < 12)
		{
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Interrupted);
			interrupts++;
		}
		CAGE_TEST(cpu->stepIndex() == 30);
		CAGE_TEST(cpu->registers()[0] == 14);
		CAGE_TEST(interrupts == 3);
	}
}

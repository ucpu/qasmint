#include <cage-core/math.h>

#include "main.h"

void testFlow()
{
	CAGE_TESTCASE("flow");

	{
		CAGE_TESTCASE("basic jump");
		constexpr const char source[] = R"asm(
set A 1
jump TheUniverse
set B 2
label TheUniverse
set C 3
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<Cpu> cpu = newCpu({});
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
		{
			CAGE_TESTCASE("verify");
			const auto rs = cpu->registers();
			CAGE_TEST(rs.size() == 26);
			CAGE_TEST(rs[0] == 1);
			CAGE_TEST(rs[1] == 0);
			CAGE_TEST(rs[2] == 3);
		}
	}

	{
		CAGE_TESTCASE("many labels and jumps");
		constexpr const char source[] = R"asm(
set A 1
jump First

label Third
set F 6
jump Fourth
set G 7

label Unused
set G 8

label First
set B 2
push SA B # just to test various alignments of parameters
jump Second

label FallThrough
set C 3 # should not be set

label Fourth
jump WhyNot
set G 9

label WhyNot
jump Done

label Second
copy D B # 2
copy E F # 0
jump Third
set G 6

label Done
set H 10
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<Cpu> cpu = newCpu({});
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
		{
			CAGE_TESTCASE("verify");
			const auto rs = cpu->registers();
			CAGE_TEST(rs.size() == 26);
			CAGE_TEST(rs[0] == 1);
			CAGE_TEST(rs[1] == 2);
			CAGE_TEST(rs[2] == 0);
			CAGE_TEST(rs[3] == 2);
			CAGE_TEST(rs[4] == 0);
			CAGE_TEST(rs[5] == 6);
			CAGE_TEST(rs[6] == 0);
			CAGE_TEST(rs[7] == 10);
			CAGE_TEST(rs[8] == 0);
			CAGE_TEST(rs[9] == 0);
		}
	}

	{
		CAGE_TESTCASE("loop with condjmp");
		constexpr const char source[] = R"asm(
set B 10
label Start
inc A
lt z A B
condjmp Start
set C 3
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<Cpu> cpu = newCpu({});
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
		{
			CAGE_TESTCASE("verify");
			const auto rs = cpu->registers();
			CAGE_TEST(rs.size() == 26);
			CAGE_TEST(rs[0] == 10);
			CAGE_TEST(rs[1] == 10);
			CAGE_TEST(rs[2] == 3);
		}
	}

	{
		CAGE_TESTCASE("just label");
		constexpr const char source[] = R"asm(
label TheEnd
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<Cpu> cpu = newCpu({});
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
	}

	{
		CAGE_TESTCASE("basic function");
		constexpr const char source[] = R"asm(
set A 1
call Fnc
set A 2

function Fnc
copy B A
return

set A 5
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<Cpu> cpu = newCpu({});
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
		{
			CAGE_TESTCASE("verify");
			const auto rs = cpu->registers();
			CAGE_TEST(rs.size() == 26);
			CAGE_TEST(rs[0] == 2);
			CAGE_TEST(rs[1] == 1);
		}
	}

	{
		CAGE_TESTCASE("functions inside functions");
		constexpr const char source[] = R"asm(
call First

function Third
inc C
call Fourth
call Fourth
return

function Second
inc B
call Third
call Third
return

function Fourth
inc D
return

function First
inc A
call Second
call Second
return
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<Cpu> cpu = newCpu({});
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
		{
			CAGE_TESTCASE("verify");
			const auto rs = cpu->registers();
			CAGE_TEST(rs.size() == 26);
			CAGE_TEST(rs[0] == 1);
			CAGE_TEST(rs[1] == 2);
			CAGE_TEST(rs[2] == 4);
			CAGE_TEST(rs[3] == 8);
		}
	}

	{
		CAGE_TESTCASE("recursion");
		constexpr const char source[] = R"asm(
set D 10
call First

function First
inc A
add B B B # B *= 2
call Second
return

function Second
inc A
inc B
lt z A D
condcall First
return

#    F S F S F S F  S  F  S
# A: 1 2 3 4 5 6 7  8  9  10
# B: 0 1 2 3 6 7 14 15 30 31
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<Cpu> cpu = newCpu({});
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
		{
			CAGE_TESTCASE("verify");
			const auto rs = cpu->registers();
			CAGE_TEST(rs.size() == 26);
			CAGE_TEST(rs[0] == 10);
			CAGE_TEST(rs[1] == 31);
		}
	}

	{
		CAGE_TESTCASE("leaving function without return terminates");
		constexpr const char source[] = R"asm(
call NoReturn
function NoReturn
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<Cpu> cpu = newCpu({});
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			CAGE_TEST_THROWN(cpu->run());
			CAGE_TEST(cpu->state() == CpuStateEnum::Terminated);
		}
	}

	{
		CAGE_TESTCASE("no function to return from");
		constexpr const char source[] = R"asm(
return
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<Cpu> cpu = newCpu({});
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			CAGE_TEST_THROWN(cpu->run());
			CAGE_TEST(cpu->state() == CpuStateEnum::Terminated);
		}
	}

	{
		CAGE_TESTCASE("stack overflow");
		constexpr const char source[] = R"asm(
call Bottomless
function Bottomless
call Bottomless
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<Cpu> cpu = newCpu({});
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			CAGE_TEST_THROWN(cpu->run());
			CAGE_TEST(cpu->state() == CpuStateEnum::Terminated);
		}
	}

	{
		CAGE_TESTCASE("labels are scoped within function");
		constexpr const char source[] = R"asm(
call First
set B 2

function First
jump Start
label Start
set A 1
return

function Second
jump Start
label Start
set A 2
return
)asm";
		Holder<Program> program = newCompiler()->compile(source);
		Holder<Cpu> cpu = newCpu({});
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
		{
			CAGE_TESTCASE("verify");
			const auto rs = cpu->registers();
			CAGE_TEST(rs.size() == 26);
			CAGE_TEST(rs[0] == 1);
			CAGE_TEST(rs[1] == 2);
		}
	}
}

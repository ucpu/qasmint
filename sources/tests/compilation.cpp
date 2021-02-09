#include <cage-core/math.h>

#include "main.h"

void testCompilation()
{
	CAGE_TESTCASE("compilation");

	{
		CAGE_TESTCASE("empty program");
		Holder<Program> program = newCompiler()->compile("");
		CAGE_TEST(program->instructionsCount() > 0 && program->instructionsCount() < 10);
		{
			CAGE_TESTCASE("run");
			Holder<Cpu> cpu = newCpu({});
			cpu->program(+program);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
	}

	{
		CAGE_TESTCASE("invalid character 1");
		CAGE_TEST_THROWN(newCompiler()->compile("ß"));
	}

	{
		CAGE_TESTCASE("invalid character 2");
		constexpr const char source[] = R"asm(
set A 5
set B ß
set C 7
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("invalid character 3");
		constexpr const char source[] = R"asm(
set A 5
set B 6 # hey ß
set C 7
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("unknown instruction");
		constexpr const char source[] = R"asm(
asdfg
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("missing parameter");
		constexpr const char source[] = R"asm(
set
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("superfluous parameter");
		constexpr const char source[] = R"asm(
set A 5 13
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("invalid register name");
		constexpr const char source[] = R"asm(
set 5 5
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("address specifier is forbidden here");
		constexpr const char source[] = R"asm(
pop A SA@13
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("structure name too short");
		constexpr const char source[] = R"asm(
pop A S
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("structure name too long");
		constexpr const char source[] = R"asm(
pop A SSS
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("invalid literal");
		constexpr const char source[] = R"asm(
set A blah
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("invalid structure name");
		constexpr const char source[] = R"asm(
swap BS CS
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("invalid address 1");
		constexpr const char source[] = R"asm(
load A MA@-5
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("invalid address 2");
		constexpr const char source[] = R"asm(
load A MA@G
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("invalid address 3");
		constexpr const char source[] = R"asm(
load A 13@MA
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("indload requires memory");
		constexpr const char source[] = R"asm(
indload A SA
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("indstore requires memory");
		constexpr const char source[] = R"asm(
indstore SA A
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("pop requires stack");
		constexpr const char source[] = R"asm(
pop A QA
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("push requires stack");
		constexpr const char source[] = R"asm(
push QA A
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("dequeue requires queue");
		constexpr const char source[] = R"asm(
dequeue A SA
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("enqueue requires queue");
		constexpr const char source[] = R"asm(
enqueue SA A
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("left requires tape");
		constexpr const char source[] = R"asm(
left SA
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("right requires tape");
		constexpr const char source[] = R"asm(
right SA
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("center requires tape");
		constexpr const char source[] = R"asm(
center SA
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("swap requires structures of same type");
		constexpr const char source[] = R"asm(
swap SA QB
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("invalid label name");
		constexpr const char source[] = R"asm(
label AA
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("label not found");
		constexpr const char source[] = R"asm(
jump InTheHole
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("label not unique");
		constexpr const char source[] = R"asm(
label First
label First
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("function name not unique");
		constexpr const char source[] = R"asm(
function First
function First
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("labels are scoped within function 1");
		constexpr const char source[] = R"asm(
jump Start
function First
label Start
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}

	{
		CAGE_TESTCASE("labels are scoped within function 1");
		constexpr const char source[] = R"asm(
label Start
function First
jump Start
)asm";
		CAGE_TEST_THROWN(newCompiler()->compile(source));
	}
}

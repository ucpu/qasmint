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
# unsigned integer instructions
set A 42
set B 13
add C A B
sub D A B
mul E A B
div F A B

# floating point instructions
fset G 42.0
fset H 13.0
fadd I G H
fsub J G H
fmul K G H
fdiv L G H

# signed integer instructions
iset M 42
iset N -13
iadd O M N
isub P M N
imul Q M N
idiv R M N
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
			CAGE_TEST(rs[0] == uint32(42));
			CAGE_TEST(rs[1] == uint32(13));
			CAGE_TEST(rs[2] == uint32(42) + uint32(13));
			CAGE_TEST(rs[3] == uint32(42) - uint32(13));
			CAGE_TEST(rs[4] == uint32(42) * uint32(13));
			CAGE_TEST(rs[5] == uint32(42) / uint32(13));
			ftest(rs[6], real(42));
			ftest(rs[7], real(13));
			ftest(rs[8], real(42) + real(13));
			ftest(rs[9], real(42) - real(13));
			ftest(rs[10], real(42) * real(13));
			ftest(rs[11], real(42) / real(13));
			CAGE_TEST(sint32(rs[12]) == sint32(42));
			CAGE_TEST(sint32(rs[13]) == sint32(-13));
			CAGE_TEST(sint32(rs[14]) == sint32(42) + sint32(-13));
			CAGE_TEST(sint32(rs[15]) == sint32(42) - sint32(-13));
			CAGE_TEST(sint32(rs[16]) == sint32(42) * sint32(-13));
			CAGE_TEST(sint32(rs[17]) == sint32(42) / sint32(-13));
		}
		cpu->program(nullptr);
		CAGE_TEST(cpu->state() == CpuStateEnum::None);
	}

	{
		CAGE_TESTCASE("logic");
		constexpr const char source[] = R"asm(
set A 10
set B 0
and C A B # 0
and D A A # 1
or  E A B # 1
or  F B B # 0
xor G A B # 1
xor H A A # 0
not I B   # 1
set J 42
inv J     # 0
set K 0
inv K     # 1
set A 1
set L 42  # 101010
shl L L A # 1010100 = 84
set M 42
shr M M A # 10101 = 21
set N 43  # 101011
rol N N A # 1010110 = 86
set O 43
ror O O A # 10000000000000000000000000010101 = 2147483669
set  P 42  # 101010
set  Q 13  # 001101
band R P Q # 001000 = 8
bor  S P Q # 101111 = 47
bxor T P Q # 100111 = 39
bnot U P   # something big
copy V P
binv     V # keep the spaces before the register name to test it
)asm";
		Holder<BinaryProgram> program = compiler->compile(source);
		CAGE_TEST(cpu->state() == CpuStateEnum::None);
		{
			CAGE_TESTCASE("run");
			cpu->program(+program);
			CAGE_TEST(cpu->state() == CpuStateEnum::Initialized);
			cpu->run();
			CAGE_TEST(cpu->state() == CpuStateEnum::Finished);
		}
		{
			CAGE_TESTCASE("verify");
			const auto rs = cpu->explicitRegisters();
			CAGE_TEST(rs.size() == 26);
			CAGE_TEST(rs[0] == 1);
			CAGE_TEST(rs[1] == 0);
			CAGE_TEST(rs[2] == 0);
			CAGE_TEST(rs[3] == 1);
			CAGE_TEST(rs[4] == 1);
			CAGE_TEST(rs[5] == 0);
			CAGE_TEST(rs[6] == 1);
			CAGE_TEST(rs[7] == 0);
			CAGE_TEST(rs[8] == 1);
			CAGE_TEST(rs[9] == 0);
			CAGE_TEST(rs[10] == 1);
			CAGE_TEST(rs[11] == 84);
			CAGE_TEST(rs[12] == 21);
			CAGE_TEST(rs[13] == 86);
			CAGE_TEST(rs[14] == 2147483669);
			CAGE_TEST(rs[15] == 42);
			CAGE_TEST(rs[16] == 13);
			CAGE_TEST(rs[17] == 8);
			CAGE_TEST(rs[18] == 47);
			CAGE_TEST(rs[19] == 39);
			CAGE_TEST(rs[20] == ~uint32(42));
			CAGE_TEST(rs[21] == ~uint32(42));
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

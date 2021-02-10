#ifndef qasm_h_sd5f4ghsed4rg
#define qasm_h_sd5f4ghsed4rg

#include "cage-core/core.h"

namespace qasm
{
	using namespace cage;

	struct Program : private Immovable
	{
		uint32 instructionsCount() const;
		detail::StringBase<20> functionName(uint32 index) const;
		PointerRange<const char> sourceCode() const;
		string sourceCodeLine(uint32 index) const;
	};

	struct Compiler : private Immovable
	{
		Holder<Program> compile(PointerRange<const char> sourceCode);
	};

	Holder<Compiler> newCompiler();

	enum class CpuStateEnum
	{
		None,
		Initialized,
		Running,
		Finished,
		Interrupted,
		Terminated,
	};

	struct Cpu : private Immovable
	{
		void program(const Program *binary); // the program must outlive the cpu
		void reinitialize();
		void run();
		void step();
		void interrupt(); // can be called from any thread
		void terminate();

		CpuStateEnum state() const;

		PointerRange<const uint32> implicitRegisters() const;
		PointerRange<const uint32> registers() const;
		void registers(PointerRange<uint32> data); // valid in Initialized state only
		Holder<PointerRange<const uint32>> stack(uint32 index) const;
		Holder<PointerRange<const uint32>> queue(uint32 index) const;
		Holder<PointerRange<const uint32>> tape(uint32 index) const;
		PointerRange<const uint32> memory(uint32 index) const;
		void memory(uint32 index, PointerRange<const uint32> data); // valid in Initialized state only

		PointerRange<const uint32> callstack() const;
		uint32 functionIndex() const;
		uint32 sourceLine() const;
		uint64 stepIndex() const;
	};

	struct CpuLimitsConfig
	{
		uint32 memoryCapacity[26] = { 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000, 1000000 }; // bummer
		bool memoryReadOnly[26] = {};
		uint32 memoriesCount = 4;
		uint32 stackCapacity = 1000000;
		uint32 stacksCount = 4;
		uint32 queueCapacity = 1000000;
		uint32 queuesCount = 4;
		uint32 tapeCapacity = 1000000;
		uint32 tapesCount = 4;
		uint32 callstackCapacity = 1000;
	};

	CpuLimitsConfig limitsFromIni(Ini *ini, const CpuLimitsConfig &defaults = {});
	void limitsToIni(const CpuLimitsConfig &limits, Ini *ini);

	struct CpuCreateConfig
	{
		CpuLimitsConfig limits;
		Delegate<bool(string &)> input;
		Delegate<bool(const string &)> output;
		uint64 interruptPeriod = m; // the cpu is automatically interrupted every N-th step
	};

	Holder<Cpu> newCpu(const CpuCreateConfig &config);
}

#endif // qasm_h_sd5f4ghsed4rg

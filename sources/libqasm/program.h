#include <qasm/qasm.h>

namespace qasm
{
	enum class InstructionEnum : uint16
	{
		nop = 0,     //

		// register
		reset,       // R
		set,         // R uint32
		iset,        // R sint32
		fset,        // R float
		copy,        // R R
		condrst,     // R
		condset,     // R uint32
		condiset,    // R sint32
		condfset,    // R float
		condcpy,     // R R
		indcpy,      //

		// arithmetic
		add,         // R R R
		sub,         // R R R
		mul,         // R R R
		div,         // R R R
		mod,         // R R R
		inc,         // R
		dec,         // R
		iadd,        // R R R
		isub,        // R R R
		imul,        // R R R
		idiv,        // R R R
		imod,        // R R R
		iinc,        // R
		idec,        // R
		iabs,        // R R
		fadd,        // R R R
		fsub,        // R R R
		fmul,        // R R R
		fdiv,        // R R R
		fpow,        // R R R
		fatan2,      // R R R
		fabs,        // R R
		fsqrt,       // R R
		flog,        // R R
		fsin,        // R R
		fcos,        // R R
		ftan,        // R R
		fasin,       // R R
		facos,       // R R
		fatan,       // R R
		ffloor,      // R R
		fround,      // R R
		fceil,       // R R
		s2f,         // R R
		u2f,         // R R
		f2s,         // R R
		f2u,         // R R

		// logic
		and_,        // R R R
		or_,         // R R R
		xor_,        // R R R
		not_,        // R R
		inv,         // R
		shl,         // R R R
		shr,         // R R R
		rol,         // R R R
		ror,         // R R R
		band,        // R R R
		bor,         // R R R
		bxor,        // R R R
		bnot,        // R R
		binv,        // R

		// conditions
		eq,          // R R R
		neq,         // R R R
		lt,          // R R R
		gt,          // R R R
		lte,         // R R R
		gte,         // R R R
		ieq,         // R R R
		ineq,        // R R R
		ilt,         // R R R
		igt,         // R R R
		ilte,        // R R R
		igte,        // R R R
		feq,         // R R R
		fneq,        // R R R
		flt,         // R R R
		fgt,         // R R R
		flte,        // R R R
		fgte,        // R R R
		fisnan,      // R R
		fisinf,      // R R
		fisfin,      // R R
		fisnorm,     // R R
		test,        // R R

		// stack
		sload,       // R S
		sstore,      // S R
		pop,         // R S
		push,        // S R
		sswap,       // S S
		indsswap,    //
		sstat,       // S
		indsstat,    //

		// queue
		qload,       // R Q
		qstore,      // Q R
		dequeue,     // R Q
		enqueue,     // Q R
		qswap,       // Q Q
		indqswap,    //
		qstat,       // Q
		indqstat,    //

		// tape
		tload,       // R T
		tstore,      // T R
		left,        // T
		right,       // T
		center,      // T
		tswap,       // T T
		indtswap,    //
		tstat,       // T
		indtstat,    //

		// memory
		mload,       // R M uint32
		indload,     // R M
		indindload,  // R
		mstore,      // M uint32 R
		indstore,    // M R
		indindstore, // R
		mswap,       // M M
		indmswap,    //
		mstat,       // M
		indmstat,    //

		// jumps
		jump,        // uint32
		condjmp,     // uint32

		// functions
		call,        // uint32
		condcall,    // uint32
		return_,     //
		condreturn,  //

		// input/output
		rstat,
		wstat,
		read,
		iread,
		fread,
		cread,
		readln,
		rreset,
		rclear,
		write,
		iwrite,
		fwrite,
		cwrite,
		writeln,
		wreset,
		wclear,
		rwswap,

		// miscellaneous
		timer,       // R R
		rdseedany,   //
		rdseed,      // R R R R
		rand,        // R
		irand,       // R
		frand,       // R
		profiling,   // bool
		tracing,     // bool
		breakpoint,  //
		terminate,   //
		disabled,    //
		exit,        //
	};

	struct ProgramImpl : public BinaryProgram
	{
		Holder<PointerRange<InstructionEnum>> instructions;
		Holder<PointerRange<uint32>> paramsOffsets;
		Holder<PointerRange<uint32>> sourceLines;
		Holder<PointerRange<uint32>> functionIndices;

		Holder<PointerRange<const char>> params;
		Holder<PointerRange<detail::StringBase<20>>> functionNames;

		Holder<PointerRange<const char>> sourceCode; // copy of source code
		uint32 linesCount = 0;
	};
}

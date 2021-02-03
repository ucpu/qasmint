#ifndef qasm_h_sd5f4ghsed4rg
#define qasm_h_sd5f4ghsed4rg

#include "cage-core/core.h"

namespace qasm
{
	using namespace cage;

	struct QasmCompiler : private Immovable
	{
	};

	Holder<QasmCompiler> newQasmCompiler();

	struct QasmCpu : private Immovable
	{
	};

	Holder<QasmCpu> newQasmCpu();
}

#endif // qasm_h_sd5f4ghsed4rg

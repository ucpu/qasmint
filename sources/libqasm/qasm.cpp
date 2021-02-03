#include "qasm/qasm.h"

namespace qasm
{
	struct QasmCompilerImpl : public QasmCompiler
	{
	};

	Holder<QasmCompiler> newQasmCompiler()
	{
		return detail::systemArena().createImpl<QasmCompiler, QasmCompilerImpl>();
	}

	struct QasmCpuImpl : public QasmCpu
	{
	};

	Holder<QasmCpu> newQasmCpu()
	{
		return detail::systemArena().createImpl<QasmCpu, QasmCpuImpl>();
	}
}

#include <qasm/qasm.h>

using namespace qasm;

void testBasics()
{
	CAGE_LOG(SeverityEnum::Info, "testcase", "basics");
	Holder<QasmCompiler> compiler = newQasmCompiler();
	Holder<QasmCpu> cpu = newQasmCpu();
	// todo
}

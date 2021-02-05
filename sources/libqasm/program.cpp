#include "program.h"

namespace qasm
{
	detail::StringBase<20> BinaryProgram::functionName(uint32 index) const
	{
		const ProgramImpl *impl = (const ProgramImpl *)this;
		if (index == m)
			return "";
		if (index >= impl->functionNames.size())
			CAGE_THROW_ERROR(Exception, "program function index out of range");
		return impl->functionNames[index];
	}

	PointerRange<const char> BinaryProgram::sourceCode() const
	{
		const ProgramImpl *impl = (const ProgramImpl *)this;
		return impl->sourceCode;
	}
}

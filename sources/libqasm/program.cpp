#include "program.h"

namespace qasm
{
	uint32 Program::instructionsCount() const
	{
		const ProgramImpl *impl = (const ProgramImpl *)this;
		return numeric_cast<uint32>(impl->instructions.size());
	}

	detail::StringBase<20> Program::functionName(uint32 index) const
	{
		const ProgramImpl *impl = (const ProgramImpl *)this;
		if (index == m)
			return "";
		if (index >= impl->functionNames.size())
			CAGE_THROW_ERROR(Exception, "program function index out of range");
		return impl->functionNames[index];
	}

	PointerRange<const char> Program::sourceCode() const
	{
		const ProgramImpl *impl = (const ProgramImpl *)this;
		return impl->sourceCode;
	}
}

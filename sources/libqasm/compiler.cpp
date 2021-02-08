#include <cage-core/lineReader.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/string.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>
#include <cage-core/macros.h>

#include "program.h"

#include <map>

namespace qasm
{
	namespace
	{
		using Name = detail::StringBase<20>;

		struct Label
		{
			Name function;
			Name label;
		};

		bool operator < (const Label &l, const Label &r)
		{
			if (l.function == r.function)
				return l.label < r.label;
			return l.function < r.function;
		}

		struct LabelReplacement : public Label
		{
			uint32 paramsOffset = m;
			uint32 sourceLine = m; // for error reporting
		};

		string decomment(const string &line)
		{
			const uint32 commentStart = find(line, "#");
			for (uint32 pos = 0; pos < line.length(); pos++)
			{
				const char c = line[pos];
				if (c >= 'a' && c <= 'z')
					continue;
				if (c >= 'A' && c <= 'Z')
					continue;
				if (c >= '0' && c <= '9')
					continue;
				switch (c)
				{
				case ' ':
				case '-':
				case '+':
				case '.':
				case '_':
				case '@':
				case '#':
					continue;
				}
				if (pos >= commentStart)
				{
					switch (c)
					{
					case '*':
					case '/':
					case ',':
					case '(':
					case ')':
					case '<':
					case '>':
					case '=':
					case '?':
					case '!':
					case ':':
					case ';':
						continue;
					}
				}
				CAGE_THROW_ERROR(Exception, "invalid character");
			}
			string s = trim(subString(line, 0, commentStart));
			s = replace(s, "\t", " ");
			s = replace(s, "  ", " ");
			return s;
		}

		void validateName(const string &name)
		{
			if (name.length() < 3 || name.length() > 20)
				CAGE_THROW_ERROR(Exception, "function/label name has invalid length");
			for (uint32 pos = 0; pos < name.length(); pos++)
			{
				const char c = name[pos];
				if (c >= 'a' && c <= 'z')
					continue;
				if (c >= 'A' && c <= 'Z')
					continue;
				if (c >= '0' && c <= '9')
					continue;
				CAGE_THROW_ERROR(Exception, "invalid character in function/label name");
			}
			if (name[0] < 'A' || name[0] > 'Z')
				CAGE_THROW_ERROR(Exception, "function/label name must start with capital letter");
		}

		uint8 getRegister(string &line)
		{
			string n = split(line);
			if (n.empty())
				CAGE_THROW_ERROR(Exception, "missing register name parameter");
			if (n.length() != 1)
				CAGE_THROW_ERROR(Exception, "register name too long");
			if (n[0] >= 'A' && n[0] <= 'Z')
				return n[0] - 'A';
			if (n[0] >= 'a' && n[0] <= 'z')
				return n[0] - 'a' + 26;
			CAGE_THROW_ERROR(Exception, "invalid character in register name");
		}

		void getStructure(string &line, uint8 &type, uint8 &index, uint32 &address)
		{
			string a = split(line);
			string n = split(a, "@");
			if (a.empty())
				address = 0;
			else
				address = toUint32(a);
			if (n.empty())
				CAGE_THROW_ERROR(Exception, "missing structure name parameter");
			if (n.length() < 2)
				CAGE_THROW_ERROR(Exception, "structure name too short");
			if (n.length() > 2)
				CAGE_THROW_ERROR(Exception, "structure name too long");
			if (n[1] < 'A' || n[1] > 'Z')
				CAGE_THROW_ERROR(Exception, "invalid character in structure instance name");
			index = n[1] - 'A';
			switch (n[0])
			{
			case 'S': type = 0; break;
			case 'Q': type = 1; break;
			case 'T': type = 2; break;
			case 'M': type = 3; break;
			default:
				CAGE_THROW_ERROR(Exception, "invalid character in structure type name");
			}
		}

		void getStructure(string &line, uint8 &type, uint8 &index)
		{
			uint32 address;
			getStructure(line, type, index, address);
			if (address != 0)
				CAGE_THROW_ERROR(Exception, "address specifier is forbidden here");
		}
	}

	struct CompilerImpl : public Compiler
	{
		PointerRangeHolder<InstructionEnum> instructions;
		PointerRangeHolder<uint32> paramsOffsets;
		PointerRangeHolder<uint32> sourceLines;
		PointerRangeHolder<uint32> functionIndices;
		MemoryBuffer paramsBuffer;
		Serializer params = Serializer(paramsBuffer);

		std::vector<LabelReplacement> labelsReplacements; // which positions in parameters should be updated to what position of label in a function
		std::map<Label, uint32> labelNameToInstruction;
		std::map<Name, uint32> functionNameToIndex;
		std::map<uint32, Name> functionIndexToName;
		uint32 currentFunctionIndex = 0;
		uint32 currentSourceLine = 0;

		void insert(InstructionEnum instruction)
		{
			instructions.push_back(instruction);
			paramsOffsets.push_back(numeric_cast<uint32>(paramsBuffer.size()));
			sourceLines.push_back(currentSourceLine);
			functionIndices.push_back(currentFunctionIndex);
		}

		void scopeExit()
		{
			// leaving function without return terminates the program
			// leaving program scope is successful exit
			insert(currentFunctionIndex == 0 ? InstructionEnum::exit : InstructionEnum::unreachable);
		}

		void processLabel(string &line)
		{
			validateName(line);
			Label label;
			label.label = split(line);
			label.function = functionIndexToName.at(currentFunctionIndex);
			if (labelNameToInstruction.count(label))
				CAGE_THROW_ERROR(Exception, "label name is not unique");
			labelNameToInstruction[label] = numeric_cast<uint32>(instructions.size());
		}

		void processJump(string &line, bool condition)
		{
			validateName(line);
			LabelReplacement label;
			label.label = split(line);
			label.function = functionIndexToName.at(currentFunctionIndex);
			insert(condition ? InstructionEnum::condjmp : InstructionEnum::jump);
			label.paramsOffset = numeric_cast<uint32>(paramsBuffer.size());
			label.sourceLine = currentSourceLine;
			labelsReplacements.push_back(label);
			params << uint32(m); // this value will be replaced by address of the label after parsing the source code has finished
		}

		void processCondskip(string &line)
		{
			CAGE_THROW_ERROR(NotImplemented, "not yet implemented");
		}

		void processFunction(string &line)
		{
			scopeExit();
			validateName(line);
			Label label;
			label.label = label.function = split(line);
			if (labelNameToInstruction.count(label))
				CAGE_THROW_ERROR(Exception, "function name is not unique");
			labelNameToInstruction[label] = numeric_cast<uint32>(instructions.size());
			currentFunctionIndex++;
			functionNameToIndex[label.function] = currentFunctionIndex;
			functionIndexToName[currentFunctionIndex] = label.function;
		}

		void processCall(string &line, bool condition)
		{
			validateName(line);
			LabelReplacement label;
			label.label = label.function = split(line);
			insert(condition ? InstructionEnum::condcall : InstructionEnum::call);
			label.paramsOffset = numeric_cast<uint32>(paramsBuffer.size());
			label.sourceLine = currentSourceLine;
			labelsReplacements.push_back(label);
			params << uint32(m); // this value will be replaced by address of the label after parsing the source code has finished
		}

		void processReturn(string &line, bool condition)
		{
			insert(condition ? InstructionEnum::condreturn : InstructionEnum::return_);
		}

		template<InstructionEnum Opcode, uint32 Registers>
		bool matchSimpleInstruction(const char *Name, const string &instruction, string &line)
		{
			if (instruction != string(Name))
				return false;
			insert(Opcode);
			for (uint32 i = 0; i < Registers; i++)
				params << getRegister(line);
			return true;
		}

		void processLine(string &line)
		{
			const string instruction = split(line);

#define MatchSimpleInstruction_0(Name) || matchSimpleInstruction<InstructionEnum::Name, 0>(CAGE_STRINGIZE(Name), instruction, line)
#define MatchSimpleInstruction_1(Name) || matchSimpleInstruction<InstructionEnum::Name, 1>(CAGE_STRINGIZE(Name), instruction, line)
#define MatchSimpleInstruction_2(Name) || matchSimpleInstruction<InstructionEnum::Name, 2>(CAGE_STRINGIZE(Name), instruction, line)
#define MatchSimpleInstruction_3(Name) || matchSimpleInstruction<InstructionEnum::Name, 3>(CAGE_STRINGIZE(Name), instruction, line)

			// registers
			if (false
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(MatchSimpleInstruction_0, indcpy))
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(MatchSimpleInstruction_1, reset, condrst))
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(MatchSimpleInstruction_2, copy, condcpy))
				)
				return;
			if (instruction == "set")
			{
				insert(InstructionEnum::set);
				params << getRegister(line);
				params << toUint32(split(line));
				return;
			}
			if (instruction == "iset")
			{
				insert(InstructionEnum::iset);
				params << getRegister(line);
				params << toSint32(split(line));
				return;
			}
			if (instruction == "fset")
			{
				insert(InstructionEnum::fset);
				params << getRegister(line);
				params << toFloat(split(line));
				return;
			}
			if (instruction == "condset")
			{
				insert(InstructionEnum::condset);
				params << getRegister(line);
				params << toUint32(split(line));
				return;
			}
			if (instruction == "condiset")
			{
				insert(InstructionEnum::condiset);
				params << getRegister(line);
				params << toSint32(split(line));
				return;
			}
			if (instruction == "condfset")
			{
				insert(InstructionEnum::condfset);
				params << getRegister(line);
				params << toFloat(split(line));
				return;
			}

			// arithmetics
			if (false
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(MatchSimpleInstruction_1, inc, dec, iinc, idec))
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(MatchSimpleInstruction_2, iabs, fabs, fsqrt, flog, fsin, fcos, ftan, fasin, facos, fatan, ffloor, fround, fceil, s2f, u2f, f2s, f2u))
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(MatchSimpleInstruction_3, add, sub, mul, div, mod, iadd, isub, imul, idiv, imod, fadd, fsub, fmul, fdiv, fpow, fatan2))
				)
				return;

			// logic
			if (false
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(MatchSimpleInstruction_1, inv, binv))
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(MatchSimpleInstruction_3, shl, shr, rol, ror, band, bor, bxor))
				|| matchSimpleInstruction<InstructionEnum::not_, 2>("not", instruction, line)
				|| matchSimpleInstruction<InstructionEnum::bnot, 2>("bnot", instruction, line)
				|| matchSimpleInstruction<InstructionEnum::and_, 3>("and", instruction, line)
				|| matchSimpleInstruction<InstructionEnum::or_, 3>("or", instruction, line)
				|| matchSimpleInstruction<InstructionEnum::xor_, 3>("xor", instruction, line)
				)
				return;

			// comparisons
			if (false
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(MatchSimpleInstruction_2, fisnan, fisinf, fisfin, fisnorm, test))
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(MatchSimpleInstruction_3, eq, neq, lt, gt, lte, gte, ieq, ineq, ilt, igt, ilte, igte, feq, fneq, flt, fgt, flte, fgte))
				)
				return;

			// structures
			if (instruction == "load")
			{
				uint8 dst = getRegister(line);
				uint8 type, index;
				uint32 address;
				getStructure(line, type, index, address);
				switch (type)
				{
				case 0:
					insert(InstructionEnum::sload);
					params << dst << index;
					break;
				case 1:
					insert(InstructionEnum::qload);
					params << dst << index;
					break;
				case 2:
					insert(InstructionEnum::tload);
					params << dst << index;
					break;
				case 3:
					insert(InstructionEnum::mload);
					params << dst << index << address;
					break;
				}
				return;
			}
			if (instruction == "store")
			{
				uint8 type, index;
				uint32 address;
				getStructure(line, type, index, address);
				uint8 src = getRegister(line);
				switch (type)
				{
				case 0:
					insert(InstructionEnum::sstore);
					params << index << src;
					break;
				case 1:
					insert(InstructionEnum::qstore);
					params << index << src;
					break;
				case 2:
					insert(InstructionEnum::tstore);
					params << index << src;
					break;
				case 3:
					insert(InstructionEnum::mstore);
					params << index << address << src;
					break;
				}
				return;
			}
			if (instruction == "indload")
			{
				uint8 dst = getRegister(line);
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 3)
					CAGE_THROW_ERROR(Exception, "indload requires memory pool");
				insert(InstructionEnum::indload);
				params << dst << index;
				return;
			}
			if (instruction == "indstore")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 3)
					CAGE_THROW_ERROR(Exception, "indstore requires memory pool");
				uint8 src = getRegister(line);
				insert(InstructionEnum::indstore);
				params << index << src;
				return;
			}
			if (instruction == "indindload")
			{
				insert(InstructionEnum::indindload);
				params << getRegister(line);
				return;
			}
			if (instruction == "indindstore")
			{
				insert(InstructionEnum::indindstore);
				params << getRegister(line);
				return;
			}
			if (instruction == "pop")
			{
				uint8 dst = getRegister(line);
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 0)
					CAGE_THROW_ERROR(Exception, "pop requires stack");
				insert(InstructionEnum::pop);
				params << dst << index;
				return;
			}
			if (instruction == "push")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 0)
					CAGE_THROW_ERROR(Exception, "push requires stack");
				uint8 src = getRegister(line);
				insert(InstructionEnum::push);
				params << index << src;
				return;
			}
			if (instruction == "dequeue")
			{
				uint8 dst = getRegister(line);
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 1)
					CAGE_THROW_ERROR(Exception, "dequeue requires queue");
				insert(InstructionEnum::dequeue);
				params << dst << index;
				return;
			}
			if (instruction == "enqueue")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 1)
					CAGE_THROW_ERROR(Exception, "enqueue requires queue");
				uint8 src = getRegister(line);
				insert(InstructionEnum::enqueue);
				params << index << src;
				return;
			}
			if (instruction == "left")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 2)
					CAGE_THROW_ERROR(Exception, "left requires tape");
				insert(InstructionEnum::left);
				params << index;
				return;
			}
			if (instruction == "right")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 2)
					CAGE_THROW_ERROR(Exception, "right requires tape");
				insert(InstructionEnum::right);
				params << index;
				return;
			}
			if (instruction == "center")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 2)
					CAGE_THROW_ERROR(Exception, "center requires tape");
				insert(InstructionEnum::center);
				params << index;
				return;
			}
			if (instruction == "swap")
			{
				uint8 type1, index1, type2, index2;
				getStructure(line, type1, index1);
				getStructure(line, type2, index2);
				if (type1 != type2)
					CAGE_THROW_ERROR(Exception, "swap requires structures of same type");
				switch (type1)
				{
				case 0: insert(InstructionEnum::sswap); break;
				case 1: insert(InstructionEnum::qswap); break;
				case 2: insert(InstructionEnum::tswap); break;
				case 3: insert(InstructionEnum::mswap); break;
				}
				params << index1 << index2;
				return;
			}
			if (instruction == "indswap")
			{
				uint8 type1, index1, type2, index2;
				getStructure(line, type1, index1);
				getStructure(line, type2, index2);
				if (type1 != type2)
					CAGE_THROW_ERROR(Exception, "indswap requires structures of same type");
				if (index1 != 0 || index2 != 0)
					CAGE_THROW_ERROR(Exception, "indswap requires A instance of structure to denote the type");
				switch (type1)
				{
				case 0: insert(InstructionEnum::indsswap); break;
				case 1: insert(InstructionEnum::indqswap); break;
				case 2: insert(InstructionEnum::indtswap); break;
				case 3: insert(InstructionEnum::indmswap); break;
				}
				return;
			}
			if (instruction == "stat")
			{
				uint8 type, index;
				getStructure(line, type, index);
				switch (type)
				{
				case 0: insert(InstructionEnum::sstat); break;
				case 1: insert(InstructionEnum::qstat); break;
				case 2: insert(InstructionEnum::tstat); break;
				case 3: insert(InstructionEnum::mstat); break;
				}
				params << index;
				return;
			}
			if (instruction == "indstat")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (index != 0)
					CAGE_THROW_ERROR(Exception, "indstat requires A instance of structure to denote the type");
				switch (type)
				{
				case 0: insert(InstructionEnum::indsstat); break;
				case 1: insert(InstructionEnum::indqstat); break;
				case 2: insert(InstructionEnum::indtstat); break;
				case 3: insert(InstructionEnum::indmstat); break;
				}
				return;
			}

			// jumps
			if (instruction == "label")
				return processLabel(line);
			if (instruction == "jump")
				return processJump(line, false);
			if (instruction == "condjmp")
				return processJump(line, true);
			if (instruction == "condskip")
				return processCondskip(line);

			// functions
			if (instruction == "function")
				return processFunction(line);
			if (instruction == "call")
				return processCall(line, false);
			if (instruction == "condcall")
				return processCall(line, true);
			if (instruction == "return")
				return processReturn(line, false);
			if (instruction == "condreturn")
				return processReturn(line, true);

			// input/output
			if (false
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(MatchSimpleInstruction_0, rstat, wstat, readln, rreset, rclear, writeln, wreset, wclear, rwswap))
				CAGE_EVAL_SMALL(CAGE_EXPAND_ARGS(MatchSimpleInstruction_1, read, iread, fread, cread, write, iwrite, fwrite, cwrite))
				)
				return;

			CAGE_THROW_ERROR(Exception, "unknown instruction");
		}

		void processLabelReplacements()
		{
			CAGE_ASSERT(functionIndexToName.size() == functionNameToIndex.size());
			for (const LabelReplacement &label : labelsReplacements)
			{
				uint32 &p = *(uint32 *)(paramsBuffer.data() + label.paramsOffset);
				CAGE_ASSERT(p == m);
				auto it = labelNameToInstruction.find(label);
				if (it == labelNameToInstruction.end())
				{
					CAGE_LOG_THROW(stringizer() + "function: " + label.function);
					CAGE_LOG_THROW(stringizer() + "label: " + label.label);
					CAGE_LOG_THROW(stringizer() + "line number: " + (label.sourceLine + 1));
					CAGE_THROW_ERROR(Exception, "label not found");
				}
				p = it->second;
			}
		}

		Holder<Program> compile(PointerRange<const char> sourceCode)
		{
			instructions.clear();
			paramsOffsets.clear();
			sourceLines.clear();
			functionIndices.clear();
			paramsBuffer.free();
			params = Serializer(paramsBuffer);

			labelsReplacements.clear();
			labelNameToInstruction.clear();
			functionNameToIndex.clear();
			functionNameToIndex[""] = 0;
			functionIndexToName.clear();
			functionIndexToName[0] = "";
			currentFunctionIndex = 0;
			currentSourceLine = 0;

			Holder<LineReader> lines = newLineReader(sourceCode);
			for (string fullLine; lines->readLine(fullLine); currentSourceLine++)
			{
				try
				{
					string line = decomment(fullLine);
					if (line.empty())
						continue;
					processLine(line);
					if (!line.empty())
						CAGE_THROW_ERROR(Exception, "superfluous argument");
				}
				catch (...)
				{
					CAGE_LOG_THROW(stringizer() + "line number: " + (currentSourceLine + 1));
					CAGE_LOG_THROW(fullLine);
					throw;
				}
			}
			scopeExit();
			processLabelReplacements();

			CAGE_ASSERT(instructions.size() == paramsOffsets.size());
			CAGE_ASSERT(instructions.size() == sourceLines.size());
			CAGE_ASSERT(instructions.size() == functionIndices.size());

			Holder<ProgramImpl> p = detail::systemArena().createHolder<ProgramImpl>();
			p->sourceCode = PointerRangeHolder<const char>(sourceCode);
			p->instructions = instructions;
			p->paramsOffsets = paramsOffsets;
			p->sourceLines = sourceLines;
			p->functionIndices = functionIndices;
			p->params = PointerRangeHolder<const char>(PointerRange<const char>(paramsBuffer));
			return templates::move(p).cast<Program>();
		}
	};

	Holder<Compiler> newCompiler()
	{
		return detail::systemArena().createImpl<Compiler, CompilerImpl>();
	}

	Holder<Program> Compiler::compile(PointerRange<const char> sourceCode)
	{
		CompilerImpl *impl = (CompilerImpl *)this;
		return impl->compile(sourceCode);
	}
}

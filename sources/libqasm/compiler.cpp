#include <cage-core/lineReader.h>
#include <cage-core/pointerRangeHolder.h>
#include <cage-core/string.h>
#include <cage-core/serialization.h>
#include <cage-core/memoryBuffer.h>

#include "program.h"

#include <map>

namespace qasm
{
	namespace
	{
		struct Label
		{
			detail::StringBase<20> function;
			detail::StringBase<20> label;
		};

		struct LabelReplacement : public Label
		{
			uint32 paramsOffset = m;
			uint32 sourceLine = m;
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
				return n[0] - 'z' + 26;
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
		std::map<Label, uint32> labelsPositions;
		uint32 lastFunction = m;
		uint32 lastLine = 0;

		void insert(InstructionEnum instruction)
		{
			instructions.push_back(instruction);
			paramsOffsets.push_back(numeric_cast<uint32>(paramsBuffer.size()));
			sourceLines.push_back(lastLine);
			functionIndices.push_back(lastFunction);
		}

		void scopeExit()
		{
			// leaving function without return terminates the program
			// leaving program scope is successful exit
			insert(lastFunction == m ? InstructionEnum::exit : InstructionEnum::terminate);
		}

		void processLine(string &line)
		{
			const string instruction = split(line);
			// registers
			if (instruction == "reset")
			{
				insert(InstructionEnum::reset);
				params << getRegister(line);
			}
			else if (instruction == "set")
			{
				insert(InstructionEnum::set);
				params << getRegister(line);
				params << toUint32(split(line));
			}
			else if (instruction == "iset")
			{
				insert(InstructionEnum::iset);
				params << getRegister(line);
				params << toSint32(split(line));
			}
			else if (instruction == "fset")
			{
				insert(InstructionEnum::fset);
				params << getRegister(line);
				params << toFloat(split(line));
			}
			else if (instruction == "copy")
			{
				insert(InstructionEnum::copy);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "condrst")
			{
				insert(InstructionEnum::condrst);
				params << getRegister(line);
			}
			else if (instruction == "condset")
			{
				insert(InstructionEnum::condset);
				params << getRegister(line);
				params << toUint32(split(line));
			}
			else if (instruction == "condiset")
			{
				insert(InstructionEnum::condiset);
				params << getRegister(line);
				params << toSint32(split(line));
			}
			else if (instruction == "condfset")
			{
				insert(InstructionEnum::condfset);
				params << getRegister(line);
				params << toFloat(split(line));
			}
			else if (instruction == "condcpy")
			{
				insert(InstructionEnum::condcpy);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "indcpy")
			{
				insert(InstructionEnum::indcpy);
			}
			// arithmetic
			else if (instruction == "add")
			{
				insert(InstructionEnum::add);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "sub")
			{
				insert(InstructionEnum::sub);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "mul")
			{
				insert(InstructionEnum::mul);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "div")
			{
				insert(InstructionEnum::div);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "mod")
			{
				insert(InstructionEnum::mod);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "inc")
			{
				insert(InstructionEnum::inc);
				params << getRegister(line);
			}
			else if (instruction == "dec")
			{
				insert(InstructionEnum::dec);
				params << getRegister(line);
			}
			else if (instruction == "iadd")
			{
				insert(InstructionEnum::iadd);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "isub")
			{
				insert(InstructionEnum::isub);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "imul")
			{
				insert(InstructionEnum::imul);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "idiv")
			{
				insert(InstructionEnum::idiv);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "imod")
			{
				insert(InstructionEnum::imod);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "iinc")
			{
				insert(InstructionEnum::iinc);
				params << getRegister(line);
			}
			else if (instruction == "idec")
			{
				insert(InstructionEnum::idec);
				params << getRegister(line);
			}
			else if (instruction == "iabs")
			{
				insert(InstructionEnum::iabs);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fadd")
			{
				insert(InstructionEnum::fadd);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fsub")
			{
				insert(InstructionEnum::fsub);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fmul")
			{
				insert(InstructionEnum::fmul);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fdiv")
			{
				insert(InstructionEnum::fdiv);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fpow")
			{
				insert(InstructionEnum::fpow);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fatan2")
			{
				insert(InstructionEnum::fatan2);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fabs")
			{
				insert(InstructionEnum::fabs);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fsqrt")
			{
				insert(InstructionEnum::fsqrt);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "flog")
			{
				insert(InstructionEnum::flog);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fsin")
			{
				insert(InstructionEnum::fsin);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fcos")
			{
				insert(InstructionEnum::fcos);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "ftan")
			{
				insert(InstructionEnum::ftan);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fasin")
			{
				insert(InstructionEnum::fasin);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "facos")
			{
				insert(InstructionEnum::facos);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fatan")
			{
				insert(InstructionEnum::fatan);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "ffloor")
			{
				insert(InstructionEnum::ffloor);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fround")
			{
				insert(InstructionEnum::fround);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fceil")
			{
				insert(InstructionEnum::fceil);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "s2f")
			{
				insert(InstructionEnum::s2f);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "u2f")
			{
				insert(InstructionEnum::u2f);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "f2s")
			{
				insert(InstructionEnum::f2s);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "f2u")
			{
				insert(InstructionEnum::f2u);
				params << getRegister(line);
				params << getRegister(line);
			}
			// logic
			else if (instruction == "and")
			{
				insert(InstructionEnum::and_);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "or")
			{
				insert(InstructionEnum::or_);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "xor")
			{
				insert(InstructionEnum::xor_);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "not")
			{
				insert(InstructionEnum::not_);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "inv")
			{
				insert(InstructionEnum::inv);
				params << getRegister(line);
			}
			else if (instruction == "shl")
			{
				insert(InstructionEnum::shl);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "shr")
			{
				insert(InstructionEnum::shr);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "rol")
			{
				insert(InstructionEnum::rol);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "ror")
			{
				insert(InstructionEnum::ror);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "band")
			{
				insert(InstructionEnum::band);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "bor")
			{
				insert(InstructionEnum::bor);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "bxor")
			{
				insert(InstructionEnum::bxor);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "bnot")
			{
				insert(InstructionEnum::bnot);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "binv")
			{
				insert(InstructionEnum::binv);
				params << getRegister(line);
			}
			// comparisons
			else if (instruction == "eq")
			{
				insert(InstructionEnum::eq);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "neq")
			{
				insert(InstructionEnum::neq);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "lt")
			{
				insert(InstructionEnum::lt);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "gt")
			{
				insert(InstructionEnum::gt);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "lte")
			{
				insert(InstructionEnum::lte);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "gte")
			{
				insert(InstructionEnum::gte);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "ieq")
			{
				insert(InstructionEnum::ieq);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "ineq")
			{
				insert(InstructionEnum::ineq);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "ilt")
			{
				insert(InstructionEnum::ilt);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "igt")
			{
				insert(InstructionEnum::igt);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "ilte")
			{
				insert(InstructionEnum::ilte);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "igte")
			{
				insert(InstructionEnum::igte);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "feq")
			{
				insert(InstructionEnum::feq);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fneq")
			{
				insert(InstructionEnum::fneq);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "flt")
			{
				insert(InstructionEnum::flt);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fgt")
			{
				insert(InstructionEnum::fgt);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "flte")
			{
				insert(InstructionEnum::flte);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fgte")
			{
				insert(InstructionEnum::fgte);
				params << getRegister(line);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fisnan")
			{
				insert(InstructionEnum::fisnan);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fisinf")
			{
				insert(InstructionEnum::fisinf);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fisfin")
			{
				insert(InstructionEnum::fisfin);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "fisnorm")
			{
				insert(InstructionEnum::fisnorm);
				params << getRegister(line);
				params << getRegister(line);
			}
			else if (instruction == "test")
			{
				insert(InstructionEnum::test);
				params << getRegister(line);
				params << getRegister(line);
			}
			// structures
			else if (instruction == "load")
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
			}
			else if (instruction == "store")
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
			}
			else if (instruction == "indload")
			{
				uint8 dst = getRegister(line);
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 3)
					CAGE_THROW_ERROR(Exception, "indload requires memory pool");
				insert(InstructionEnum::indload);
				params << dst << index;
			}
			else if (instruction == "indstore")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 3)
					CAGE_THROW_ERROR(Exception, "indstore requires memory pool");
				uint8 src = getRegister(line);
				insert(InstructionEnum::indstore);
				params << index << src;
			}
			else if (instruction == "indindload")
			{
				insert(InstructionEnum::indindload);
				params << getRegister(line);
			}
			else if (instruction == "indindstore")
			{
				insert(InstructionEnum::indindstore);
				params << getRegister(line);
			}
			else if (instruction == "pop")
			{
				uint8 dst = getRegister(line);
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 0)
					CAGE_THROW_ERROR(Exception, "pop requires stack");
				insert(InstructionEnum::pop);
				params << dst << index;
			}
			else if (instruction == "push")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 0)
					CAGE_THROW_ERROR(Exception, "push requires stack");
				uint8 src = getRegister(line);
				insert(InstructionEnum::push);
				params << index << src;
			}
			else if (instruction == "dequeue")
			{
				uint8 dst = getRegister(line);
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 1)
					CAGE_THROW_ERROR(Exception, "dequeue requires queue");
				insert(InstructionEnum::dequeue);
				params << dst << index;
			}
			else if (instruction == "enqueue")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 1)
					CAGE_THROW_ERROR(Exception, "enqueue requires queue");
				uint8 src = getRegister(line);
				insert(InstructionEnum::enqueue);
				params << index << src;
			}
			else if (instruction == "left")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 2)
					CAGE_THROW_ERROR(Exception, "left requires tape");
				insert(InstructionEnum::left);
				params << index;
			}
			else if (instruction == "right")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 2)
					CAGE_THROW_ERROR(Exception, "right requires tape");
				insert(InstructionEnum::right);
				params << index;
			}
			else if (instruction == "center")
			{
				uint8 type, index;
				getStructure(line, type, index);
				if (type != 2)
					CAGE_THROW_ERROR(Exception, "center requires tape");
				insert(InstructionEnum::center);
				params << index;
			}
			else if (instruction == "swap")
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
			}
			else if (instruction == "indswap")
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
			}
			else if (instruction == "stat")
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
			}
			else if (instruction == "indstat")
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
			}

			// todo remaining instructions
		}

		void processLabelReplacements()
		{
			// todo
		}

		Holder<BinaryProgram> compile(PointerRange<const char> sourceCode)
		{
			instructions.clear();
			paramsOffsets.clear();
			sourceLines.clear();
			functionIndices.clear();
			paramsBuffer.free();
			params = Serializer(paramsBuffer);
			labelsReplacements.clear();
			labelsPositions.clear();
			lastFunction = m;
			lastLine = 0;

			Holder<LineReader> lines = newLineReader(sourceCode);
			for (string fullLine; lines->readLine(fullLine); lastLine++)
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
					CAGE_LOG_THROW(stringizer() + "on " + (lastLine + 1) + "th line:");
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
			return templates::move(p).cast<BinaryProgram>();
		}
	};

	Holder<Compiler> newCompiler()
	{
		return detail::systemArena().createImpl<Compiler, CompilerImpl>();
	}

	Holder<BinaryProgram> Compiler::compile(PointerRange<const char> sourceCode)
	{
		CompilerImpl *impl = (CompilerImpl *)this;
		return impl->compile(sourceCode);
	}
}

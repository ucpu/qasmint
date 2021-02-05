#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h>
#include <cage-core/math.h>

#include "program.h"

#include <vector>

namespace qasm
{
	namespace
	{
		struct Stack
		{
			std::vector<uint32> data;
			bool enabled = false;
		};

		struct Queue
		{
			std::vector<uint32> data;
			bool enabled = false;
		};

		struct Tape
		{
			std::vector<uint32> data;
			uint32 center = 0;
			uint32 position = 0;
			bool enabled = false;
		};

		struct Memory
		{
			std::vector<uint32> data;
			bool enabled = false;
			bool readOnly = false;
		};

		struct Callstack
		{
			std::vector<uint16> data;
		};

		struct DataState
		{
			Stack stacks[26] = {};
			Queue queues[26] = {};
			Tape tapes[26] = {};
			Memory memories[26] = {};
			uint32 registers[26 + 26] = {};
			Callstack callstack;
			uint32 programCounter = 0; // index of current instruction in the program
		};
	}

	struct CpuImpl : public Cpu, public DataState
	{
		CpuCreateConfig config;

		CpuStateEnum state = CpuStateEnum::None;
		const ProgramImpl *binary = nullptr;

		CpuImpl(const CpuCreateConfig &config) : config(config)
		{}

		void init()
		{
			CAGE_ASSERT(state != CpuStateEnum::None);
			state = CpuStateEnum::Terminated;
			(DataState &) *this = DataState();
			for (uint32 i = 0; i < 26; i++)
			{
				stacks[i].enabled = i < config.limits.stacksCount;
				queues[i].enabled = i < config.limits.queuesCount;
				tapes[i].enabled = i < config.limits.tapesCount;
				memories[i].enabled = i < config.limits.memoriesCount;
				if (tapes[i].enabled)
					tapes[i].data.resize(1, 0);
				if (memories[i].enabled)
				{
					memories[i].data.resize(config.limits.memoryCapacity[i], 0);
					memories[i].readOnly = config.limits.memoryReadOnly[i];
				}
			}
			state = CpuStateEnum::Initialized;
		}

		uint32 get(uint8 index) const
		{
			CAGE_ASSERT(index < 26 + 26);
			return registers[index];
		}

		void set(uint8 index, uint32 value)
		{
			CAGE_ASSERT(index < 26 + 26);
			registers[index] = value;
		}

		sint32 iget(uint8 index) const
		{
			CAGE_ASSERT(index < 26 + 26);
			return *(sint32 *)&registers[index];
		}

		void iset(uint8 index, sint32 value)
		{
			CAGE_ASSERT(index < 26 + 26);
			registers[index] = *(uint32 *)&value;
		}

		real fget(uint8 index) const
		{
			CAGE_ASSERT(index < 26 + 26);
			return *(real *)&registers[index];
		}

		void fset(uint8 index, real value)
		{
			CAGE_ASSERT(index < 26 + 26);
			registers[index] = *(uint32 *)&value;
		}

		void step()
		{
			CAGE_ASSERT(state == CpuStateEnum::Running);
			const uint32 pc = programCounter++;
			Deserializer params = Deserializer(binary->params);
			params.advance(binary->paramsOffsets[pc]);
			switch (binary->instructions[pc])
			{
			case InstructionEnum::nop:
				break;
			case InstructionEnum::reset:
			{
				uint8 r;
				params >> r;
				set(r, 0);
			} break;
			case InstructionEnum::set:
			{
				uint8 r; uint32 v;
				params >> r >> v;
				set(r, v);
			} break;
			case InstructionEnum::iset:
			{
				uint8 r; sint32 v;
				params >> r >> v;
				iset(r, v);
			} break;
			case InstructionEnum::fset:
			{
				uint8 r; real v;
				params >> r >> v;
				fset(r, v);
			} break;
			case InstructionEnum::copy:
			{
				uint8 r1, r2;
				params >> r1 >> r2;
				set(r1, get(r2));
			} break;
			case InstructionEnum::condrst:
			{
				if (get('z' - 'a' + 26))
				{
					uint8 r;
					params >> r;
					set(r, 0);
				}
			} break;
			case InstructionEnum::condset:
			{
				if (get('z' - 'a' + 26))
				{
					uint8 r; uint32 v;
					params >> r >> v;
					set(r, v);
				}
			} break;
			case InstructionEnum::condiset:
			{
				if (get('z' - 'a' + 26))
				{
					uint8 r; sint32 v;
					params >> r >> v;
					iset(r, v);
				}
			} break;
			case InstructionEnum::condfset:
			{
				if (get('z' - 'a' + 26))
				{
					uint8 r; real v;
					params >> r >> v;
					fset(r, v);
				}
			} break;
			case InstructionEnum::condcpy:
			{
				if (get('z' - 'a' + 26))
				{
					uint8 r1, r2;
					params >> r1 >> r2;
					set(r1, get(r2));
				}
			} break;
			case InstructionEnum::indcpy:
			{
				uint8 d = get('d' - 'a' + 26);
				uint8 s = get('s' - 'a' + 26);
				if (d >= 52 || s >= 52)
					CAGE_THROW_ERROR(Exception, "register index out of range");
				set(d, get(s));
			} break;
			case InstructionEnum::add:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) + get(r));
			} break;
			case InstructionEnum::sub:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) - get(r));
			} break;
			case InstructionEnum::mul:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) * get(r));
			} break;
			case InstructionEnum::div:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				uint32 e = get(r);
				if (!e)
					CAGE_THROW_ERROR(Exception, "division by zero");
				set(d, get(l) / e);
			} break;
			case InstructionEnum::mod:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				uint32 e = get(r);
				if (!e)
					CAGE_THROW_ERROR(Exception, "division by zero");
				set(d, get(l) % e);
			} break;
			case InstructionEnum::inc:
			{
				uint8 r;
				params >> r;
				set(r, get(r) + 1);
			} break;
			case InstructionEnum::dec:
			{
				uint8 r;
				params >> r;
				set(r, get(r) - 1);
			} break;
			case InstructionEnum::iadd:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				iset(d, iget(l) + iget(r));
			} break;
			case InstructionEnum::isub:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				iset(d, iget(l) - iget(r));
			} break;
			case InstructionEnum::imul:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				iset(d, iget(l) * iget(r));
			} break;
			case InstructionEnum::idiv:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				sint32 e = iget(r);
				if (!e)
					CAGE_THROW_ERROR(Exception, "division by zero");
				iset(d, iget(l) / e);
			} break;
			case InstructionEnum::imod:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				sint32 e = iget(r);
				if (!e)
					CAGE_THROW_ERROR(Exception, "division by zero");
				iset(d, iget(l) % e);
			} break;
			case InstructionEnum::iinc:
			{
				uint8 r;
				params >> r;
				iset(r, iget(r) + 1);
			} break;
			case InstructionEnum::idec:
			{
				uint8 r;
				params >> r;
				iset(r, iget(r) - 1);
			} break;
			case InstructionEnum::iabs:
			{
				uint8 r;
				params >> r;
				iset(r, cage::abs(iget(r)));
			} break;
			case InstructionEnum::fadd:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				fset(d, fget(l) + fget(r));
			} break;
			case InstructionEnum::fsub:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				fset(d, fget(l) - fget(r));
			} break;
			case InstructionEnum::fmul:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				fset(d, fget(l) * fget(r));
			} break;
			case InstructionEnum::fdiv:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				fset(d, fget(l) / fget(r));
			} break;
			case InstructionEnum::fpow:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				fset(d, cage::pow(fget(l), fget(r)));
			} break;
			case InstructionEnum::fatan2:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				fset(d, cage::atan2(fget(l), fget(r)).value);
			} break;
			case InstructionEnum::fabs:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, cage::abs(fget(s)));
			} break;
			case InstructionEnum::fsqrt:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, cage::sqrt(fget(s)));
			} break;
			case InstructionEnum::flog:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, cage::log(fget(s)));
			} break;
			case InstructionEnum::fsin:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, cage::sin(rads(fget(s))));
			} break;
			case InstructionEnum::fcos:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, cage::cos(rads(fget(s))));
			} break;
			case InstructionEnum::ftan:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, cage::tan(rads(fget(s))));
			} break;
			case InstructionEnum::fasin:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, cage::asin(fget(s)).value);
			} break;
			case InstructionEnum::facos:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, cage::acos(fget(s)).value);
			} break;
			case InstructionEnum::fatan:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, cage::atan(fget(s)).value);
			} break;
			case InstructionEnum::s2f:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, (real)iget(s));
			} break;
			case InstructionEnum::u2f:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, (real)get(s));
			} break;
			case InstructionEnum::f2s:
			{
				uint8 d, s;
				params >> d >> s;
				iset(d, (sint32)fget(s).value);
			} break;
			case InstructionEnum::f2u:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, (uint32)fget(s).value);
			} break;
			case InstructionEnum::and_:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) && get(r));
			} break;
			case InstructionEnum::or_:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) || get(r));
			} break;
			case InstructionEnum::xor_:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, (get(l) != 0) != (get(r) != 0));
			} break;
			case InstructionEnum::not_:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, !get(s));
			} break;
			case InstructionEnum::inv:
			{
				uint8 d;
				params >> d;
				set(d, !get(d));
			} break;
			case InstructionEnum::shl:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) << get(r));
			} break;
			case InstructionEnum::shr:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) >> get(r));
			} break;
			case InstructionEnum::rol:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				uint32 n = get(l), k = get(r);
				set(d, (n << k) | (n >> (32 - k)));
			} break;
			case InstructionEnum::ror:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				uint32 n = get(l), k = get(r);
				set(d, (n >> k) | (n << (32 - k)));
			} break;
			case InstructionEnum::band:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) & get(r));
			} break;
			case InstructionEnum::bor:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) | get(r));
			} break;
			case InstructionEnum::bxor:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) ^ get(r));
			} break;
			case InstructionEnum::bnot:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, ~get(s));
			} break;
			case InstructionEnum::binv:
			{
				uint8 d;
				params >> d;
				set(d, ~get(d));
			} break;
			case InstructionEnum::eq:
			case InstructionEnum::neq:
			case InstructionEnum::lt:
			case InstructionEnum::gt:
			case InstructionEnum::lte:
			case InstructionEnum::gte:
			case InstructionEnum::ieq:
			case InstructionEnum::ineq:
			case InstructionEnum::ilt:
			case InstructionEnum::igt:
			case InstructionEnum::ilte:
			case InstructionEnum::igte:
			case InstructionEnum::feq:
			case InstructionEnum::fneq:
			case InstructionEnum::flt:
			case InstructionEnum::fgt:
			case InstructionEnum::flte:
			case InstructionEnum::fgte:
			case InstructionEnum::fisnan:
			case InstructionEnum::fisinf:
			case InstructionEnum::fisfin:
			case InstructionEnum::test:
			case InstructionEnum::sload:
			case InstructionEnum::sstore:
			case InstructionEnum::pop:
			case InstructionEnum::push:
			case InstructionEnum::sswap:
			case InstructionEnum::indsswap:
			case InstructionEnum::sstat:
			case InstructionEnum::qload:
			case InstructionEnum::qstore:
			case InstructionEnum::dequeue:
			case InstructionEnum::enqueue:
			case InstructionEnum::qswap:
			case InstructionEnum::indqswap:
			case InstructionEnum::qstat:
			case InstructionEnum::tload:
			case InstructionEnum::tstore:
			case InstructionEnum::left:
			case InstructionEnum::right:
			case InstructionEnum::indleft:
			case InstructionEnum::indright:
			case InstructionEnum::center:
			case InstructionEnum::tswap:
			case InstructionEnum::indtswap:
			case InstructionEnum::tstat:
			case InstructionEnum::mload:
			case InstructionEnum::indload:
			case InstructionEnum::indindload:
			case InstructionEnum::mstore:
			case InstructionEnum::indstore:
			case InstructionEnum::indindstore:
			case InstructionEnum::mswap:
			case InstructionEnum::indmswap:
			case InstructionEnum::mstat:
			case InstructionEnum::jump:
			case InstructionEnum::condjmp:
			case InstructionEnum::call:
			case InstructionEnum::condcall:
			case InstructionEnum::return_:
			case InstructionEnum::condreturn:
			case InstructionEnum::rstat:
			case InstructionEnum::wstat:
			case InstructionEnum::read:
			case InstructionEnum::iread:
			case InstructionEnum::fread:
			case InstructionEnum::cread:
			case InstructionEnum::readln:
			case InstructionEnum::rreset:
			case InstructionEnum::rclear:
			case InstructionEnum::write:
			case InstructionEnum::iwrite:
			case InstructionEnum::fwrite:
			case InstructionEnum::cwrite:
			case InstructionEnum::writeln:
			case InstructionEnum::wreset:
			case InstructionEnum::wclear:
			case InstructionEnum::rwswap:
			case InstructionEnum::timer:
			case InstructionEnum::rdseedany:
			case InstructionEnum::rdseed:
			case InstructionEnum::rand:
			case InstructionEnum::irand:
			case InstructionEnum::frand:
			case InstructionEnum::terminate:
			case InstructionEnum::profiling:
			case InstructionEnum::tracing:
			default:
				CAGE_THROW_ERROR(NotImplemented, "invalid or not implemented instruction");
				break;
			case InstructionEnum::exit:
				state = CpuStateEnum::Finished;
				break;
			}
		}
	};

	Holder<Cpu> newCpu(const CpuCreateConfig &config)
	{
		return detail::systemArena().createImpl<Cpu, CpuImpl>(config);
	}

	void Cpu::program(const BinaryProgram *binary)
	{
		CpuImpl *impl = (CpuImpl *)this;
		impl->binary = (const ProgramImpl *)binary;
		if (binary)
		{
			impl->state = CpuStateEnum::Terminated;
			impl->init();
		}
		else
			impl->state = CpuStateEnum::None;
	}

	void Cpu::reinitialize()
	{
		CpuImpl *impl = (CpuImpl *)this;
		impl->init();
	}

	void Cpu::run()
	{
		CpuImpl *impl = (CpuImpl *)this;
		CAGE_ASSERT(impl->state == CpuStateEnum::Initialized || impl->state == CpuStateEnum::Running);
		impl->state = CpuStateEnum::Running;
		try
		{
			while (impl->state == CpuStateEnum::Running)
				impl->step();
		}
		catch (...)
		{
			impl->state = CpuStateEnum::Terminated;
			throw;
		}
	}

	void Cpu::step()
	{
		CpuImpl *impl = (CpuImpl *)this;
		CAGE_ASSERT(impl->state == CpuStateEnum::Initialized || impl->state == CpuStateEnum::Running);
		impl->state = CpuStateEnum::Running;
		try
		{
			impl->step();
		}
		catch (...)
		{
			impl->state = CpuStateEnum::Terminated;
			throw;
		}
	}

	void Cpu::terminate()
	{
		CpuImpl *impl = (CpuImpl *)this;
		CAGE_ASSERT(impl->state != CpuStateEnum::None);
		impl->state = CpuStateEnum::Terminated;
	}

	CpuStateEnum Cpu::state() const
	{
		const CpuImpl *impl = (const CpuImpl *)this;
		return impl->state;
	}

	PointerRange<const uint32> Cpu::implicitRegisters() const
	{
		const CpuImpl *impl = (const CpuImpl *)this;
		return PointerRange<const uint32>(impl->registers + 26, impl->registers + 26 + 26);
	}

	PointerRange<const uint32> Cpu::explicitRegisters() const
	{
		const CpuImpl *impl = (const CpuImpl *)this;
		return PointerRange<const uint32>(impl->registers, impl->registers + 26);
	}

	void Cpu::explicitRegisters(PointerRange<uint32> data)
	{
		CAGE_ASSERT(data.size() == 26);
		CpuImpl *impl = (CpuImpl *)this;
		CAGE_ASSERT(impl->state == CpuStateEnum::Initialized);
		detail::memcpy(impl->registers, data.data(), 26 * sizeof(uint32));
	}

	Holder<PointerRange<const uint32>> Cpu::stack(uint32 index) const
	{
		const CpuImpl *impl = (const CpuImpl *)this;
		CAGE_ASSERT(index < 26);
		PointerRangeHolder<const uint32> p;
		p.insert(p.end(), impl->stacks[index].data.begin(), impl->stacks[index].data.end());
		return p;
	}

	Holder<PointerRange<const uint32>> Cpu::queue(uint32 index) const
	{
		const CpuImpl *impl = (const CpuImpl *)this;
		CAGE_ASSERT(index < 26);
		PointerRangeHolder<const uint32> p;
		p.insert(p.end(), impl->queues[index].data.begin(), impl->queues[index].data.end());
		return p;
	}

	Holder<PointerRange<const uint32>> Cpu::tape(uint32 index) const
	{
		const CpuImpl *impl = (const CpuImpl *)this;
		CAGE_ASSERT(index < 26);
		PointerRangeHolder<const uint32> p;
		p.insert(p.end(), impl->tapes[index].data.begin(), impl->tapes[index].data.end());
		return p;
	}

	PointerRange<const uint32> Cpu::memory(uint32 index) const
	{
		const CpuImpl *impl = (const CpuImpl *)this;
		CAGE_ASSERT(index < 26);
		return impl->memories[index].data;
	}

	void Cpu::memory(uint32 index, PointerRange<uint32> data)
	{
		CpuImpl *impl = (CpuImpl *)this;
		CAGE_ASSERT(index < 26);
		CAGE_ASSERT(impl->memories[index].data.size() == data.size());
		detail::memcpy(impl->memories[index].data.data(), data.data(), data.size() * sizeof(uint32));
	}

	PointerRange<const uint32> Cpu::callstack() const
	{
		CpuImpl *impl = (CpuImpl *)this;
		// todo
		return {};
	}

	uint32 Cpu::functionIndex() const
	{
		const CpuImpl *impl = (const CpuImpl *)this;
		CAGE_ASSERT(impl->binary);
		return impl->binary->functionIndices[impl->programCounter];
	}

	uint32 Cpu::sourceLine() const
	{
		const CpuImpl *impl = (const CpuImpl *)this;
		CAGE_ASSERT(impl->binary);
		return impl->binary->sourceLines[impl->programCounter];
	}
}

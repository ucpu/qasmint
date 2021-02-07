#include <cage-core/pointerRangeHolder.h>
#include <cage-core/serialization.h>
#include <cage-core/math.h>

#include "program.h"

#include <vector>
#include <cmath> // isnan etc

namespace qasm
{
	namespace
	{
		struct Stat
		{
			uint32 capacity = 0;
			uint32 size = 0;
			sint32 position = 0;
			sint32 leftmost = 0;
			sint32 rightmost = 0;
			bool enabled = false;
			bool writable = true;
		};

		struct StructureBase
		{
			uint32 capacity = 0;
			bool enabled = false;

			void checkEnabled() const
			{
				if (!enabled)
					CAGE_THROW_ERROR(Exception, "structure is disabled");
			}
		};

		struct Stack : public StructureBase
		{
			std::vector<uint32> data;

			Stat stat() const
			{
				Stat s;
				s.capacity = capacity;
				s.size = numeric_cast<uint32>(data.size());
				s.enabled = enabled;
				return s;
			}

			uint32 load() const
			{
				checkEnabled();
				if (data.empty())
					CAGE_THROW_ERROR(Exception, "structure is empty");
				return data.back();
			}

			void store(uint32 value)
			{
				checkEnabled();
				if (data.empty())
					CAGE_THROW_ERROR(Exception, "structure is empty");
				data.back() = value;
			}

			uint32 pop()
			{
				checkEnabled();
				if (data.empty())
					CAGE_THROW_ERROR(Exception, "structure is empty");
				uint32 val = data.back();
				data.pop_back();
				return val;
			}

			void push(uint32 value)
			{
				checkEnabled();
				if (data.size() == capacity)
					CAGE_THROW_ERROR(Exception, "structure is full");
				data.push_back(value);
			}
		};

		struct Queue : public StructureBase
		{
			std::vector<uint32> data;

			Stat stat() const
			{
				Stat s;
				s.capacity = capacity;
				s.size = numeric_cast<uint32>(data.size());
				s.enabled = enabled;
				return s;
			}

			uint32 load() const
			{
				checkEnabled();
				if (data.empty())
					CAGE_THROW_ERROR(Exception, "structure is empty");
				return data.front();
			}

			void store(uint32 value)
			{
				checkEnabled();
				if (data.empty())
					CAGE_THROW_ERROR(Exception, "structure is empty");
				data.front() = value;
			}

			uint32 dequeue()
			{
				checkEnabled();
				if (data.empty())
					CAGE_THROW_ERROR(Exception, "structure is empty");
				uint32 val = data.front();
				data.erase(data.begin());
				return val;
			}

			void enqueue(uint32 value)
			{
				checkEnabled();
				if (data.size() == capacity)
					CAGE_THROW_ERROR(Exception, "structure is full");
				data.push_back(value);
			}
		};

		struct Tape : public StructureBase
		{
			std::vector<uint32> data;
			sint32 offset = 0;
			sint32 position = 0;

			Stat stat() const
			{
				Stat s;
				s.capacity = capacity;
				s.size = numeric_cast<uint32>(data.size());
				s.position = position;
				s.leftmost = -offset;
				s.rightmost = s.size - offset - 1;
				s.enabled = enabled;
				return s;
			}

			uint32 load() const
			{
				checkEnabled();
				return data[offset + position];
			}

			void store(uint32 value)
			{
				checkEnabled();
				data[offset + position] = value;
			}

			void left()
			{
				checkEnabled();
				if (position == -offset)
				{
					if (data.size() == capacity)
						CAGE_THROW_ERROR(Exception, "structure is full");
					data.resize(data.size() + 1, 0);
					for (uint32 i = 1; i < data.size(); i++)
						data[i] = data[i - 1];
					data[0] = 0;
					offset++;
				}
				position--;
			}

			void right()
			{
				checkEnabled();
				if (position + offset + 1 == data.size())
				{
					if (data.size() == capacity)
						CAGE_THROW_ERROR(Exception, "structure is full");
					data.resize(data.size() + 1, 0);
				}
				position++;
			}

			void center()
			{
				checkEnabled();
				position = 0;
			}
		};

		struct Memory : public StructureBase
		{
			std::vector<uint32> data;
			bool readOnly = false;

			Stat stat() const
			{
				Stat s;
				s.capacity = capacity;
				s.size = numeric_cast<uint32>(data.size());
				s.enabled = enabled;
				s.writable = !readOnly;
				return s;
			}

			uint32 load(uint32 addr) const
			{
				checkEnabled();
				if (addr >= data.size())
					CAGE_THROW_ERROR(Exception, "memory address out of bounds");
				return data[addr];
			}

			void store(uint32 addr, uint32 value)
			{
				checkEnabled();
				if (addr >= data.size())
					CAGE_THROW_ERROR(Exception, "memory address out of bounds");
				data[addr] = value;
			}
		};

		struct Callstack
		{
			std::vector<uint16> data;
			uint32 capacity = 0;
		};

		struct DataState
		{
			Stack stacks[26] = {};
			Queue queues[26] = {};
			Tape tapes[26] = {};
			Memory memories[26] = {};
			uint32 registers_[26 + 26] = {};
			Callstack callstack_;
			uint32 programCounter = 0; // index of current instruction in the program
			uint64 stepIndex_ = 0;
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
				stacks[i].capacity = config.limits.stackCapacity;
				queues[i].capacity = config.limits.queueCapacity;
				tapes[i].capacity = config.limits.tapeCapacity;
				memories[i].capacity = config.limits.memoryCapacity[i];
				if (tapes[i].enabled)
					tapes[i].data.resize(1, 0);
				if (memories[i].enabled)
				{
					memories[i].data.resize(config.limits.memoryCapacity[i], 0);
					memories[i].readOnly = config.limits.memoryReadOnly[i];
				}
			}
			callstack_.capacity = config.limits.callstackCapacity;
			state = CpuStateEnum::Initialized;
		}

		uint32 get(uint8 index) const
		{
			CAGE_ASSERT(index < 26 + 26);
			return registers_[index];
		}

		void set(uint8 index, uint32 value)
		{
			CAGE_ASSERT(index < 26 + 26);
			registers_[index] = value;
		}

		sint32 iget(uint8 index) const
		{
			CAGE_ASSERT(index < 26 + 26);
			return *(sint32 *)&registers_[index];
		}

		void iset(uint8 index, sint32 value)
		{
			CAGE_ASSERT(index < 26 + 26);
			registers_[index] = *(uint32 *)&value;
		}

		real fget(uint8 index) const
		{
			CAGE_ASSERT(index < 26 + 26);
			return *(real *)&registers_[index];
		}

		void fset(uint8 index, real value)
		{
			CAGE_ASSERT(index < 26 + 26);
			registers_[index] = *(uint32 *)&value;
		}

		void set(const Stat &stat)
		{
			set('e' - 'a' + 26, stat.enabled);
			set('a' - 'a' + 26, stat.size > 0);
			set('f' - 'a' + 26, stat.size == stat.capacity);
			set('w' - 'a' + 26, stat.writable);
			set('c' - 'a' + 26, stat.capacity);
			set('s' - 'a' + 26, stat.size);
			iset('p' - 'a' + 26, stat.position);
			iset('l' - 'a' + 26, stat.leftmost);
			iset('r' - 'a' + 26, stat.rightmost);
		}

		void jump(uint32 position)
		{
			programCounter = position;
		}

		void step()
		{
			CAGE_ASSERT(state == CpuStateEnum::Running);
			if ((++stepIndex_ % config.interruptPeriod) == 0)
			{
				state = CpuStateEnum::Interrupted;
				return;
			}
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
				if (e == 0)
					CAGE_THROW_ERROR(Exception, "division by zero");
				set(d, get(l) / e);
			} break;
			case InstructionEnum::mod:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				uint32 e = get(r);
				if (e == 0)
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
				if (e == 0)
					CAGE_THROW_ERROR(Exception, "division by zero");
				iset(d, iget(l) / e);
			} break;
			case InstructionEnum::imod:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				sint32 e = iget(r);
				if (e == 0)
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
			case InstructionEnum::ffloor:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, cage::floor(fget(s)));
			} break;
			case InstructionEnum::fround:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, cage::round(fget(s)));
			} break;
			case InstructionEnum::fceil:
			{
				uint8 d, s;
				params >> d >> s;
				fset(d, cage::ceil(fget(s)));
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
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) == get(r));
			} break;
			case InstructionEnum::neq:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) != get(r));
			} break;
			case InstructionEnum::lt:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) < get(r));
			} break;
			case InstructionEnum::gt:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) > get(r));
			} break;
			case InstructionEnum::lte:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) <= get(r));
			} break;
			case InstructionEnum::gte:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, get(l) >= get(r));
			} break;
			case InstructionEnum::ieq:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, iget(l) == iget(r));
			} break;
			case InstructionEnum::ineq:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, iget(l) != iget(r));
			} break;
			case InstructionEnum::ilt:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, iget(l) < iget(r));
			} break;
			case InstructionEnum::igt:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, iget(l) > iget(r));
			} break;
			case InstructionEnum::ilte:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, iget(l) <= iget(r));
			} break;
			case InstructionEnum::igte:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, iget(l) >= iget(r));
			} break;
			case InstructionEnum::feq:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, fget(l) == fget(r));
			} break;
			case InstructionEnum::fneq:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, fget(l) != fget(r));
			} break;
			case InstructionEnum::flt:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, fget(l) < fget(r));
			} break;
			case InstructionEnum::fgt:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, fget(l) > fget(r));
			} break;
			case InstructionEnum::flte:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, fget(l) <= fget(r));
			} break;
			case InstructionEnum::fgte:
			{
				uint8 d, l, r;
				params >> d >> l >> r;
				set(d, fget(l) >= fget(r));
			} break;
			case InstructionEnum::fisnan:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, std::isnan(fget(s).value));
			} break;
			case InstructionEnum::fisinf:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, std::isinf(fget(s).value));
			} break;
			case InstructionEnum::fisfin:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, std::isfinite(fget(s).value));
			} break;
			case InstructionEnum::fisnorm:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, std::isnormal(fget(s).value));
			} break;
			case InstructionEnum::test:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, !!get(s));
			} break;
			case InstructionEnum::sload:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, stacks[s].load());
			} break;
			case InstructionEnum::sstore:
			{
				uint8 d, s;
				params >> d >> s;
				stacks[d].store(get(s));
			} break;
			case InstructionEnum::pop:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, stacks[s].pop());
			} break;
			case InstructionEnum::push:
			{
				uint8 d, s;
				params >> d >> s;
				stacks[d].push(get(s));
			} break;
			case InstructionEnum::sswap:
			{
				uint8 a, b;
				params >> a >> b;
				std::swap(stacks[a], stacks[b]);
			} break;
			case InstructionEnum::indsswap:
			{
				uint8 a = get('i' - 'a' + 26);
				uint8 b = get('j' - 'a' + 26);
				if (a >= 26 || b >= 26)
					CAGE_THROW_ERROR(Exception, "stack index out of range");
				std::swap(stacks[a], stacks[b]);
			} break;
			case InstructionEnum::sstat:
			{
				uint8 s;
				params >> s;
				set(stacks[s].stat());
			} break;
			case InstructionEnum::indsstat:
			{
				uint8 s = get('i' - 'a' + 26);
				if (s >= 26)
					CAGE_THROW_ERROR(Exception, "stack index out of range");
				set(stacks[s].stat());
			} break;
			case InstructionEnum::qload:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, queues[s].load());
			} break;
			case InstructionEnum::qstore:
			{
				uint8 d, s;
				params >> d >> s;
				queues[d].store(get(s));
			} break;
			case InstructionEnum::dequeue:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, queues[s].dequeue());
			} break;
			case InstructionEnum::enqueue:
			{
				uint8 d, s;
				params >> d >> s;
				queues[d].enqueue(get(s));
			} break;
			case InstructionEnum::qswap:
			{
				uint8 a, b;
				params >> a >> b;
				std::swap(queues[a], queues[b]);
			} break;
			case InstructionEnum::indqswap:
			{
				uint8 a = get('i' - 'a' + 26);
				uint8 b = get('j' - 'a' + 26);
				if (a >= 26 || b >= 26)
					CAGE_THROW_ERROR(Exception, "queue index out of range");
				std::swap(queues[a], queues[b]);
			} break;
			case InstructionEnum::qstat:
			{
				uint8 s;
				params >> s;
				set(queues[s].stat());
			} break;
			case InstructionEnum::indqstat:
			{
				uint8 s = get('i' - 'a' + 26);
				if (s >= 26)
					CAGE_THROW_ERROR(Exception, "queue index out of range");
				set(queues[s].stat());
			} break;
			case InstructionEnum::tload:
			{
				uint8 d, s;
				params >> d >> s;
				set(d, tapes[s].load());
			} break;
			case InstructionEnum::tstore:
			{
				uint8 d, s;
				params >> d >> s;
				tapes[d].store(get(s));
			} break;
			case InstructionEnum::left:
			{
				uint8 d;
				params >> d;
				tapes[d].left();
			} break;
			case InstructionEnum::right:
			{
				uint8 d;
				params >> d;
				tapes[d].right();
			} break;
			case InstructionEnum::center:
			{
				uint8 d;
				params >> d;
				tapes[d].center();
			} break;
			case InstructionEnum::tswap:
			{
				uint8 a, b;
				params >> a >> b;
				std::swap(tapes[a], tapes[b]);
			} break;
			case InstructionEnum::indtswap:
			{
				uint8 a = get('i' - 'a' + 26);
				uint8 b = get('j' - 'a' + 26);
				if (a >= 26 || b >= 26)
					CAGE_THROW_ERROR(Exception, "tape index out of range");
				std::swap(tapes[a], tapes[b]);
			} break;
			case InstructionEnum::tstat:
			{
				uint8 s;
				params >> s;
				set(tapes[s].stat());
			} break;
			case InstructionEnum::indtstat:
			{
				uint8 s = get('i' - 'a' + 26);
				if (s >= 26)
					CAGE_THROW_ERROR(Exception, "tape index out of range");
				set(tapes[s].stat());
			} break;
			case InstructionEnum::mload:
			{
				uint8 d, s; uint32 a;
				params >> d >> s >> a;
				set(d, memories[s].load(a));
			} break;
			case InstructionEnum::indload:
			{
				uint8 d, s;
				params >> d >> s;
				uint32 a = get('i' - 'a' + 26);
				set(d, memories[s].load(a));
			} break;
			case InstructionEnum::indindload:
			{
				uint8 d;
				params >> d;
				uint32 a = get('i' - 'a' + 26);
				uint8 s = get('j' - 'a' + 26);
				if (s >= 26)
					CAGE_THROW_ERROR(Exception, "memory index out of range");
				set(d, memories[s].load(a));
			} break;
			case InstructionEnum::mstore:
			{
				uint8 d, s; uint32 a;
				params >> d >> a >> s;
				memories[d].store(a, get(s));
			} break;
			case InstructionEnum::indstore:
			{
				uint8 d, s;
				params >> d >> s;
				uint32 a = get('i' - 'a' + 26);
				memories[d].store(a, get(s));
			} break;
			case InstructionEnum::indindstore:
			{
				uint8 s;
				params >> s;
				uint32 a = get('i' - 'a' + 26);
				uint8 d = get('j' - 'a' + 26);
				if (d >= 26)
					CAGE_THROW_ERROR(Exception, "memory index out of range");
				memories[d].store(a, get(s));
			} break;
			case InstructionEnum::mswap:
			{
				uint8 a, b;
				params >> a >> b;
				std::swap(memories[a], memories[b]);
			} break;
			case InstructionEnum::indmswap:
			{
				uint8 a = get('i' - 'a' + 26);
				uint8 b = get('j' - 'a' + 26);
				if (a >= 26 || b >= 26)
					CAGE_THROW_ERROR(Exception, "memory index out of range");
				std::swap(memories[a], memories[b]);
			} break;
			case InstructionEnum::mstat:
			{
				uint8 s;
				params >> s;
				set(memories[s].stat());
			} break;
			case InstructionEnum::indmstat:
			{
				uint8 s = get('i' - 'a' + 26);
				if (s >= 26)
					CAGE_THROW_ERROR(Exception, "memory index out of range");
				set(memories[s].stat());
			} break;
			case InstructionEnum::jump:
			{
				uint32 pos;
				params >> pos;
				jump(pos);
			} break;
			case InstructionEnum::condjmp:
			{
				if (get('z' - 'a' + 26) != 0)
				{
					uint32 pos;
					params >> pos;
					jump(pos);
				}
			} break;
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
			case InstructionEnum::profiling:
			case InstructionEnum::tracing:
				CAGE_THROW_ERROR(NotImplemented, "not yet implemented instruction");
				break;
			case InstructionEnum::breakpoint:
				state = CpuStateEnum::Interrupted;
				break;
			case InstructionEnum::terminate:
				CAGE_THROW_ERROR(Exception, "explicit terminate");
				break;
			case InstructionEnum::disabled:
				CAGE_THROW_ERROR(Exception, "disabled instruction");
				break;
			case InstructionEnum::exit:
				state = CpuStateEnum::Finished;
				break;
			default:
				CAGE_THROW_ERROR(Exception, "invalid instruction");
				break;
			}
		}
	};

	Holder<Cpu> newCpu(const CpuCreateConfig &config)
	{
		return detail::systemArena().createImpl<Cpu, CpuImpl>(config);
	}

	void Cpu::program(const Program *binary)
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
		CAGE_ASSERT(impl->state == CpuStateEnum::Initialized || impl->state == CpuStateEnum::Running || impl->state == CpuStateEnum::Interrupted);
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
		CAGE_ASSERT(impl->state == CpuStateEnum::Initialized || impl->state == CpuStateEnum::Running || impl->state == CpuStateEnum::Interrupted);
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

	void Cpu::interrupt()
	{
		CpuImpl *impl = (CpuImpl *)this;
		CAGE_ASSERT(impl->state != CpuStateEnum::None);
		impl->state = CpuStateEnum::Interrupted;
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
		return PointerRange<const uint32>(impl->registers_ + 26, impl->registers_ + 26 + 26);
	}

	PointerRange<const uint32> Cpu::registers() const
	{
		const CpuImpl *impl = (const CpuImpl *)this;
		return PointerRange<const uint32>(impl->registers_, impl->registers_ + 26);
	}

	void Cpu::registers(PointerRange<uint32> data)
	{
		CAGE_ASSERT(data.size() == 26);
		CpuImpl *impl = (CpuImpl *)this;
		CAGE_ASSERT(impl->state == CpuStateEnum::Initialized);
		detail::memcpy(impl->registers_, data.data(), 26 * sizeof(uint32));
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

	uint64 Cpu::stepIndex() const
	{
		const CpuImpl *impl = (const CpuImpl *)this;
		CAGE_ASSERT(impl->binary);
		return impl->stepIndex_;
	}
}

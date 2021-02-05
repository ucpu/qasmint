#include <cage-core/ini.h>

#include "qasm/qasm.h"

namespace qasm
{
	CpuLimitsConfig limitsFromIni(Ini *ini)
	{
		CpuLimitsConfig limits;

		{ // memories
			for (uint32 i = 0; i < 26; i++)
			{
				limits.memoryCapacity[i] = ini->getUint32("memory", stringizer() + "capacity_" + (i + 1), limits.memoryCapacity[i]);
				limits.memoryReadOnly[i] = ini->getBool("memory", stringizer() + "read_only_" + (i + 1), limits.memoryReadOnly[i]);
			}
			limits.memoriesCount = ini->getUint32("memory", "instances", limits.memoriesCount);
		}

		{ // stacks
			limits.stackCapacity = ini->getUint32("stacks", "capacity", limits.stackCapacity);
			limits.stacksCount = ini->getUint32("stacks", "instances", limits.stacksCount);
		}

		{ // queues
			limits.queueCapacity = ini->getUint32("queues", "capacity", limits.stackCapacity);
			limits.queuesCount = ini->getUint32("queues", "instances", limits.stacksCount);
		}

		{ // tapes
			limits.tapeCapacity = ini->getUint32("tapes", "capacity", limits.stackCapacity);
			limits.tapesCount = ini->getUint32("tapes", "instances", limits.stacksCount);
		}

		return limits;
	}

	void limitsToIni(const CpuLimitsConfig &limits, Ini *ini)
	{
		{ // memories
			for (uint32 i = 0; i < 26; i++)
			{
				ini->setUint32("memory", stringizer() + "capacity_" + (i + 1), limits.memoryCapacity[i]);
				ini->setBool("memory", stringizer() + "read_only_" + (i + 1), limits.memoryReadOnly[i]);
			}
			ini->setUint32("memory", "instances", limits.memoriesCount);
		}

		{ // stacks
			ini->setUint32("stacks", "capacity", limits.stackCapacity);
			ini->setUint32("stacks", "instances", limits.stacksCount);
		}

		{ // queues
			ini->setUint32("queues", "capacity", limits.stackCapacity);
			ini->setUint32("queues", "instances", limits.stacksCount);
		}

		{ // tapes
			ini->setUint32("tapes", "capacity", limits.stackCapacity);
			ini->setUint32("tapes", "instances", limits.stacksCount);
		}
	}
}

#pragma once

#include "defines.hpp"
#include "motherboard.hpp"

#define RAM_SIZE 16777216

class RAM
{
	public:
		RAM(Motherboard* mb);

		void tick();

	private:
		uint8_t getData(uint32_t address);
		void setData(uint8_t data, uint32_t address);
		void set5bData(uint64_t data, uint32_t address); // To set instructions manually
		void dumpData(uint32_t startAddress, uint32_t endAddress);

		Motherboard* m_mb;

		uint8_t m_memory[RAM_SIZE];
};

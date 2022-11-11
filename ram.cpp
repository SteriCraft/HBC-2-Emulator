#include "ram.hpp"

RAM::RAM(Motherboard* mb)
{
	m_mb = mb;

	for (unsigned int i(0); i < RAM_SIZE; i++)
	{
		m_memory[i] = 0x00;
	}

	set5bData(0xF000000000, 0x00010C);
	set5bData(0x4C00000000, 0xF00000);

	dumpData(0x000100, 0x00010F);

	// Draw "Hello" on the screen at position (10, 10)
	set5bData(0x78A05A0000, WORK_MEMORY_START_ADDRESS);
	set5bData(0x7880480000, WORK_MEMORY_START_ADDRESS + 5);
	set5bData(0x78B00A0000, WORK_MEMORY_START_ADDRESS + 10);
	set5bData(0x78B80A0000, WORK_MEMORY_START_ADDRESS + 15);
	set5bData(0x118000A000, WORK_MEMORY_START_ADDRESS + 20);
	set5bData(0x7880650000, WORK_MEMORY_START_ADDRESS + 25);
	set5bData(0x4470000000, WORK_MEMORY_START_ADDRESS + 30);
	set5bData(0x118000A000, WORK_MEMORY_START_ADDRESS + 35);
	set5bData(0x78806C0000, WORK_MEMORY_START_ADDRESS + 40);
	set5bData(0x4470000000, WORK_MEMORY_START_ADDRESS + 45);
	set5bData(0x118000A000, WORK_MEMORY_START_ADDRESS + 50);
	set5bData(0x78806C0000, WORK_MEMORY_START_ADDRESS + 55);
	set5bData(0x4470000000, WORK_MEMORY_START_ADDRESS + 60);
	set5bData(0x118000A000, WORK_MEMORY_START_ADDRESS + 65);
	set5bData(0x78806F0000, WORK_MEMORY_START_ADDRESS + 70);
	set5bData(0x4470000000, WORK_MEMORY_START_ADDRESS + 75);
	set5bData(0x118000A000, WORK_MEMORY_START_ADDRESS + 80);
	set5bData(0x118000B000, WORK_MEMORY_START_ADDRESS + 85);
	set5bData(0x3800000000, WORK_MEMORY_START_ADDRESS + 90);

	// Draw character subroutine
	set5bData(0x8848000000, 0x00A000);
	set5bData(0x8850000000, 0x00A000 + 5);
	set5bData(0x8858000000, 0x00A000 + 10);
	set5bData(0x7898000000, 0x00A000 + 15);
	set5bData(0x7888000000, 0x00A000 + 20);
	set5bData(0x4048000000, 0x00A000 + 25);
	set5bData(0x7888010000, 0x00A000 + 30);
	set5bData(0x404E000000, 0x00A000 + 35);
	set5bData(0x7888020000, 0x00A000 + 40);
	set5bData(0x404F000000, 0x00A000 + 45);
	set5bData(0x7888030000, 0x00A000 + 50);
	set5bData(0x7890010000, 0x00A000 + 55);
	set5bData(0x404A000000, 0x00A000 + 60);
	set5bData(0x404B000000, 0x00A000 + 65);
	set5bData(0x8458000000, 0x00A000 + 70);
	set5bData(0x8450000000, 0x00A000 + 75);
	set5bData(0x8448000000, 0x00A000 + 80);
	set5bData(0x8C00000000, 0x00A000 + 85);

	// Refresh screen subroutine
	set5bData(0x8840000000, 0x00B000);
	set5bData(0x8848000000, 0x00B000 + 5);
	set5bData(0x8850000000, 0x00B000 + 10);
	set5bData(0x7880020000, 0x00B000 + 15);
	set5bData(0x7888030000, 0x00B000 + 20);
	set5bData(0x7890000000, 0x00B000 + 25);
	set5bData(0x4048000000, 0x00B000 + 30);
	set5bData(0x404A000000, 0x00B000 + 35);
	set5bData(0x8450000000, 0x00B000 + 40);
	set5bData(0x8448000000, 0x00B000 + 45);
	set5bData(0x8440000000, 0x00B000 + 50);
	set5bData(0x8C00000000, 0x00B000 + 55);
}

void RAM::tick()
{
	if (m_mb != nullptr) // No need to process data if the ram chip is not plugged in a motherboard
	{
		if (m_mb->getRE()) // CPU asking for data access
		{
			if (m_mb->getRW()) // CPU asking to write data
			{
				setData(m_mb->getDataBus(),  m_mb->getAddressBus());
			}
			else // CPU asking to read data
			{
				m_mb->setDataBus(getData(m_mb->getAddressBus()));
			}

			m_mb->setRE(false); // In reality, this is done on CPU side
		}
	}
}

// PRIVATE
uint8_t RAM::getData(uint32_t address)
{
	return m_memory[address];
}

void RAM::setData(uint8_t data, uint32_t address)
{
	m_memory[address] = data;
}

void RAM::set5bData(uint64_t data, uint32_t address)
{
	if (address < WORK_MEMORY_END_ADDRESS - 5)
	{
		data &= 0x000000FFFFFFFFFF;

		uint8_t byte1((uint8_t)((data & 0x000000FF00000000) >> 32));
		uint8_t byte2((uint8_t)((data & 0x00000000FF000000) >> 24));
		uint8_t byte3((uint8_t)((data & 0x0000000000FF0000) >> 16));
		uint8_t byte4((uint8_t)((data & 0x000000000000FF00) >> 8));
		uint8_t byte5((uint8_t)(data & 0x00000000000000FF));

		m_memory[address] = byte1;
		m_memory[address + 1] = byte2;
		m_memory[address + 2] = byte3;
		m_memory[address + 3] = byte4;
		m_memory[address + 4] = byte5;
	}
}

void RAM::dumpData(uint32_t startAddress, uint32_t endAddress)
{
	uint8_t value(0x00);

	startAddress = startAddress & 0x00FFFFFF;
	endAddress = endAddress & 0x00FFFFFF;

	if (endAddress > startAddress)
	{
		std::cout << "=== RAM CONTENT from $(0x" << uintToString(startAddress) << ") to $(0x" << uintToString(endAddress) << ") ===" << std::endl;

		for (uint32_t i(startAddress); i < endAddress; i += 5)
		{
			std::cout << "0x" << uintToString(i) << " : ";

			for (unsigned int j(0); j < 5; j++)
			{
				value = m_memory[i + j];

				std::cout << uintToString(value) << " ";
			}

			std::cout << std::endl;
		}

		std::cout << "===================================================" << std::endl;
	}
}

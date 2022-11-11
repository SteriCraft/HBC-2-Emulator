#pragma once

#include "defines.hpp"
#include "device.hpp"
#include "motherboard.hpp"

#define INTERRUPT_QUEUE_SIZE 256

class IOD
{
	public:
		IOD(Motherboard* mb);

		void tick();

		// GETTERS
		uint8_t getStackCount();

	private:
		Motherboard* m_mb;

		std::vector<std::pair<uint8_t, uint8_t>> m_interruptsQueue; // Pair of uint8_t, the first being the I/O port number and the second being the data sent by the device
};

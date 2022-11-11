#include "iod.hpp"

IOD::IOD(Motherboard* mb)
{
	m_mb = mb;
}

void IOD::tick()
{
	for (unsigned int i(0); i < PORTS_NB; i++) // Checking for interrupts from plugged devices
	{
		Device* dev(m_mb->getDevice(i));

		if (dev != nullptr)
		{
			if (dev->getINT())
			{
				if (m_interruptsQueue.size() < INTERRUPT_QUEUE_SIZE) // If the queue is full, new interrupts are discarded
				{
					m_interruptsQueue.push_back(std::pair<uint8_t, uint8_t>(i, m_mb->getPortData(i)));
				}
			}
		}
	}

	if (!m_interruptsQueue.empty() && !m_mb->getINT()) // Signaling the CPU that an interrupt is waiting
	{
		m_mb->setINT(true);
	}
	else if (m_mb->getINT() && m_mb->getINR()) // CPU ready for the next interrupt, which means it is necessarily not asking for data access from a I/O port TODO : I'm not sure about that last sentence
	{
		m_mb->setINT(false);

		m_mb->setAddressBus(m_interruptsQueue[0].first);
		m_mb->setDataBus(m_interruptsQueue[0].second);
		std::cout << "[IOD] : Interrupt data : " << uintToString(m_interruptsQueue[0].second) << std::endl;

		m_interruptsQueue.erase(m_interruptsQueue.begin());
	}
	else if (m_mb->getIE()) // CPU asking for data access on a I/O port, which it cannot do while handling an interrupt TODO : I'm not sure about that last sentence
	{
		uint8_t address = (uint8_t)(m_mb->getAddressBus() & 0x000000FF);

		if (m_mb->getDevice(address) != nullptr) // If a device is plugged in this I/O port
		{
			if (m_mb->getRW()) // CPU asking to write data
			{
				m_mb->setPortData(m_mb->getDataBus(), address);
			}
			else // CPU asking to read data
			{
				m_mb->getPortData(address);
			}
		}

		m_mb->setIE(false); // In reality, this is done on CPU side
	}
}

// GETTERS
uint8_t IOD::getStackCount()
{
	return (uint8_t)m_interruptsQueue.size();
}

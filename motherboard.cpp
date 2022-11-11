#include "motherboard.hpp"

Motherboard::Motherboard()
{
	m_rw = false;
	m_re = false;
	m_ie = false;
	m_int = false;
	m_inr = false;

	m_dataBus = 0;
	m_addressBus = 0;

	for (unsigned int i(0); i < PORTS_NB; i++)
	{
		m_ports[i].first = nullptr;
	}
}

Motherboard::~Motherboard()
{
	for (unsigned int i(0); i < PORTS_NB; i++)
	{
		if (m_ports[i].first != nullptr)
		{
			delete m_ports[i].first;
		}
	}
}

uint8_t Motherboard::getDataBus()
{
	return m_dataBus;
}

uint32_t Motherboard::getAddressBus()
{
	return m_addressBus & 0x00FFFFFF; // Discards the two most significant bytes in the 32-bit integer variable
}

void Motherboard::setDataBus(uint8_t data)
{
	m_dataBus = data;
}

void Motherboard::setAddressBus(uint32_t address)
{
	m_addressBus = address & 0x00FFFFFF; // Discards the two most significant bytes in the 32-bit integer variable
}

Device* Motherboard::getDevice(uint8_t portID)
{
	return m_ports[portID].first;
}

bool Motherboard::plugDevice(Device* dev)
{
	for (unsigned int i(0); i < PORTS_NB; i++)
	{
		if (m_ports[i].first == nullptr)
		{
			for (unsigned int j(0); j < dev->getPortsNumber(); j++)
			{
				m_ports[i + j].first = dev;
				m_ports[i + j].second = j;
			}

			return true;
		}
	}

	return false;
}

void Motherboard::unplugDevice(Device* dev)
{
	for (unsigned int i(0); i < PORTS_NB; i++)
	{
		if (m_ports[i].first == dev)
		{
			for (unsigned int j(0); m_ports[i + j].first == dev; j++)
			{
				m_ports[i + j].first = nullptr;
				m_ports[i + j].second = 0;
			}

			return;
		}
	}
}

void Motherboard::setPortData(uint8_t data, uint8_t portNb)
{
	if (m_ports[portNb].first != nullptr)
	{
		m_ports[portNb].first->setData(data, m_ports[portNb].second);
	}
}

uint8_t Motherboard::getPortData(uint8_t portNb)
{
	if (m_ports[portNb].first != nullptr)
	{
		return m_ports[portNb].first->getData(m_ports[portNb].second);
	}

	return 0x00; // Default value
}

bool Motherboard::getRW()
{
	return m_rw;
}

bool Motherboard::getRE()
{
	return m_re;
}

bool Motherboard::getIE()
{
	return m_ie;
}

bool Motherboard::getINT()
{
	return m_int;
}

bool Motherboard::getINR()
{
	return m_inr;
}

void Motherboard::setRW(bool _rw)
{
	m_rw = _rw;
}

void Motherboard::setRE(bool _re)
{
	m_re = _re;
}

void Motherboard::setIE(bool _ie)
{
	m_ie = _ie;
}

void Motherboard::setINT(bool _int)
{
	m_int = _int;
}

void Motherboard::setINR(bool _inr)
{
	m_inr = _inr;
}

#include "device.hpp"

Device::Device()
{
	m_INT = false;
}

uint8_t Device::getPortsNumber()
{
	return (uint8_t)m_ports.size();
}

uint8_t Device::getData(uint8_t portNb)
{
	return m_ports[portNb];
}

void Device::setData(uint8_t data, uint8_t portNb)
{
	m_ports[portNb] = data;
}

bool Device::getINT()
{
	return m_INT;
}

void Device::interruptAcknoledgement()
{
	m_INT = false;
}

// PRIVATE
void Device::triggerInterrupt()
{
	m_INT = true;
}

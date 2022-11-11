#pragma once

#include "defines.hpp"

class Device
{
	public:
		Device();

		uint8_t getPortsNumber();

		uint8_t getData(uint8_t portNb);
		void setData(uint8_t data, uint8_t portNb);
		
		bool getINT();
		void interruptAcknoledgement();

	protected:
		void triggerInterrupt();

		bool m_INT;

		std::vector<uint8_t> m_ports;
};

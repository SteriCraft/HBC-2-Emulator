#pragma once

#include "device.hpp"

#define PORTS_NB 256

class Motherboard
{
public:
	Motherboard();
	~Motherboard();

	uint8_t getDataBus();
	uint32_t getAddressBus();

	void setDataBus(uint8_t data);
	void setAddressBus(uint32_t address);

	// I/O
	Device* getDevice(uint8_t portID);
	bool plugDevice(Device* dev);
	void unplugDevice(Device* dev);
	void setPortData(uint8_t data, uint8_t portNb);
	uint8_t getPortData(uint8_t portNb);

	bool getRW();
	bool getRE();
	bool getIE();
	bool getINT();
	bool getINR();

	void setRW(bool _rw);
	void setRE(bool _re);
	void setIE(bool _ie);
	void setINT(bool _int);
	void setINR(bool _inr);

private:
	bool m_rw; // Read / Write pin
	bool m_re; // Ram Enable pin
	bool m_ie; // I/O Enable pin
	bool m_int; // Interrupt triggerred pin
	bool m_inr; // Interrupt acknoledgement pin (ready for I/O port)

	uint8_t m_dataBus;
	uint32_t m_addressBus;

	std::pair<Device*, uint8_t> m_ports[PORTS_NB]; // The uint8_t value stands for the port nb on device side
};

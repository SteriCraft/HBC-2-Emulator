#pragma once

#include "device.hpp"

class Keyboard : public Device
{
	public:
		Keyboard();
		~Keyboard();

		void tick();

		void receiveKeyCode(uint8_t k, bool pressed);

	private:
		enum class KeyboardStep { CODE, PRESS_STATE };

		KeyboardStep m_step;
		std::vector<std::pair<uint8_t, bool>> m_keyQueue;
};

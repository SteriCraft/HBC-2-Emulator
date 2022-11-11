#include "keyboard.hpp"

Keyboard::Keyboard()
{
	m_step = KeyboardStep::CODE;

	m_ports.push_back(0x00); // Keycode port

	m_INT = false;
}

Keyboard::~Keyboard()
{

}

void Keyboard::tick()
{
	m_INT = false;

	if (m_step == KeyboardStep::CODE) // When a key is pressed, the keyboard sends the key code first
	{
		if (m_keyQueue.size() > 0)
		{
			m_INT = true;

			m_ports[0] = m_keyQueue[0].first;

			m_step = KeyboardStep::PRESS_STATE;
		}
	}
	else if (m_step == KeyboardStep::PRESS_STATE) // On the next cycle, the keyboard sends a code to precise if the key was pressed or released
	{
		m_INT = true;

		m_ports[0] = m_keyQueue[0].second ? 0x0E : 0x0F; // true = key pressed, custom ascii code 0x0E, false = key released, custom ascii code 0x0F
		m_keyQueue.erase(m_keyQueue.begin());

		m_step = KeyboardStep::CODE;
	}
}

void Keyboard::receiveKeyCode(uint8_t k, bool pressed)
{
	std::pair<uint8_t, bool> keyCode;

	keyCode.first = k;
	keyCode.second = pressed;

	m_keyQueue.push_back(keyCode);
}

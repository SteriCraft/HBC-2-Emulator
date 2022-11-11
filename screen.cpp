#include "screen.hpp"

Screen::Screen(Keyboard* k)
{
	m_keyboard = k;

	m_screenWindow = new sf::RenderWindow(sf::VideoMode(SCREEN_WIDTH, SCREEN_HEIGHT), "HBC-2 Emulator - Monitor", sf::Style::Titlebar);
	clearScreen();
	m_evt = new sf::Event();

	m_characterMap = new sf::Texture();
	m_characterMap->loadFromFile("ascii_character_map.png");

	m_charToDisplay = new sf::RectangleShape(sf::Vector2f(CHAR_WIDTH, CHAR_HEIGHT));
	m_charToDisplay->setScale(PIXEL_WIDTH, PIXEL_WIDTH);

	m_ports.push_back(0x00); // Port 0 = CHARACTER CODE (CHAR)
	m_ports.push_back(0x00); // Port 1 = POS X (POS_X)
	m_ports.push_back(0x00); // Port 2 = POS Y (POS_Y)
	m_ports.push_back(0x00); // Port 3 = COMMAND (CMD)

	m_screenRefreshed = false;
}

Screen::~Screen()
{
	delete m_evt;
	delete m_characterMap;
	delete m_charToDisplay;
	delete m_screenWindow;
}

void Screen::tick()
{
	while (m_screenWindow->pollEvent(*m_evt))
	{
		if (m_evt->type == sf::Event::KeyPressed)
		{
			if (m_keyboard != nullptr)
			{
				m_keyboard->receiveKeyCode(m_evt->key.code, true); // This is supposed to be keyboard work only, but it is the window that handles key hits in SFML
			}
		}
		else if (m_evt->type == sf::Event::KeyReleased)
		{
			if (m_keyboard != nullptr)
			{
				m_keyboard->receiveKeyCode(m_evt->key.code, false); // This is supposed to be keyboard work only, but it is the window that handles key hits in SFML
			}
		}
	}

	switch (m_ports[(uint8_t)Port::CMD])
	{
		case (uint8_t)Cmd::REFRESH:
			if (!m_screenRefreshed)
			{
				refreshScreen();

				m_screenRefreshed = true;
			}
			break;

		case (uint8_t)Cmd::DRAW:
			drawCharacter(m_ports[(uint8_t)Port::CHAR], m_ports[(uint8_t)Port::POS_X], m_ports[(uint8_t)Port::POS_Y]);
			break;

		case (uint8_t)Cmd::CLEAR:
			clearScreen();
			break;

		default:
			m_screenRefreshed = false;
	}
}

// PRIVATE
void Screen::drawCharacter(char c, uint8_t row, uint8_t line)
{
	int charX(0), charY(0); // Used to select the right character in the character map

	if (c >= 32 && c <= 126 && row < SCREEN_CHAR_WIDTH && line < SCREEN_CHAR_HEIGHT) // ' ' is the first character to be displayable, '~' is the last
	{
		c -= 32; // Set it to 0;

		charX = c % 16; // 16 characters per line in the character map
		charY = c / 16;

		m_charToDisplay->setTexture(m_characterMap);
		m_charToDisplay->setTextureRect(sf::IntRect(charX * CHAR_WIDTH, charY * CHAR_HEIGHT, CHAR_WIDTH, CHAR_HEIGHT)); // Picking just one character in the map

		m_charToDisplay->setPosition((float)(row * CHAR_WIDTH * PIXEL_WIDTH), (float)(line * CHAR_HEIGHT * PIXEL_WIDTH));
		m_screenWindow->draw(*m_charToDisplay);
	}
}

void Screen::clearScreen()
{
	m_screenWindow->clear(BACKGROUND_COLOR);

	refreshScreen();
}

void Screen::refreshScreen()
{
	m_screenWindow->display();
}

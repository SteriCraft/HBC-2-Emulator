#pragma once

#include "keyboard.hpp"

#define SCREEN_CHAR_WIDTH 40
#define SCREEN_CHAR_HEIGHT 25

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

#define PIXEL_WIDTH 4

#define SCREEN_WIDTH_PX (SCREEN_CHAR_WIDTH * CHAR_WIDTH) // 240 px by default
#define SCREEN_HEIGHT_PX (SCREEN_CHAR_HEIGHT * CHAR_HEIGHT) // 200 px by default

#define SCREEN_WIDTH SCREEN_WIDTH_PX * PIXEL_WIDTH
#define SCREEN_HEIGHT SCREEN_HEIGHT_PX * PIXEL_WIDTH

#define BACKGROUND_COLOR sf::Color(0, 0, 0, 255)

class Screen : public Device
{
	public:
		Screen(Keyboard* k);
		~Screen();

		void tick();

	private:
		void drawCharacter(char c, uint8_t row, uint8_t line);
		void clearScreen();
		void refreshScreen();

		enum class Port { CHAR = 0, POS_X = 1, POS_Y = 2, CMD = 3 };
		enum class Cmd { DRAW = 1, REFRESH = 2, CLEAR = 3 };

		Keyboard* m_keyboard;

		sf::RenderWindow* m_screenWindow;
		sf::Event* m_evt;
		sf::RectangleShape* m_charToDisplay;
		sf::Texture* m_characterMap, m_temp;

		bool m_screenRefreshed;
};

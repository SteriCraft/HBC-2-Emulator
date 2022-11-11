#include "cpu.hpp"
#include "iod.hpp"
#include "ram.hpp"
#include "screen.hpp"

typedef struct
{
	sf::Text addressBusTxt;
	sf::Text dataBusTxt;
	sf::Text stackCountTxt;
	sf::Text current甥odeTxt;
	sf::Text stackPointerTxt;
	sf::Text programCounterTxt;
	sf::Text flagsRegisterTxt;
	sf::Text acc1Txt;
	sf::Text acc2Txt;
	sf::Text 發pTxt;
	sf::Text aluTxt;
	sf::Text regATxt;
	sf::Text regBTxt;
	sf::Text regCTxt;
	sf::Text regDTxt;
	sf::Text regITxt;
	sf::Text regJTxt;
	sf::Text regXTxt;
	sf::Text regYTxt;

	sf::Text stepByStep;
	sf::Text frequency;
} TextStruct;

void initTexts(sf::Font* font, TextStruct* texts);
void updateTexts(TextStruct* texts, Motherboard* mb, CPU* cpuChip, IOD* iodChip, bool stepMode, int freq);
void computerTick(CPU* cpuChip, IOD* iodChip, RAM* ramChip, Screen* monitor, Keyboard* kb);
void drawTexts(TextStruct* texts, sf::RenderWindow* window);
void drawCPUState(Step cpuState, sf::RectangleShape* redInd1, sf::RectangleShape* redInd2, sf::RectangleShape* redInd3, sf::RectangleShape* redInd4,
				  sf::RectangleShape* redInd5, sf::RectangleShape* orgInd, sf::RectangleShape* grnInd, sf::RenderWindow* window);

int main()
{
	// Computer init
	Motherboard* mb = new Motherboard();
	CPU* cpuChip = new CPU(mb);
	IOD* iodChip = new IOD(mb);
	RAM* ramChip = new RAM(mb);
	Keyboard* kb = new Keyboard();
	Screen* monitor = new Screen(kb);

	mb->plugDevice(monitor);
	mb->plugDevice(kb);

	// Window init
	sf::RenderWindow* window = new sf::RenderWindow(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "HBC-2 Emulator - CPU Diagram", sf::Style::Titlebar | sf::Style::Close);
	sf::Event* evt = new sf::Event();

	// Text
	sf::Font* font = new sf::Font();
	TextStruct* texts = new TextStruct();

	initTexts(font, texts);

	// Background
	sf::Texture* backgroundTx = new sf::Texture();
	sf::RectangleShape* background = new sf::RectangleShape(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));

	backgroundTx->loadFromFile("diagram.png");
	background->setTexture(backgroundTx);

	// Clock indicator
	sf::Texture* clockLowTx = new sf::Texture();
	sf::Texture* clockHighTx = new sf::Texture();

	clockLowTx->loadFromFile("clock_low.png");
	clockHighTx->loadFromFile("clock_high.png");

	sf::RectangleShape* clockLow =  new sf::RectangleShape(sf::Vector2f((float)clockLowTx->getSize().x, (float)clockLowTx->getSize().y));
	sf::RectangleShape* clockHigh = new sf::RectangleShape(sf::Vector2f((float)clockHighTx->getSize().x, (float)clockHighTx->getSize().y));

	clockLow->setTexture(clockLowTx);
	clockHigh->setTexture(clockHighTx);

	clockLow->setPosition(CLOCK_POS_X, CLOCK_POS_Y);
	clockHigh->setPosition(CLOCK_POS_X, CLOCK_POS_Y);

	// CPU state indicator
	sf::Texture* redIndicatorTx1 = new sf::Texture();
	sf::Texture* redIndicatorTx2 = new sf::Texture();
	sf::Texture* redIndicatorTx3 = new sf::Texture();
	sf::Texture* redIndicatorTx4 = new sf::Texture();
	sf::Texture* redIndicatorTx5 = new sf::Texture();
	sf::Texture* orangeIndicatorTx = new sf::Texture();
	sf::Texture* greenIndicatorTx = new sf::Texture();

	redIndicatorTx1->loadFromFile("red_indicator.png");
	redIndicatorTx2->loadFromFile("red_indicator.png");
	redIndicatorTx3->loadFromFile("red_indicator.png");
	redIndicatorTx4->loadFromFile("red_indicator_tight.png");
	redIndicatorTx5->loadFromFile("red_indicator.png");
	orangeIndicatorTx->loadFromFile("orange_indicator.png");
	greenIndicatorTx->loadFromFile("green_indicator.png");

	sf::RectangleShape* redIndicator1 =   new sf::RectangleShape(sf::Vector2f((float)redIndicatorTx1->getSize().x, (float)redIndicatorTx1->getSize().y));
	sf::RectangleShape* redIndicator2 =   new sf::RectangleShape(sf::Vector2f((float)redIndicatorTx2->getSize().x, (float)redIndicatorTx2->getSize().y));
	sf::RectangleShape* redIndicator3 =   new sf::RectangleShape(sf::Vector2f((float)redIndicatorTx3->getSize().x, (float)redIndicatorTx3->getSize().y));
	sf::RectangleShape* redIndicator4 =   new sf::RectangleShape(sf::Vector2f((float)redIndicatorTx4->getSize().x, (float)redIndicatorTx4->getSize().y));
	sf::RectangleShape* redIndicator5 =   new sf::RectangleShape(sf::Vector2f((float)redIndicatorTx5->getSize().x, (float)redIndicatorTx5->getSize().y));
	sf::RectangleShape* orangeIndicator = new sf::RectangleShape(sf::Vector2f((float)orangeIndicatorTx->getSize().x, (float)orangeIndicatorTx->getSize().y));
	sf::RectangleShape* greenIndicator =  new sf::RectangleShape(sf::Vector2f((float)greenIndicatorTx->getSize().x, (float)greenIndicatorTx->getSize().y));

	redIndicator1->setTexture(redIndicatorTx1);
	redIndicator2->setTexture(redIndicatorTx2);
	redIndicator3->setTexture(redIndicatorTx3);
	redIndicator4->setTexture(redIndicatorTx4);
	redIndicator5->setTexture(redIndicatorTx5);
	orangeIndicator->setTexture(orangeIndicatorTx);
	greenIndicator->setTexture(greenIndicatorTx);

	redIndicator1->setPosition(RED1_POS_X, RED1_POS_Y);
	redIndicator2->setPosition(RED2_POS_X, RED2_POS_Y);
	redIndicator3->setPosition(RED3_POS_X, RED3_POS_Y);
	redIndicator4->setPosition(RED4_POS_X, RED4_POS_Y);
	redIndicator5->setPosition(RED5_POS_X, RED5_POS_Y);
	orangeIndicator->setPosition(ORANGE_POS_X, ORANGE_POS_Y);
	greenIndicator->setPosition(GREEN_POS_X, GREEN_POS_Y);

	// Clock
	bool stepByStepMode(true);
	bool clockState(false); // Low = false | High = true
	bool tick(true); // True if the user asks to go one step forward (T key)
	int currentFrequency(0); // In kHz
	sf::Time currentTime, previousTime, lastFreqMeasure;
	sf::Clock clock;
	int clockCycles(0);

	// === Event loop ===
	while (window->isOpen())
	{
		while (window->pollEvent(*evt))
		{
			if (evt->type == sf::Event::Closed)
				window->close();
			else if (evt->type == sf::Event::KeyPressed)
			{
				if (evt->key.code == sf::Keyboard::Escape)
					window->close();
				else if (evt->key.code == sf::Keyboard::T) // Step by step command (usable only in step by step mode)
				{
					if (stepByStepMode)
					{
						tick = true;
					}
				}
				else if (evt->key.code == sf::Keyboard::S) // Step by step mode command
				{
					stepByStepMode = !stepByStepMode;

					if (!stepByStepMode)
						tick = true;
				}
			}
		}

		// Computer update
		if (tick)
		{
			clockState = !clockState;

			computerTick(cpuChip, iodChip, ramChip, monitor, kb);
			clockCycles++;

			if (stepByStepMode)
				tick = false;
		}

		// Window update (60 FPS)
		currentTime = clock.getElapsedTime();
		if (currentTime.asMilliseconds() - previousTime.asMilliseconds() >= (1000.f / FPS))
		{
			// Calculating average frequency during last second
			if (currentTime.asMilliseconds() - lastFreqMeasure.asMilliseconds() >= 1000.f) // Measuring every second
			{
				currentFrequency = (int)((float)clockCycles / 1000.f);

				clockCycles = 0;
				lastFreqMeasure = clock.getElapsedTime();
			}

			updateTexts(texts, mb, cpuChip, iodChip, stepByStepMode, currentFrequency);

			window->clear();

			window->draw(*background);
			window->draw((clockState) ? *clockHigh : *clockLow); // I love ternary conditions
			drawCPUState(cpuChip->getCurrentStep(), redIndicator1, redIndicator2, redIndicator3, redIndicator4, redIndicator5, orangeIndicator, greenIndicator, window);
			drawTexts(texts, window);

			window->display();

			currentTime = previousTime = clock.getElapsedTime();
		}
    }

	mb->unplugDevice(kb);
	mb->unplugDevice(monitor);
	
	// Memory clearance
	delete redIndicator1;     delete redIndicator2;     delete redIndicator3;     delete redIndicator4;     delete redIndicator5;
	delete orangeIndicator;   delete greenIndicator;
	delete redIndicatorTx1;   delete redIndicatorTx2;   delete redIndicatorTx3;   delete redIndicatorTx4;   delete redIndicatorTx5;
	delete orangeIndicatorTx; delete greenIndicatorTx;

	delete clockLow;		  delete clockHigh;
	delete background;
	delete clockLowTx;		  delete clockHighTx;
	delete backgroundTx;

	delete texts;
	delete font;

	delete evt;
	delete window;

	delete monitor;
	delete ramChip;
	delete iodChip;
	delete cpuChip;
	delete mb;

    return 0;
}

void initTexts(sf::Font* font, TextStruct* texts)
{
	font->loadFromFile("calibri.ttf");

	texts->addressBusTxt.setFont(*font);
	texts->addressBusTxt.setCharacterSize(FONT_SIZE);
	texts->addressBusTxt.setString("0x000000");
	texts->addressBusTxt.setPosition(ADDR_BUS_TXT_POS_X - (texts->addressBusTxt.getGlobalBounds().width / 2), ADDR_BUS_TXT_POS_Y - (texts->addressBusTxt.getGlobalBounds().height / 2));

	texts->dataBusTxt.setFont(*font);
	texts->dataBusTxt.setCharacterSize(FONT_SIZE);
	texts->dataBusTxt.setString("0x00");
	texts->dataBusTxt.setPosition(DATA_BUS_TXT_POS_X - (texts->dataBusTxt.getGlobalBounds().width / 2), DATA_BUS_TXT_POS_Y - (texts->dataBusTxt.getGlobalBounds().height / 2));

	texts->stackCountTxt.setFont(*font);
	texts->stackCountTxt.setCharacterSize(FONT_SIZE);

	texts->current甥odeTxt.setFont(*font);
	texts->current甥odeTxt.setCharacterSize(FONT_SIZE);

	texts->stackPointerTxt.setFont(*font);
	texts->stackPointerTxt.setCharacterSize(FONT_SIZE);
	texts->stackPointerTxt.setString("0x00");
	texts->stackPointerTxt.setPosition(STK_PTR_TXT_POS_X - (texts->stackPointerTxt.getGlobalBounds().width / 2), STK_PTR_TXT_POS_Y - (texts->stackPointerTxt.getGlobalBounds().height / 2));

	texts->programCounterTxt.setFont(*font);
	texts->programCounterTxt.setCharacterSize(FONT_SIZE);
	texts->programCounterTxt.setString("0x000400");
	texts->programCounterTxt.setPosition(PRG_CNT_TXT_POS_X - (texts->programCounterTxt.getGlobalBounds().width / 2), PRG_CNT_TXT_POS_Y - (texts->programCounterTxt.getGlobalBounds().height / 2));

	texts->flagsRegisterTxt.setFont(*font);
	texts->flagsRegisterTxt.setCharacterSize(FONT_SIZE);
	texts->flagsRegisterTxt.setString("0 0 0 0 0 0 0 1");
	texts->flagsRegisterTxt.setPosition(FLG_REG_TXT_POS_X - (texts->flagsRegisterTxt.getGlobalBounds().width / 2), FLG_REG_TXT_POS_Y - (texts->flagsRegisterTxt.getGlobalBounds().height / 2));

	texts->acc1Txt.setFont(*font);
	texts->acc1Txt.setCharacterSize(FONT_SIZE);
	texts->acc1Txt.setString("0x00");
	texts->acc1Txt.setPosition(ACC1_TXT_POS_X - (texts->acc1Txt.getGlobalBounds().width / 2), ACC_TXT_POS_Y - (texts->acc1Txt.getGlobalBounds().height / 2));

	texts->acc2Txt.setFont(*font);
	texts->acc2Txt.setCharacterSize(FONT_SIZE);
	texts->acc2Txt.setString("0x00");
	texts->acc2Txt.setPosition(ACC2_TXT_POS_X - (texts->acc2Txt.getGlobalBounds().width / 2), ACC_TXT_POS_Y - (texts->acc2Txt.getGlobalBounds().height / 2));

	texts->發pTxt.setFont(*font);
	texts->發pTxt.setCharacterSize(FONT_SIZE);

	texts->aluTxt.setFont(*font);
	texts->aluTxt.setCharacterSize(FONT_SIZE);
	texts->aluTxt.setString("0x00");
	texts->aluTxt.setPosition(ALU_TXT_POS_X - (texts->aluTxt.getGlobalBounds().width / 2), ALU_TXT_POS_Y - (texts->aluTxt.getGlobalBounds().height / 2));

	texts->regATxt.setFont(*font);
	texts->regATxt.setCharacterSize(FONT_SIZE);
	texts->regATxt.setString("0x00");
	texts->regATxt.setPosition(REG_A_I_TXT_POS_X - (texts->regATxt.getGlobalBounds().width / 2), REG1_TXT_POS_Y - (texts->regATxt.getGlobalBounds().height / 2));

	texts->regBTxt.setFont(*font);
	texts->regBTxt.setCharacterSize(FONT_SIZE);
	texts->regBTxt.setString("0x00");
	texts->regBTxt.setPosition(REG_B_J_TXT_POS_X - (texts->regBTxt.getGlobalBounds().width / 2), REG1_TXT_POS_Y - (texts->regBTxt.getGlobalBounds().height / 2));

	texts->regCTxt.setFont(*font);
	texts->regCTxt.setCharacterSize(FONT_SIZE);
	texts->regCTxt.setString("0x00");
	texts->regCTxt.setPosition(REG_C_X_TXT_POS_X - (texts->regCTxt.getGlobalBounds().width / 2), REG1_TXT_POS_Y - (texts->regCTxt.getGlobalBounds().height / 2));

	texts->regDTxt.setFont(*font);
	texts->regDTxt.setCharacterSize(FONT_SIZE);
	texts->regDTxt.setString("0x00");
	texts->regDTxt.setPosition(REG_D_Y_TXT_POS_X - (texts->regDTxt.getGlobalBounds().width / 2), REG1_TXT_POS_Y - (texts->regDTxt.getGlobalBounds().height / 2));

	texts->regITxt.setFont(*font);
	texts->regITxt.setCharacterSize(FONT_SIZE);
	texts->regITxt.setString("0x00");
	texts->regITxt.setPosition(REG_A_I_TXT_POS_X - (texts->regITxt.getGlobalBounds().width / 2), REG2_TXT_POS_Y - (texts->regITxt.getGlobalBounds().height / 2));

	texts->regJTxt.setFont(*font);
	texts->regJTxt.setCharacterSize(FONT_SIZE);
	texts->regJTxt.setString("0x00");
	texts->regJTxt.setPosition(REG_B_J_TXT_POS_X - (texts->regJTxt.getGlobalBounds().width / 2), REG2_TXT_POS_Y - (texts->regJTxt.getGlobalBounds().height / 2));

	texts->regXTxt.setFont(*font);
	texts->regXTxt.setCharacterSize(FONT_SIZE);
	texts->regXTxt.setString("0x00");
	texts->regXTxt.setPosition(REG_C_X_TXT_POS_X - (texts->regXTxt.getGlobalBounds().width / 2), REG2_TXT_POS_Y - (texts->regXTxt.getGlobalBounds().height / 2));

	texts->regYTxt.setFont(*font);
	texts->regYTxt.setCharacterSize(FONT_SIZE);
	texts->regYTxt.setString("0x00");
	texts->regYTxt.setPosition(REG_D_Y_TXT_POS_X - (texts->regYTxt.getGlobalBounds().width / 2), REG2_TXT_POS_Y - (texts->regYTxt.getGlobalBounds().height / 2));

	texts->stepByStep.setFont(*font);
	texts->stepByStep.setCharacterSize(FONT_SIZE);
	texts->stepByStep.setString("Step by step : On");
	texts->stepByStep.setPosition(STEP_MODE_TXT_POS_X, STEP_MODE_TXT_POS_Y);

	texts->frequency.setFont(*font);
	texts->frequency.setCharacterSize(FONT_SIZE);
	texts->frequency.setString("Frequency : ");
	texts->frequency.setPosition(FREQ_TXT_POS_X, FREQ_TXT_POS_Y);
}

void updateTexts(TextStruct* texts, Motherboard* mb, CPU* cpuChip, IOD* iodChip, bool stepMode, int freq)
{
	texts->addressBusTxt.setString("0x" + uintToString(mb->getAddressBus()));
	texts->dataBusTxt.setString("0x" + uintToString(mb->getDataBus()));

	texts->stackCountTxt.setString(std::to_string(iodChip->getStackCount()));
	texts->stackCountTxt.setPosition(STK_CNT_TXT_POS_X - (texts->stackCountTxt.getGlobalBounds().width / 2), STK_CNT_TXT_POS_Y - (texts->stackCountTxt.getGlobalBounds().height / 2));

	texts->current甥odeTxt.setString(cpuChip->getCurrent湣ode());
	texts->current甥odeTxt.setPosition(焜PCODE_TXT_POS_X - (texts->current甥odeTxt.getGlobalBounds().width / 2), 焜PCODE_TXT_POS_Y - (texts->current甥odeTxt.getGlobalBounds().height / 2));

	texts->stackPointerTxt.setString("0x" + uintToString(cpuChip->getStackPointer()));
	texts->programCounterTxt.setString("0x" + uintToString(cpuChip->getProgramCounter()));
	texts->flagsRegisterTxt.setString(cpuChip->getFlagsRegister());
	texts->acc1Txt.setString("0x" + uintToString(cpuChip->getAcc1()));
	texts->acc2Txt.setString("0x" + uintToString(cpuChip->getAcc2()));

	texts->發pTxt.setString(cpuChip->get發p());
	texts->發pTxt.setPosition(焜P_TXT_POS_X - (texts->發pTxt.getGlobalBounds().width / 2), 焜P_TXT_POS_Y - (texts->發pTxt.getGlobalBounds().height / 2));

	texts->aluTxt.setString("0x" + uintToString(cpuChip->getAluOut()));
	texts->regATxt.setString("0x" + uintToString(cpuChip->getRegA()));
	texts->regBTxt.setString("0x" + uintToString(cpuChip->getRegB()));
	texts->regCTxt.setString("0x" + uintToString(cpuChip->getRegC()));
	texts->regDTxt.setString("0x" + uintToString(cpuChip->getRegD()));
	texts->regITxt.setString("0x" + uintToString(cpuChip->getRegI()));
	texts->regJTxt.setString("0x" + uintToString(cpuChip->getRegJ()));
	texts->regXTxt.setString("0x" + uintToString(cpuChip->getRegX()));
	texts->regYTxt.setString("0x" + uintToString(cpuChip->getRegY()));

	texts->stepByStep.setString(stepMode ? "Step by step" : "");
	
	std::string frequencyStr(std::to_string(freq));
	frequencyStr += " kHz";
	texts->frequency.setString(frequencyStr);
}

void computerTick(CPU* cpuChip, IOD* iodChip, RAM* ramChip, Screen* monitor, Keyboard* kb)
{
	cpuChip->tick();
	iodChip->tick();
	ramChip->tick();
	monitor->tick();
	kb->tick();
}

void drawTexts(TextStruct* texts, sf::RenderWindow* window)
{
	window->draw(texts->addressBusTxt);
	window->draw(texts->dataBusTxt);
	window->draw(texts->stackCountTxt);
	window->draw(texts->current甥odeTxt);
	window->draw(texts->stackPointerTxt);
	window->draw(texts->programCounterTxt);
	window->draw(texts->flagsRegisterTxt);
	window->draw(texts->acc1Txt);
	window->draw(texts->acc2Txt);
	window->draw(texts->發pTxt);
	window->draw(texts->aluTxt);
	window->draw(texts->regATxt);
	window->draw(texts->regBTxt);
	window->draw(texts->regCTxt);
	window->draw(texts->regDTxt);
	window->draw(texts->regITxt);
	window->draw(texts->regJTxt);
	window->draw(texts->regXTxt);
	window->draw(texts->regYTxt);

	window->draw(texts->stepByStep);
	window->draw(texts->frequency);
}

void drawCPUState(Step cpuState, sf::RectangleShape* redInd1, sf::RectangleShape* redInd2, sf::RectangleShape* redInd3, sf::RectangleShape* redInd4,
				  sf::RectangleShape* redInd5, sf::RectangleShape* orgInd, sf::RectangleShape* grnInd, sf::RenderWindow* window)
{
	switch (cpuState)
	{
		case Step::FETCH_1:
			window->draw(*redInd1);
			break;

		case Step::FETCH_2:
			window->draw(*redInd1);
			window->draw(*redInd2);
			break;

		case Step::FETCH_3:
			window->draw(*redInd1);
			window->draw(*redInd2);
			window->draw(*redInd3);
			break;

		case Step::FETCH_4:
			window->draw(*redInd1);
			window->draw(*redInd2);
			window->draw(*redInd3);
			window->draw(*redInd4);
			break;

		case Step::FETCH_5:
			window->draw(*redInd1);
			window->draw(*redInd2);
			window->draw(*redInd3);
			window->draw(*redInd4);
			window->draw(*redInd5);
			break;

		case Step::DECODE:
			window->draw(*redInd1);
			window->draw(*redInd2);
			window->draw(*redInd3);
			window->draw(*redInd4);
			window->draw(*redInd5);
			window->draw(*orgInd);
			break;

		case Step::EXECUTE:
			window->draw(*redInd1);
			window->draw(*redInd2);
			window->draw(*redInd3);
			window->draw(*redInd4);
			window->draw(*redInd5);
			window->draw(*orgInd);
			window->draw(*grnInd);
			break;
	}
}

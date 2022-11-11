#pragma once

#include <iostream>
#include <vector>
#include <SFML/Graphics.hpp>
#include <stdint.h>
#include <string>
#include <sstream>

// SCREEN
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 835
#define FPS 20.f

// TEXT
#define FONT_SIZE 18

#define FREQ_TXT_POS_X 765
#define FREQ_TXT_POS_Y 705
#define STEP_MODE_TXT_POS_X 740
#define STEP_MODE_TXT_POS_Y 805

#define ADDR_BUS_TXT_POS_X 620
#define ADDR_BUS_TXT_POS_Y 38

#define DATA_BUS_TXT_POS_X 602
#define DATA_BUS_TXT_POS_Y 104

#define STK_CNT_TXT_POS_X 1085
#define STK_CNT_TXT_POS_Y 524

#define 킣PCODE_TXT_POS_X 483
#define 킣PCODE_TXT_POS_Y 311

#define STK_PTR_TXT_POS_X 484
#define STK_PTR_TXT_POS_Y 390

#define PRG_CNT_TXT_POS_X 484
#define PRG_CNT_TXT_POS_Y 476

#define FLG_REG_TXT_POS_X 471
#define FLG_REG_TXT_POS_Y 590

#define ACC1_TXT_POS_X 119
#define ACC2_TXT_POS_X 230
#define ACC_TXT_POS_Y 366

#define 킣P_TXT_POS_X 323
#define 킣P_TXT_POS_Y 425

#define ALU_TXT_POS_X 175
#define ALU_TXT_POS_Y 514

#define REG_A_I_TXT_POS_X 86
#define REG_B_J_TXT_POS_X 177
#define REG_C_X_TXT_POS_X 265
#define REG_D_Y_TXT_POS_X 352
#define REG1_TXT_POS_Y 690
#define REG2_TXT_POS_Y 786

// CLOCK INDICATOR
#define CLOCK_INDICATOR_WIDTH 23
#define CLOCK_POS_X 775.f
#define CLOCK_POS_Y 768.f

// CPU STATE INDICATOR
#define RED1_POS_X 360
#define RED1_POS_Y 261

#define RED2_POS_X 396
#define RED2_POS_Y 261

#define RED3_POS_X 432
#define RED3_POS_Y 261

#define RED4_POS_X 468
#define RED4_POS_Y 261

#define RED5_POS_X 503
#define RED5_POS_Y 261

#define ORANGE_POS_X 539
#define ORANGE_POS_Y 261

#define GREEN_POS_X 575
#define GREEN_POS_Y 261

// RAM
#define WORK_MEMORY_START_ADDRESS 0x00000400
#define WORK_MEMORY_END_ADDRESS 0x00FFFFFF

#define STACK_START_ADDRESS 0x00000000
#define STACK_END_ADDRESS 0x000000FF

std::string uintToString(uint8_t);
std::string uintToString(uint32_t);

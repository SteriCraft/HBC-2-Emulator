#pragma once

#include "defines.hpp"
#include "motherboard.hpp"

enum class Step {FETCH_1, FETCH_2, FETCH_3, FETCH_4, FETCH_5, DECODE, EXECUTE, STOP, INTERRUPT_1, INTERRUPT_2, INTERRUPT_3, INTERRUPT_4, INTERRUPT_5, INTERRUPT_6, INTERRUPT_7, INTERRUPT_8};

#define INSTRUCTIONS_NB 48 // Including NOP (0x0 by definition)
#define ADDRESSING_MODES_NB 8

enum class InstructionsList {NOP = 0x0,  ADC = 0x1,  ADD = 0x2,  AND = 0x3,  CAL = 0x4,  CLC = 0x5,  CLE = 0x6,  CLI = 0x7,  CLN = 0x8,  CLS = 0x9,  CLZ = 0xA,  CLF = 0xB,
							 CMP = 0xC,  DEC = 0xD,  HLT = 0xE,  IN = 0xF,   OUT = 0x10, INC = 0x11, INT = 0x12, IRT = 0x13, JMC = 0x14, JME = 0x15, JMF = 0x16, JMK = 0x17,
							 JMP = 0x18, JMS = 0x19, JMZ = 0x1A, JMN = 0x1B, STR = 0x1C, LOD = 0x1D, MOV = 0x1E, NOT = 0x1F, OR = 0x20,  POP = 0x21, PSH = 0x22, RET = 0x23,
							 SHL = 0x24, ASR = 0x25, SHR = 0x26, STC = 0x27, STI = 0x28, STN = 0x29, STF = 0x2A, STS = 0x2B, STE = 0x2C, STZ = 0x2D, SUB = 0x2E, XOR = 0x2F};

enum class AddressingModesList {NONE = 0x0, REG = 0x1, REG_IMM8 = 0x2, REG_RAM = 0x3, RAMREG_IMMREG = 0x4, REG24 = 0x5, IMM24 = 0x6, IMM8 = 0x7};

enum class 發pcodesList {MOVACC1, MOVACC2, MOVPC, MOVADDBUS, MOVDATABUS, MOVREG, RAMREAD, RAMWRITE, DECSTK, INCSTK, CLC, CLE, CLI, CLN, CLS, CLZ, CLF, STH, STC, STI,
						 STN, STF, STS, STE, STZ, JMC, JME, JMF, JMK, JMP, JMS, JMZ, JMN, IN, OUT, INT, ADC, ADD, SUB, AND, OR, XOR, NOT, SHL, ASR, SHR, CMP, INCPC, UNDEFINED};

enum class 發perandsList {R1, R2, R4, V1, VX, RX, ALUOUT, DATABUS, DATABUS16, PCDATABUS8, PCDATABUS, STK, X1, PC_16, PC_8, PC8, I};

typedef struct
{
	發pcodesList uopcode;
	std::vector<發perandsList> uoperands;
} uInstruction;

typedef struct
{
	std::vector<uInstruction> addrMode[ADDRESSING_MODES_NB];
} Instruction;

#define REGISTER_NB 8
enum class Registers {A = 0x0, B = 0x1, C = 0x2, D = 0x3, I = 0x4, J = 0x5, X = 0x6, Y = 0x7};

typedef struct
{
	bool CARRY;
	bool ZERO;
	bool HALT;
	bool NEGATIVE;
	bool INFERIOR;
	bool SUPERIOR;
	bool EQUAL;
	bool INTERRUPT;
} Flags;

class CPU
{
	public:
		CPU(Motherboard* mb);

		void tick();

		// Getters
		Step getCurrentStep();
		std::string getCurrent湣ode();
		uint8_t getStackPointer();
		uint32_t getProgramCounter();
		std::string getFlagsRegister();
		uint8_t getAcc1();
		uint8_t getAcc2();
		std::string get發p();
		uint8_t getAluOut();
		uint8_t getRegA();
		uint8_t getRegB();
		uint8_t getRegC();
		uint8_t getRegD();
		uint8_t getRegI();
		uint8_t getRegJ();
		uint8_t getRegX();
		uint8_t getRegY();

	private:
		void init甥ode();
		// 發pcodes
		void _movAcc1(uint8_t v);
		void _movAcc2(uint8_t v);
		void _movPC(uint32_t v);
		void _movAddBus(uint32_t v);
		void _movDataBus(uint8_t v);
		void _movReg(uint8_t* reg, uint8_t* v);
		void _ramRead();
		void _ramWrite();
		void _decSTK();
		void _incSTK();
		void _incPC();
		void _clc();
		void _cle();
		void _cli();
		void _cln();
		void _cls();
		void _clz();
		void _clf();
		void _sth();
		void _stc();
		void _sti();
		void _stn();
		void _stf();
		void _sts();
		void _ste();
		void _stz();
		void _jmc(uint32_t addr);
		void _jme(uint32_t addr);
		void _jmf(uint32_t addr);
		void _jmk(uint32_t addr);
		void _jmp(uint32_t addr);
		void _jms(uint32_t addr);
		void _jmz(uint32_t addr);
		void _jmn(uint32_t addr);
		void _in();
		void _out();
		void _interrupt();
		void _adc();
		void _add();
		void _sub();
		void _and();
		void _or();
		void _xor();
		void _not();
		void _shl();
		void _asr();
		void _shr();
		void _cmp();

		Instruction m_instructionsUCode[INSTRUCTIONS_NB];

		Motherboard* m_mb;

		Step m_step;
		int m_甥odeStep;

		// Decoded instruction variables
		uint64_t m_fetchedInstruction;
		uint8_t m_opcode; // 6 last bits
		uint8_t m_addressingMode; // 4 last bits
		uint8_t* m_R1; // 3 last bits
		uint8_t* m_R2; // 3 last bits
		uint8_t* m_R3; // 3 last bits
		uint8_t* m_R4; // 3 last bits
		uint8_t m_V1;
		uint8_t m_V2;
		uint8_t m_Ex;
		uint32_t m_Vx;
		uint32_t m_Rx;
		uint32_t m_interruptVector;
		uint8_t m_interruptPort;
		uint8_t m_dataBusValue;
		uint8_t* m_pointedRegister;
		uint8_t* m_valueToStore;
		發pcodesList m_甥ode;
		
		bool m_jump;
		bool m_softwareInterrupt;
		uint8_t m_accu1;
		uint8_t m_accu2;
		uint8_t m_aluOut;
		uint8_t m_registers[REGISTER_NB];
		uint8_t m_interruptData;
		Flags m_flags;
		uint32_t m_programCounter;
		uint32_t m_stackPointer;
};

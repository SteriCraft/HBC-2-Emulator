#include "cpu.hpp"

// TODO : Carry flag when stack overflow

CPU::CPU(Motherboard* mb)
{
	m_mb = mb;

	for (uint8_t i(0); i <= 7; i++)
	{
		m_registers[i] = 0x00;
	}
	m_interruptData = 0x00;

	m_flags.CARRY = false;
	m_flags.EQUAL = false;
	m_flags.HALT = false;
	m_flags.INFERIOR = false;
	m_flags.INTERRUPT = false;
	m_flags.NEGATIVE = false;
	m_flags.SUPERIOR = false;
	m_flags.ZERO = false;

	init�code();

	m_aluOut = 0;
	m_accu1 = 0;
	m_accu2 = 0;
	m_flags.INTERRUPT = true; // Interrupt flag up by default, CPU ready to handle interrupts
	m_programCounter = WORK_MEMORY_START_ADDRESS;
	m_stackPointer = STACK_START_ADDRESS;
	m_softwareInterrupt = false;

	m_fetchedInstruction = 0;
	m_opcode = 0;
	m_addressingMode = 0;
	m_R1 = nullptr;
	m_R2 = nullptr;
	m_R3 = nullptr;
	m_R4 = nullptr;
	m_V1 = 0;
	m_V2 = 0;
	m_Ex = 0;
	m_Vx = 0;
	m_Rx = 0;
	m_interruptVector = 0;
	m_interruptPort = 0;
	m_dataBusValue = 0;
	m_pointedRegister = nullptr;
	m_valueToStore = nullptr;

	m_jump = false;
	m_step = Step::FETCH_1;
	m_�codeStep = 0;
}

void CPU::tick()
{
	if (m_flags.HALT)
	{
		m_flags.INTERRUPT = true; // CPU always ready to handle interrupts when halted (otherwise, no way to get out of the interrupt)

		if (m_step != Step::FETCH_1)
		{
			m_step = Step::FETCH_1;
			m_�codeStep = 0; // Safety feature

			m_programCounter += 5;
			if (m_programCounter >= WORK_MEMORY_END_ADDRESS)
				m_programCounter = 0;
		}
		else if (m_mb->getINT()) // An interrupt has been triggered and the CPU is ready to handle it
		{
			m_step = Step::INTERRUPT_1;

			m_mb->setINR(true); // Needless to check if it is a software interrupt, since the CPU is currently halted
			m_flags.HALT = false;
		}
	}
	else
	{
		switch (m_step)
		{
		case Step::INTERRUPT_1:
			m_mb->setINR(false);

			m_flags.INTERRUPT = false; // CPU unable to handle new interrupts from this point (until IRT instruction is reached, or if the programmer uses instruction STI before IRT)

			m_interruptPort = (uint8_t)(m_mb->getAddressBus() & 0x000000FF);
			m_interruptData = m_mb->getDataBus(); // Done here so the IOD chip hasn't time to change the data bus value (for the next interrupt if there is any)
			std::cout << "Interrupt data : " << uintToString(m_interruptData) << std::endl;

			_movAddBus(m_stackPointer); // Pushing less significant byte of program counter in the stack
			_movDataBus((uint8_t)(m_programCounter & 0x000000FF));
			_ramWrite();
			_incSTK();

			m_step = Step::INTERRUPT_2;
			break;

		case Step::INTERRUPT_2:
			_movAddBus(m_stackPointer); // Pushing average significant byte of program counter in the stack
			_movDataBus((uint8_t)((m_programCounter & 0x0000FF00) >> 8));
			_ramWrite();
			_incSTK();

			m_step = Step::INTERRUPT_3;
			break;

		case Step::INTERRUPT_3:
			_movAddBus(m_stackPointer); // Pushing most significant byte of program counter in the stack
			_movDataBus((uint8_t)((m_programCounter & 0x00FF0000) >> 16));
			_ramWrite();
			_incSTK();

			m_step = Step::INTERRUPT_4;
			break;

		case Step::INTERRUPT_4:
			if (!m_softwareInterrupt) // Pushes I register on the stack if it is not a software interrupt
			{
				_movAddBus(m_stackPointer);
				_movDataBus(m_registers[(int)Registers::I]);
				_ramWrite();
				_incSTK();
			}
			else
			{
				m_softwareInterrupt = false;
			}

			m_registers[(int)Registers::I] = m_interruptData;

			m_step = Step::INTERRUPT_5; // Starting to execute the interrupt associated code
			break;

		case Step::INTERRUPT_5:
			_movAddBus(0x000100 + 3 * (uint32_t)(m_interruptPort)); // Requesting first byte of the interrupt vector address in the interrupt vector table
			_ramRead();

			m_step = Step::INTERRUPT_6;
			break;

		case Step::INTERRUPT_6:
			m_interruptVector = ((uint32_t)(m_mb->getDataBus())) << 16; // Saving first byte

			_movAddBus(0x000101 + 3 * (uint32_t)(m_interruptPort)); // Requesting second byte of the interrupt vector address in the interrupt vector table
			_ramRead();

			m_step = Step::INTERRUPT_7;
			break;

		case Step::INTERRUPT_7:
			m_interruptVector += ((uint32_t)(m_mb->getDataBus())) << 8; // Saving second byte

			_movAddBus(0x000102 + 3 * (uint32_t)(m_interruptPort)); // Requesting third byte of the interrupt vector address in the interrupt vector table
			_ramRead();

			m_step = Step::INTERRUPT_8;
			break;

		case Step::INTERRUPT_8:
			m_interruptVector += (uint32_t)(m_mb->getDataBus()); // Saving third byte

			m_programCounter = m_interruptVector; // Setting the program counter according to the interrupt vector

			m_step = Step::FETCH_1; // Starting to execute the interrupt associated code
			break;

		case Step::FETCH_1:
			if (m_flags.INTERRUPT && (m_mb->getINT() || m_softwareInterrupt)) // An interrupt has been triggered and the CPU is ready to handle it
			{
				m_step = Step::INTERRUPT_1;
				
				if (m_softwareInterrupt)
				{
					m_mb->setINR(false);
				}
				else
				{
					m_mb->setINR(true);
				}
			}
			else
			{
				// I : Fetch byte 1 of the next instruction
				_movAddBus(m_programCounter);
				_ramRead();

				m_step = Step::FETCH_2;
			}
			break;

		case Step::FETCH_2:
			// II : Save byte 1 and fetch byte 2 of the next instruction
			m_fetchedInstruction = (uint64_t)(m_mb->getDataBus()) << 32;

			_movAddBus(m_programCounter + 1);
			_ramRead();

			m_step = Step::FETCH_3;
			break;

		case Step::FETCH_3:
			// III : Save byte 2 and fetch byte 3 of the next instruction
			m_fetchedInstruction += (uint64_t)m_mb->getDataBus() << 24;

			_movAddBus(m_programCounter + 2);
			_ramRead();

			m_step = Step::FETCH_4;
			break;

		case Step::FETCH_4:
			// IV : Save byte 3 and fetch byte 4 of the next instruction
			m_fetchedInstruction += (uint64_t)m_mb->getDataBus() << 16;

			_movAddBus(m_programCounter + 3);
			_ramRead();

			m_step = Step::FETCH_5;
			break;

		case Step::FETCH_5:
			// V : Save byte 4 and fetch byte 5 of the next instruction
			m_fetchedInstruction += (uint64_t)m_mb->getDataBus() << 8;

			_movAddBus(m_programCounter + 4);
			_ramRead();

			m_step = Step::DECODE;
			break;

		case Step::DECODE:
			// VI : Save byte 5 of the next instruction and decode it (opcode, addressing mode, operands)
			m_fetchedInstruction += m_mb->getDataBus();

			m_opcode = (uint8_t)((m_fetchedInstruction & 0xFC00000000) >> 34);
			m_addressingMode = (uint8_t)((m_fetchedInstruction & 0x03C0000000) >> 30);
			m_R1 = &m_registers[(m_fetchedInstruction & 0x0038000000) >> 27];
			m_R2 = &m_registers[(m_fetchedInstruction & 0x0007000000) >> 24];
			m_V1 = (uint8_t)((m_fetchedInstruction & 0x0000FF0000) >> 16);
			m_V2 = (m_fetchedInstruction & 0x000000FF00) >> 8;
			m_Ex = m_fetchedInstruction & 0x00000000FF;
			m_Vx = ((uint32_t)(m_V1) << 16) + ((uint32_t)(m_V2) << 8) + (uint32_t)(m_Ex);
			m_R3 = &m_registers[m_V1 & 0x07];
			m_R4 = &m_registers[m_Ex & 0x07];
			m_Rx = ((uint32_t)(*m_R1) << 16) + ((uint32_t)(*m_R2) << 8) + (uint32_t)(*m_R3);

			m_step = Step::EXECUTE;
			m_�codeStep = 0;
			m_�code = �opcodesList::UNDEFINED;
			break;

		case Step::EXECUTE:
			// VII : Execute the instruction
			m_dataBusValue = m_mb->getDataBus();

			if (m_�codeStep >= m_instructionsUCode[m_opcode].addrMode[m_addressingMode].size()) // Instruction fully executed
			{
				m_step = Step::FETCH_1;
				m_�codeStep = 0; // Safety feature

				if (!m_jump)
				{
					m_programCounter += 5;
					if (m_programCounter >= WORK_MEMORY_END_ADDRESS)
						m_programCounter = 0;
				}
				else
					m_jump = false;
			}
			else // �code to execute
			{
				m_�code = m_instructionsUCode[m_opcode].addrMode[m_addressingMode].at(m_�codeStep).uopcode;
				std::vector<�operandsList> operands = m_instructionsUCode[m_opcode].addrMode[m_addressingMode].at(m_�codeStep).uoperands;

				switch (m_�code)
				{
				case �opcodesList::ADC:
					_adc();
					break;

				case �opcodesList::ADD:
					_add();
					break;

				case �opcodesList::SUB:
					_sub();
					break;

				case �opcodesList::AND:
					_and();
					break;

				case �opcodesList::XOR:
					_xor();
					break;

				case �opcodesList::NOT:
					_not();
					break;

				case �opcodesList::OR:
					_or();
					break;

				case �opcodesList::SHR:
					_shr();
					break;

				case �opcodesList::ASR:
					_asr();
					break;

				case �opcodesList::SHL:
					_shl();
					break;

				case �opcodesList::CMP:
					_cmp();
					break;

				case �opcodesList::IN:
					_in();
					break;

				case �opcodesList::OUT:
					_out();
					break;

				case �opcodesList::INT:
					_interrupt();
					break;

				case �opcodesList::CLC:
					_clc();
					break;

				case �opcodesList::CLE:
					_cle();
					break;

				case �opcodesList::CLI:
					_cli();
					break;

				case �opcodesList::CLN:
					_cln();
					break;

				case �opcodesList::CLS:
					_cls();
					break;

				case �opcodesList::CLZ:
					_clz();
					break;

				case �opcodesList::CLF:
					_clf();
					break;

				case �opcodesList::STH:
					_sth();
					break;

				case �opcodesList::STC:
					_stc();
					break;

				case �opcodesList::STI:
					_sti();
					break;

				case �opcodesList::STN:
					_stn();
					break;

				case �opcodesList::STF:
					_stf();
					break;

				case �opcodesList::STS:
					_sts();
					break;

				case �opcodesList::STE:
					_ste();
					break;

				case �opcodesList::STZ:
					_stz();
					break;

				case �opcodesList::DECSTK:
					_decSTK();
					break;

				case �opcodesList::INCSTK:
					_incSTK();
					break;

				case �opcodesList::INCPC:
					_incPC();
					break;

				case �opcodesList::RAMREAD:
					_ramRead();
					break;

				case �opcodesList::RAMWRITE:
					_ramWrite();
					break;

				case �opcodesList::MOVACC1:
					switch (operands[0])
					{
						case �operandsList::ALUOUT:
							_movAcc1(m_aluOut);
							break;

						case �operandsList::DATABUS:
							_movAcc1(m_mb->getDataBus());
							break;

						case �operandsList::R1:
							_movAcc1(*m_R1);
							break;

						case �operandsList::R2:
							_movAcc1(*m_R2);
							break;

						case �operandsList::R4:
							_movAcc1(*m_R4);
							break;

						case �operandsList::V1:
							_movAcc1(m_V1);
							break;
					}
					break;

				case �opcodesList::MOVACC2:
					switch (operands[0])
					{
						case �operandsList::X1:
							_movAcc2(0x1);
							break;

						case �operandsList::ALUOUT:
							_movAcc2(m_aluOut);
							break;

						case �operandsList::DATABUS:
							_movAcc2(m_mb->getDataBus());
							break;

						case �operandsList::R1:
							_movAcc2(*m_R1);
							break;

						case �operandsList::R2:
							_movAcc2(*m_R2);
							break;

						case �operandsList::R4:
							_movAcc2(*m_R4);
							break;

						case �operandsList::V1:
							_movAcc2(m_V1);
							break;
					}
					break;

				case �opcodesList::MOVREG:
					m_pointedRegister = nullptr;
					m_valueToStore = nullptr;

					switch (operands[0])
					{
						case �operandsList::R1:
							m_pointedRegister = m_R1;
							break;

						case �operandsList::R2:
							m_pointedRegister = m_R2;
							break;

						case �operandsList::R4:
							m_pointedRegister = m_R4;
							break;

						case �operandsList::I:
							m_pointedRegister = &m_registers[(int)Registers::I];
							break;
					}

					switch (operands[1])
					{
						case �operandsList::ALUOUT:
							m_valueToStore = &m_aluOut;
							break;

						case �operandsList::DATABUS:
							m_valueToStore = &m_dataBusValue;
							break;

						case �operandsList::R1:
							m_valueToStore = m_R1;
							break;

						case �operandsList::R2:
							m_valueToStore = m_R2;
							break;

						case �operandsList::R4:
							m_valueToStore = m_R4;
							break;

						case �operandsList::V1:
							m_valueToStore = &m_V1;
							break;
					}

					if (m_pointedRegister != nullptr && m_valueToStore != nullptr)
						_movReg(m_pointedRegister, m_valueToStore);
					break;

				case �opcodesList::MOVDATABUS:
					switch (operands[0])
					{
						case �operandsList::ALUOUT:
							_movDataBus(m_aluOut);
							break;

						case �operandsList::R1:
							_movDataBus(*m_R1);
							break;

						case �operandsList::R2:
							_movDataBus(*m_R2);
							break;

						case �operandsList::R4:
							_movDataBus(*m_R4);
							break;

						case �operandsList::V1:
							_movDataBus(m_V1);
							break;

						case �operandsList::PC_16:
							_movDataBus((uint8_t)((m_programCounter & 0xFF0000) >> 16));
							break;

						case �operandsList::PC_8:
							_movDataBus((uint8_t)((m_programCounter & 0x00FF00) >> 8));
							break;

						case �operandsList::PC8:
							_movDataBus((uint8_t)(m_programCounter & 0x0000FF));
							break;
					}
					break;

				case �opcodesList::MOVADDBUS:
					switch (operands[0])
					{
						case �operandsList::V1:
							_movAddBus(m_V1);
							break;

						case �operandsList::R1:
							_movAddBus(*m_R1);
							break;

						case �operandsList::R2:
							_movAddBus(*m_R2);
							break;

						case �operandsList::RX:
							_movAddBus(m_Rx);
							break;

						case �operandsList::VX:
							_movAddBus(m_Vx);
							break;

						case �operandsList::STK:
							_movAddBus(m_stackPointer);
							break;
						}
					break;

				case �opcodesList::MOVPC:
					switch (operands[0])
					{
						case �operandsList::RX:
							_movPC(m_Rx);
							break;

						case �operandsList::VX:
							_movPC(m_Vx);
							break;

						case �operandsList::DATABUS16:
							_movPC(m_mb->getDataBus() << 16);
							break;

						case �operandsList::PCDATABUS8:
							_movPC((m_mb->getDataBus() << 8) + m_programCounter);
							break;

						case �operandsList::PCDATABUS:
							_movPC(m_mb->getDataBus() + m_programCounter);
							break;
					}
					break;

				case �opcodesList::JMC:
					switch (operands[0])
					{
						case �operandsList::RX:
							_jmc(m_Rx);
							break;

						case �operandsList::VX:
							_jmc(m_Vx);
							break;
					}
					break;

				case �opcodesList::JME:
					switch (operands[0])
					{
						case �operandsList::RX:
							_jme(m_Rx);
							break;

						case �operandsList::VX:
							_jme(m_Vx);
							break;
					}
					break;

				case �opcodesList::JMF:
					switch (operands[0])
					{
						case �operandsList::RX:
							_jmf(m_Rx);
							break;

						case �operandsList::VX:
							_jmf(m_Vx);
							break;
					}
					break;

				case �opcodesList::JMK:
					switch (operands[0])
					{
						case �operandsList::RX:
							_jmk(m_Rx);
							break;

						case �operandsList::VX:
							_jmk(m_Vx);
							break;
					}
					break;

				case �opcodesList::JMP:
					switch (operands[0])
					{
						case �operandsList::RX:
							_jmp(m_Rx);
							break;

						case �operandsList::VX:
							_jmp(m_Vx);
							break;
					}
					break;

				case �opcodesList::JMS:
					switch (operands[0])
					{
						case �operandsList::RX:
							_jms(m_Rx);
							break;

						case �operandsList::VX:
							_jms(m_Vx);
							break;
					}
					break;

				case �opcodesList::JMZ:
					switch (operands[0])
					{
						case �operandsList::RX:
							_jmz(m_Rx);
							break;

						case �operandsList::VX:
							_jmz(m_Vx);
							break;
					}
					break;

				case �opcodesList::JMN:
					switch (operands[0])
					{
						case �operandsList::RX:
							_jmn(m_Rx);
							break;

						case �operandsList::VX:
							_jmn(m_Vx);
							break;
					}
					break;
				}

				m_�codeStep++;
			}
			break;
		}
	}
}

// GETTERS
Step CPU::getCurrentStep()
{
	return m_step;
}

std::string CPU::getCurrent�Code()
{
	std::string instruction("");
	std::string addressingMode("");
	std::string �opcode("");

	if (m_step == Step::DECODE)
		return "DECODE";
	else if (m_step == Step::STOP)
		return "STOP";
	else if (m_step == Step::EXECUTE)
	{
		switch (m_opcode)
		{
			case (int)InstructionsList::ADC:
				instruction = "ADC";
				break;

			case (int)InstructionsList::ADD:
				instruction = "ADD";
				break;

			case (int)InstructionsList::AND:
				instruction = "AND";
				break;

			case (int)InstructionsList::CAL:
				instruction = "CAL";
				break;

			case (int)InstructionsList::CLC:
				instruction = "CLC";
				break;

			case (int)InstructionsList::CLE:
				instruction = "CLE";
				break;

			case (int)InstructionsList::CLI:
				instruction = "CLI";
				break;

			case (int)InstructionsList::CLN:
				instruction = "CLN";
				break;

			case (int)InstructionsList::CLS:
				instruction = "CLS";
				break;

			case (int)InstructionsList::CLZ:
				instruction = "CLZ";
				break;

			case (int)InstructionsList::CLF:
				instruction = "CLF";
				break;

			case (int)InstructionsList::CMP:
				instruction = "CMP";
				break;

			case (int)InstructionsList::DEC:
				instruction = "DEC";
				break;

			case (int)InstructionsList::HLT:
				instruction = "HLT";
				break;

			case (int)InstructionsList::IN:
				instruction = "IN";
				break;

			case (int)InstructionsList::OUT:
				instruction = "OUT";
				break;

			case (int)InstructionsList::INC:
				instruction = "INC";
				break;

			case (int)InstructionsList::INT:
				instruction = "INT";
				break;

			case (int)InstructionsList::IRT:
				instruction = "IRT";
				break;

			case (int)InstructionsList::JMC:
				instruction = "JMC";
				break;

			case (int)InstructionsList::JME:
				instruction = "JME";
				break;

			case (int)InstructionsList::JMF:
				instruction = "JMF";
				break;

			case (int)InstructionsList::JMK:
				instruction = "JMK";
				break;

			case (int)InstructionsList::JMP:
				instruction = "JMP";
				break;

			case (int)InstructionsList::JMS:
				instruction = "JMS";
				break;

			case (int)InstructionsList::JMZ:
				instruction = "JMZ";
				break;

			case (int)InstructionsList::JMN:
				instruction = "JMN";
				break;

			case (int)InstructionsList::STR:
				instruction = "STR";
				break;

			case (int)InstructionsList::LOD:
				instruction = "LOD";
				break;

			case (int)InstructionsList::MOV:
				instruction = "MOV";
				break;

			case (int)InstructionsList::NOT:
				instruction = "NOT";
				break;

			case (int)InstructionsList::OR:
				instruction = "OR";
				break;

			case (int)InstructionsList::POP:
				instruction = "POP";
				break;

			case (int)InstructionsList::PSH:
				instruction = "PSH";
				break;

			case (int)InstructionsList::RET:
				instruction = "RET";
				break;

			case (int)InstructionsList::SHL:
				instruction = "SHL";
				break;

			case (int)InstructionsList::ASR:
				instruction = "ASR";
				break;

			case (int)InstructionsList::SHR:
				instruction = "SHR";
				break;

			case (int)InstructionsList::STC:
				instruction = "STC";
				break;

			case (int)InstructionsList::STI:
				instruction = "STI";
				break;

			case (int)InstructionsList::STN:
				instruction = "STN";
				break;

			case (int)InstructionsList::STF:
				instruction = "STF";
				break;

			case (int)InstructionsList::STS:
				instruction = "STS";
				break;

			case (int)InstructionsList::STE:
				instruction = "STE";
				break;

			case (int)InstructionsList::STZ:
				instruction = "STZ";
				break;

			case (int)InstructionsList::SUB:
				instruction = "SUB";
				break;

			case (int)InstructionsList::XOR:
				instruction = "XOR";
				break;

			default:
				return "NOP";
		}

		switch (m_addressingMode)
		{
			case (int)AddressingModesList::IMM24:
				addressingMode = "Imm24";
				break;

			case (int)AddressingModesList::IMM8:
				addressingMode = "Imm8";
				break;

			case (int)AddressingModesList::NONE:
				addressingMode = "None";
				break;

			case (int)AddressingModesList::RAMREG_IMMREG:
				addressingMode = "RamReg/ImmReg";
				break;

			case (int)AddressingModesList::REG:
				addressingMode = "Reg";
				break;

			case (int)AddressingModesList::REG24:
				addressingMode = "Reg24";
				break;

			case (int)AddressingModesList::REG_IMM8:
				addressingMode = "Reg/Imm8";
				break;

			case (int)AddressingModesList::REG_RAM:
				addressingMode = "Reg/Ram";
				break;

			default:
				addressingMode = "Err";
		}

		switch (m_�code)
		{
			case �opcodesList::UNDEFINED:
				�opcode = "";
				break;
				
			case �opcodesList::ADC:
				�opcode = "adc";
				break;

			case �opcodesList::ADD:
				�opcode = "add";
				break;

			case �opcodesList::SUB:
				�opcode = "sub";
				break;

			case �opcodesList::AND:
				�opcode = "and";
				break;

			case �opcodesList::XOR:
				�opcode = "xor";
				break;

			case �opcodesList::NOT:
				�opcode = "not";
				break;

			case �opcodesList::OR:
				�opcode = "or";
				break;

			case �opcodesList::SHR:
				�opcode = "shr";
				break;

			case �opcodesList::SHL:
				�opcode = "shl";
				break;

			case �opcodesList::ASR:
				�opcode = "asr";
				break;

			case �opcodesList::CMP:
				�opcode = "cmp";
				break;

			case �opcodesList::MOVACC1:
				�opcode = "movAcc1";
				break;

			case �opcodesList::MOVACC2:
				�opcode = "movAcc2";
				break;

			case �opcodesList::MOVPC:
				�opcode = "movPC";
				break;

			case �opcodesList::MOVADDBUS:
				�opcode = "movAddBus";
				break;

			case �opcodesList::MOVDATABUS:
				�opcode = "movDataBus";
				break;

			case �opcodesList::MOVREG:
				�opcode = "movReg";
				break;

			case �opcodesList::RAMREAD:
				�opcode = "ramRead";
				break;

			case �opcodesList::RAMWRITE:
				�opcode = "ramWrite";
				break;

			case �opcodesList::DECSTK:
				�opcode = "decStk";
				break;

			case �opcodesList::INCSTK:
				�opcode = "incStk";
				break;

			case �opcodesList::INCPC:
				�opcode = "incPC";
				break;

			case �opcodesList::IN:
				�opcode = "in";
				break;

			case �opcodesList::OUT:
				�opcode = "out";
				break;

			case �opcodesList::INT:
				�opcode = "int";
				break;

			case �opcodesList::CLC:
				�opcode = "clc";
				break;

			case �opcodesList::CLE:
				�opcode = "cle";
				break;

			case �opcodesList::CLI:
				�opcode = "cli";
				break;

			case �opcodesList::CLN:
				�opcode = "cln";
				break;

			case �opcodesList::CLS:
				�opcode = "cls";
				break;

			case �opcodesList::CLZ:
				�opcode = "clz";
				break;

			case �opcodesList::CLF:
				�opcode = "clf";
				break;

			case �opcodesList::STH:
				�opcode = "sth";
				break;

			case �opcodesList::STC:
				�opcode = "stc";
				break;

			case �opcodesList::STI:
				�opcode = "sti";
				break;

			case �opcodesList::STN:
				�opcode = "stn";
				break;

			case �opcodesList::STF:
				�opcode = "stf";
				break;

			case �opcodesList::STS:
				�opcode = "sts";
				break;

			case �opcodesList::STE:
				�opcode = "ste";
				break;

			case �opcodesList::STZ:
				�opcode = "stz";
				break;

			case �opcodesList::JMC:
				�opcode = "jmc";
				break;

			case �opcodesList::JME:
				�opcode = "jme";
				break;

			case �opcodesList::JMF:
				�opcode = "jmf";
				break;

			case �opcodesList::JMK:
				�opcode = "jmk";
				break;

			case �opcodesList::JMP:
				�opcode = "jmp";
				break;

			case �opcodesList::JMS:
				�opcode = "jms";
				break;

			case �opcodesList::JMZ:
				�opcode = "jmz";
				break;

			case �opcodesList::JMN:
				�opcode = "jmn";
				break;
		}

		return instruction + " - " + addressingMode + " : " + �opcode;
	}
	else
		return "FETCH";
}

uint8_t CPU::getStackPointer()
{
	return m_stackPointer;
}

uint32_t CPU::getProgramCounter()
{
	return m_programCounter;
}

std::string CPU::getFlagsRegister()
{
	std::string carry(m_flags.CARRY ? "1  " : "0  ");
	std::string zero(m_flags.ZERO ? "1  " : "0  ");
	std::string halt(m_flags.HALT ? "1  " : "0  ");
	std::string negative(m_flags.NEGATIVE ? "1  " : "0  ");
	std::string inferior(m_flags.INFERIOR ? "1  " : "0  ");
	std::string superior(m_flags.SUPERIOR ? "1  " : "0  ");
	std::string equal(m_flags.EQUAL ? "1  " : "0  ");
	std::string interrupt(m_flags.INTERRUPT ? "1" : "0");

	return carry + zero + halt + negative + inferior + superior + equal + interrupt;
}

uint8_t CPU::getAcc1()
{
	return m_accu1;
}

uint8_t CPU::getAcc2()
{
	return m_accu2;
}

std::string CPU::get�op()
{
	switch (m_�code) // ALU operations only
	{
		case �opcodesList::ADC:
			return "adc";
			break;

		case �opcodesList::ADD:
			return "add";
			break;

		case �opcodesList::AND:
			return "and";
			break;

		case �opcodesList::ASR:
			return "asr";
			break;

		case �opcodesList::NOT:
			return "not";
			break;

		case �opcodesList::OR:
			return "or";
			break;

		case �opcodesList::SHL:
			return "shl";
			break;

		case �opcodesList::SHR:
			return "shr";
			break;

		case �opcodesList::SUB:
			return "sub";
			break;

		case �opcodesList::XOR:
			return "xor";
			break;

		default:
			return "";
	}
}

uint8_t CPU::getAluOut()
{
	return m_aluOut;
}

uint8_t CPU::getRegA()
{
	return m_registers[(int)Registers::A];
}

uint8_t CPU::getRegB()
{
	return m_registers[(int)Registers::B];
}

uint8_t CPU::getRegC()
{
	return m_registers[(int)Registers::C];
}

uint8_t CPU::getRegD()
{
	return m_registers[(int)Registers::D];
}

uint8_t CPU::getRegI()
{
	return m_registers[(int)Registers::I];
}

uint8_t CPU::getRegJ()
{
	return m_registers[(int)Registers::J];
}

uint8_t CPU::getRegX()
{
	return m_registers[(int)Registers::X];
}

uint8_t CPU::getRegY()
{
	return m_registers[(int)Registers::Y];
}


// PRIVATE
void CPU::init�code()
{
	uInstruction temp;

	// === ADC ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::R2 };
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::ADC;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// -- Reg/Imm8 --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::V1 };
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::ADC;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	// -- Reg/Ram --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::ADC;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::ADC].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	// === ADD ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::R2 };
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::ADD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// -- Reg/Imm8 --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::V1 };
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::ADD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	// -- Reg/Ram --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::ADD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::ADD].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	// === AND ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::R2 };
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::AND;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// -- Reg/Imm8 --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::V1 };
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::AND;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	// -- Reg/Ram --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::AND;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::AND].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	// === CAL ===
	// -- Reg24 --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::PC8 };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::INCSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::PC_8 };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::INCSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::PC_16 };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::INCSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVPC;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	// -- Imm24 --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::PC8 };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::INCSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::PC_8 };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::INCSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::PC_16 };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::INCSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVPC;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::CAL].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	// === CLC ===
	// -- None --
	temp.uopcode = �opcodesList::CLC;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CLC].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === CLE ===
	// -- None --
	temp.uopcode = �opcodesList::CLE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CLE].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === CLI ===
	// -- None --
	temp.uopcode = �opcodesList::CLI;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CLI].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === CLN ===
	// -- None --
	temp.uopcode = �opcodesList::CLN;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CLN].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === CLS ===
	// -- None --
	temp.uopcode = �opcodesList::CLS;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CLS].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === CLZ ===
	// -- None --
	temp.uopcode = �opcodesList::CLZ;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CLZ].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === CLF ===
	// -- None --
	temp.uopcode = �opcodesList::CLF;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CLF].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === CMP ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::CMP].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::R2 };
	m_instructionsUCode[(int)InstructionsList::CMP].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::CMP;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CMP].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// -- Reg/Imm8 --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::CMP].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::V1 };
	m_instructionsUCode[(int)InstructionsList::CMP].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::CMP;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CMP].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	// -- Reg/Ram --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::CMP].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands = { �operandsList::V1 };
	m_instructionsUCode[(int)InstructionsList::CMP].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::CMP].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R4 };
	m_instructionsUCode[(int)InstructionsList::CMP].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::CMP;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::CMP].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);
	
	// === DEC ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::X1 };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::SUB;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::REG].push_back(temp);
	
	// -- Reg24 --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::X1 };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::SUB;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	// -- Imm24 --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::X1 };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::SUB;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::DEC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	// === HLT ===
	// -- None --
	temp.uopcode = �opcodesList::STH;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::HLT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === IN ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::R2 };
	m_instructionsUCode[(int)InstructionsList::IN].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::IN;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::IN].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::IN].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// === OUT ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::R2 };
	m_instructionsUCode[(int)InstructionsList::OUT].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::OUT].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::OUT;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::OUT].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// === INC ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::X1 };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::ADD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// -- Reg24 --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::X1 };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::ADD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	// -- Imm24 --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);
	
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::X1 };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::ADD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::INC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	// === INT ===
	// -- Imm8 --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::V1 };
	m_instructionsUCode[(int)InstructionsList::INT].addrMode[(int)AddressingModesList::IMM8].push_back(temp);

	temp.uopcode = �opcodesList::INT;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::INT].addrMode[(int)AddressingModesList::IMM8].push_back(temp);

	// === IRT ===
	// -- None --
	temp.uopcode = �opcodesList::STI;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::DECSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK };
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::I, �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::DECSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK};
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVPC;
	temp.uoperands = { �operandsList::DATABUS16 };
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::DECSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK};
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVPC;
	temp.uoperands = { �operandsList::PCDATABUS8 };
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::DECSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK};
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVPC;
	temp.uoperands = { �operandsList::PCDATABUS };
	m_instructionsUCode[(int)InstructionsList::IRT].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === JMC ===
	// -- Reg24 --
	temp.uopcode = �opcodesList::JMC;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::JMC].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	// -- Imm24 --
	temp.uopcode = �opcodesList::JMC;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::JMC].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	// === JME ===
	// -- Reg24 --
	temp.uopcode = �opcodesList::JME;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::JME].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	// -- Imm24 --
	temp.uopcode = �opcodesList::JME;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::JME].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	// === JMF ===
	// -- Reg24 --
	temp.uopcode = �opcodesList::JMF;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::JMF].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	// -- Imm24 --
	temp.uopcode = �opcodesList::JMF;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::JMF].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	// === JMK ===
	// -- Reg24 --
	temp.uopcode = �opcodesList::JMK;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::JMK].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	// -- Imm24 --
	temp.uopcode = �opcodesList::JMK;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::JMK].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	// === JMP ===
	// -- Reg24 --
	temp.uopcode = �opcodesList::JMP;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::JMP].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	// -- Imm24 --
	temp.uopcode = �opcodesList::JMP;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::JMP].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	// === JMS ===
	// -- Reg24 --
	temp.uopcode = �opcodesList::JMS;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::JMS].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	// -- Imm24 --
	temp.uopcode = �opcodesList::JMS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::JMS].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	// === JMZ ===
	// -- Reg24 --
	temp.uopcode = �opcodesList::JMZ;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::JMZ].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	// -- Imm24 --
	temp.uopcode = �opcodesList::JMZ;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::JMZ].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	// === JMN ===
	// -- Reg24 --
	temp.uopcode = �opcodesList::JMN;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::JMN].addrMode[(int)AddressingModesList::REG24].push_back(temp);

	// -- Imm24 --
	temp.uopcode = �opcodesList::JMN;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::JMN].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	// === STR ===
	// -- RamReg / ImmReg --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::STR].addrMode[(int)AddressingModesList::RAMREG_IMMREG].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::R4 };
	m_instructionsUCode[(int)InstructionsList::STR].addrMode[(int)AddressingModesList::RAMREG_IMMREG].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::STR].addrMode[(int)AddressingModesList::RAMREG_IMMREG].push_back(temp);

	// -- Reg / Ram --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::STR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::STR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::STR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	// === LOD ===
	// -- RamReg / ImmReg --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::RX };
	m_instructionsUCode[(int)InstructionsList::LOD].addrMode[(int)AddressingModesList::RAMREG_IMMREG].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::LOD].addrMode[(int)AddressingModesList::RAMREG_IMMREG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R4, �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::LOD].addrMode[(int)AddressingModesList::RAMREG_IMMREG].push_back(temp);

	// -- Reg / Ram --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::LOD].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::LOD].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::LOD].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	// === MOV ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::R2 };
	m_instructionsUCode[(int)InstructionsList::MOV].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// -- Reg / Imm8 --
	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::V1 };
	m_instructionsUCode[(int)InstructionsList::MOV].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	// === NOT ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::NOT].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::NOT;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::NOT].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::NOT].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// -- Imm24 --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::NOT].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::NOT].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::NOT].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::NOT;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::NOT].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::NOT].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::NOT].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::NOT].addrMode[(int)AddressingModesList::IMM24].push_back(temp);

	// === OR ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::R2 };
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::OR;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// -- Reg / Imm8 --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::V1 };
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::OR;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	// -- Reg / Ram --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::OR;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::OR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	// === POP ===
	// -- Reg --
	temp.uopcode = �opcodesList::DECSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::POP].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK};
	m_instructionsUCode[(int)InstructionsList::POP].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::POP].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::POP].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// === PSH ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK};
	m_instructionsUCode[(int)InstructionsList::PSH].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVDATABUS;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::PSH].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::RAMWRITE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::PSH].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::INCSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::PSH].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// === RET ===
	// -- None --
	temp.uopcode = �opcodesList::DECSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK};
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVPC;
	temp.uoperands = { �operandsList::DATABUS16 };
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::DECSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK};
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVPC;
	temp.uoperands = { �operandsList::PCDATABUS8 };
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::DECSTK;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::STK};
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::MOVPC;
	temp.uoperands = { �operandsList::PCDATABUS };
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	temp.uopcode = �opcodesList::INCPC;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::RET].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === SHL ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::SHL].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::SHL;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::SHL].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::SHL].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// === ASR ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::ASR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::ASR;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::ASR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::ASR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// === SHR ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::SHR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::SHR;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::SHR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::SHR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// === STC ===
	// -- None --
	temp.uopcode = �opcodesList::STC;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::STC].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === STI ===
	// -- None --
	temp.uopcode = �opcodesList::STI;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::STI].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === STN ===
	// -- None --
	temp.uopcode = �opcodesList::STN;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::STN].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === STF ===
	// -- None --
	temp.uopcode = �opcodesList::STF;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::STF].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === STS ===
	// -- None --
	temp.uopcode = �opcodesList::STS;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::STS].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === STE ===
	// -- None --
	temp.uopcode = �opcodesList::STE;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::STE].addrMode[(int)AddressingModesList::NONE].push_back(temp);

	// === STZ ===
	// -- None --
	temp.uopcode = �opcodesList::STZ;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::STZ].addrMode[(int)AddressingModesList::NONE].push_back(temp);
	
	// === SUB ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::R2 };
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::SUB;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// -- Reg/Imm8 --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::V1 };
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::SUB;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	// -- Reg/Ram --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::SUB;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::SUB].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	// === XOR ===
	// -- Reg --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG].push_back(temp);
	
	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::R2 };
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::XOR;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG].push_back(temp);

	// -- Reg/Imm8 --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::V1 };
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::XOR;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG_IMM8].push_back(temp);

	// -- Reg/Ram --
	temp.uopcode = �opcodesList::MOVACC1;
	temp.uoperands = { �operandsList::R1 };
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVADDBUS;
	temp.uoperands = { �operandsList::VX };
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::RAMREAD;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVACC2;
	temp.uoperands = { �operandsList::DATABUS };
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::XOR;
	temp.uoperands.clear();
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);

	temp.uopcode = �opcodesList::MOVREG;
	temp.uoperands = { �operandsList::R1, �operandsList::ALUOUT };
	m_instructionsUCode[(int)InstructionsList::XOR].addrMode[(int)AddressingModesList::REG_RAM].push_back(temp);
}

// �pocodes
void CPU::_movAcc1(uint8_t v)
{
	m_accu1 = v;
}

void CPU::_movAcc2(uint8_t v)
{
	m_accu2 = v;
}

void CPU::_movPC(uint32_t v)
{
	m_programCounter = v & 0x00FFFFFF;
	m_jump = true;
}

void CPU::_movAddBus(uint32_t v)
{
	m_mb->setAddressBus(v & 0x00FFFFFF);
}

void CPU::_movDataBus(uint8_t v)
{
	m_mb->setDataBus(v);
}

void CPU::_movReg(uint8_t* reg, uint8_t* v)
{
	*reg = *v;
}

void CPU::_ramRead()
{
	m_mb->setRW(false); // Read
	m_mb->setRE(true); // RAM Enable
}

void CPU::_ramWrite()
{
	m_mb->setRW(true); // Write
	m_mb->setRE(true); // RAM Enable
}

void CPU::_decSTK()
{
	m_stackPointer--;
}

void CPU::_incSTK()
{
	m_stackPointer++;
}

void CPU::_incPC()
{
	m_programCounter += 0x5;

	if (m_programCounter >= WORK_MEMORY_END_ADDRESS)
		m_programCounter = 0;
}

void CPU::_clc()
{
	m_flags.CARRY = false;
}

void CPU::_cle()
{
	m_flags.EQUAL = false;
}

void CPU::_cli()
{
	m_flags.INTERRUPT = false;
}

void CPU::_cln()
{
	m_flags.NEGATIVE = false;
}

void CPU::_cls()
{
	m_flags.SUPERIOR = false;
}

void CPU::_clz()
{
	m_flags.ZERO = false;
}

void CPU::_clf()
{
	m_flags.INFERIOR = false;
}

void CPU::_sth()
{
	m_flags.HALT = true;
}

void CPU::_stc()
{
	m_flags.CARRY = true;
}

void CPU::_sti()
{
	m_flags.INTERRUPT = true;
}

void CPU::_stn()
{
	m_flags.NEGATIVE = true;
}

void CPU::_stf()
{
	m_flags.INFERIOR = true;
}

void CPU::_sts()
{
	m_flags.SUPERIOR = true;
}

void CPU::_ste()
{
	m_flags.EQUAL = true;
}

void CPU::_stz()
{
	m_flags.ZERO = true;
}

void CPU::_jmc(uint32_t addr)
{
	if (m_flags.CARRY)
	{
		m_programCounter = addr & 0x00FFFFFF;
		m_jump = true;
	}
}

void CPU::_jme(uint32_t addr)
{
	if (m_flags.EQUAL)
	{
		m_programCounter = addr & 0x00FFFFFF;
		m_jump = true;
	}
}

void CPU::_jmf(uint32_t addr)
{
	if (m_flags.INFERIOR)
	{
		m_programCounter = addr & 0x00FFFFFF;
		m_jump = true;
	}
}

void CPU::_jmk(uint32_t addr)
{
	m_programCounter += addr;

	if (m_programCounter > WORK_MEMORY_END_ADDRESS)
	{
		m_programCounter = 0;
	}

	m_jump = true;
}

void CPU::_jmp(uint32_t addr)
{
	m_programCounter = addr & 0x00FFFFFF;

	m_jump = true;
}

void CPU::_jms(uint32_t addr)
{
	if (m_flags.SUPERIOR)
	{
		m_programCounter = addr & 0x00FFFFFF;
		m_jump = true;
	}
}

void CPU::_jmz(uint32_t addr)
{
	if (m_flags.ZERO)
	{
		m_programCounter = addr & 0x00FFFFFF;
		m_jump = true;
	}
}

void CPU::_jmn(uint32_t addr)
{
	if (m_flags.NEGATIVE)
	{
		m_programCounter = addr & 0x00FFFFFF;
		m_jump = true;
	}
}

void CPU::_in()
{
	m_mb->setRW(false);
	m_mb->setIE(true);
}

void CPU::_out()
{
	m_mb->setRW(true);
	m_mb->setIE(true);
}

void CPU::_interrupt()
{
	m_softwareInterrupt = true;
}

// ALU OPERATIONS
void CPU::_adc()
{
	m_aluOut = m_accu1 + m_accu2 + 1;

	m_flags.CARRY = (((int)m_accu1 + (int)m_accu2 + 1) > 0xFF) ? true : false;
	m_flags.ZERO = (m_aluOut == 0x00) ? true : false;
	m_flags.NEGATIVE = ((m_aluOut & 0x80) == 0x00) ? false : true; // Sign bit
}

void CPU::_add()
{
	m_aluOut = m_accu1 + m_accu2;

	m_flags.CARRY = (((int)m_accu1 + (int)m_accu2) > 0xFF) ? true : false;
	m_flags.ZERO = (m_aluOut == 0x00) ? true : false;
	m_flags.NEGATIVE = ((m_aluOut & 0x80) == 0x00) ? false : true; // Sign bit
}

void CPU::_sub()
{
	m_aluOut = m_accu1 - m_accu2;

	m_flags.CARRY = (((int)m_accu1 + (int)m_accu2) > 0xFF) ? true : false;
	m_flags.ZERO = (m_aluOut == 0x00) ? true : false;
	m_flags.NEGATIVE = ((m_aluOut & 0x80) == 0x00) ? false : true; // Sign bit
}

void CPU::_and()
{
	m_aluOut = m_accu1 & m_accu2;

	m_flags.ZERO = (m_aluOut == 0x00) ? true : false;
	m_flags.NEGATIVE = ((m_aluOut & 0x80) == 0x00) ? false : true; // Sign bit
}

void CPU::_or()
{
	m_aluOut = m_accu1 | m_accu2;

	m_flags.ZERO = (m_aluOut == 0x00) ? true : false;
	m_flags.NEGATIVE = ((m_aluOut & 0x80) == 0x00) ? false : true; // Sign bit
}

void CPU::_xor()
{
	m_aluOut = m_accu1 ^ m_accu2;

	m_flags.ZERO = (m_aluOut == 0x00) ? true : false;
	m_flags.NEGATIVE = ((m_aluOut & 0x80) == 0x00) ? false : true; // Sign bit
}

void CPU::_not()
{
	m_aluOut = ~m_accu1;

	m_flags.ZERO = (m_aluOut == 0x00) ? true : false;
	m_flags.NEGATIVE = ((m_aluOut & 0x80) == 0x00) ? false : true; // Sign bit
}

void CPU::_shl()
{
	m_aluOut = m_accu1 << 1;

	m_flags.CARRY = (((int)m_accu1 << 1) > 0xFF) ? true : false;
	m_flags.ZERO = (m_aluOut == 0x00) ? true : false;
	m_flags.NEGATIVE = ((m_aluOut & 0x80) == 0x00) ? false : true; // Sign bit
}

void CPU::_asr()
{
	m_aluOut = m_accu1 >> 1;

	if ((m_accu1 & 0x80) == 0x80) // If the sign bit is 1, it must keep it
	{
		m_aluOut |= (0x01 << 7); // This code differentiates arithmetical shift from logical shift
	}

	m_flags.CARRY = (((int)m_accu1 << 1) > 0xFF) ? true : false;
	m_flags.ZERO = (m_aluOut == 0x00) ? true : false;
	m_flags.NEGATIVE = ((m_aluOut & 0x80) == 0x00) ? false : true;
}

void CPU::_shr()
{
	m_aluOut = m_accu1 >> 1;

	m_flags.CARRY = (((int)m_accu1 << 1) > 0xFF) ? true : false;
	m_flags.ZERO = (m_aluOut == 0x00) ? true : false;
	m_flags.NEGATIVE = ((m_aluOut & 0x80) == 0x00) ? false : true;
}

void CPU::_cmp()
{
	m_flags.ZERO = (m_accu1 == 0x00) ? true : false;
	m_flags.EQUAL = (m_accu1 == m_accu2) ? true : false;
	m_flags.INFERIOR = (m_accu1 < m_accu2) ? true : false;
	m_flags.SUPERIOR = (m_accu1 > m_accu2) ? true : false;
}

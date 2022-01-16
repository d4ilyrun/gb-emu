#pragma once

#include "cpu.h"
#include "utils/types.h"
#include "memory.h"

/*
 * Run the next instruction at PC
 */
u8 execute_instruction();

typedef enum instruction_name {
    IN_ERR,
    IN_NOP,
    IN_LD,
    IN_PUSH,
    IN_POP,
    IN_JP,
    IN_JR,
    IN_CALL,
    IN_RET,
    IN_RETI,
    IN_RST,
    IN_HALT,
    IN_STOP,
    IN_DI,
    IN_EI,
    IN_CCF,
    IN_SCF,
    IN_DAA,
    IN_CPL,
    IN_ADD,
    IN_ADC,
    IN_SUB,
    IN_SBC,
    IN_INC,
    IN_DEC,
    IN_MUL,
    IN_AND,
    IN_OR,
    IN_XOR,
} in_name;

/*
 * Different types of operands possible for an instruction.
 *
 * Meaning:
 * - R8/16: 8/16-bit register
 * - D8/D16: 16-bit immediate data
 * - A16: 16-bit address
 * - U8: 8-bit unsigned data, which are added to $FF00 in certain instructions
 * - S8: 8-bit signed data, which are added to PC
 * - FLAG: condition ...
 *
 * Specific register:
 * - HL: HL register
 * - A: A register
 * - C: C register
 *
 * Prefixes:
 * - REL: relative addressing
 */
typedef enum operand_type {
    ERR_OPERAND,
    NO_OPERAND,

    // Only one operand
    R8,
    R16,
    A16,
    HL_IMM,
    S8,
    FLAG,
    RST, // special case: data in opcode

    // Two operands
    FLAG_A16,
    FLAG_S8,
} operand_type;

#define HAS_CONDITION(_in) ((_in).type == FLAG_S8 || (_in).type == FLAG_A16)

/*
 * A struct representing an instruction and its operands.
 * To simply things we also include the instruction's cycle count and address.
 *
 * An instruction operand can be:
 * - 1 or 2 registers (16 or 8 bit)
 * - An immediate value (16 or 8 bit)
 * - An address (16-bit immediate value)
 * - A condition
 */
struct instruction {
    in_name instruction;
    operand_type type;
    u16 pc;

    // Operand values
    cpu_register_name reg1;
    cpu_register_name reg2;
    u16 address;
    bool condition;
    u16 data;

    // Cycle count
    u8 cycle_count;
    u8 cycle_count_false; // For conditional jumps
};

/*
* (MSB -> LSB)
* x = the opcode's 1st octal digit (i.e. bits 7-6)
* y = the opcode's 2nd octal digit (i.e. bits 5-3)
* z = the opcode's 3rd octal digit (i.e. bits 2-0)
* p = y rightshifted one position (i.e. bits 5-4)
* q = y modulo 2 (i.e. bit 3)
*/

#if __BYTE_ORDER == __BIG_ENDIAN
    #define OPCODE_X(_opcode) ((_opcode) >> 6)
    #define OPCODE_Y(_opcode) (((_opcode) >> 3) & 0x07)
    #define OPCODE_Z(_opcode) ((_opcode) & 0x07)
    #define OPCODE_Q(_opcode) (((_opcode) >> 3) & 0x01)
    #define OPCODE_P(_opcode) (((_opcode) >> 4) & 0x03)
#else
    #define OPCODE_X(_opcode) ((_opcode) & 0x03)
    #define OPCODE_Y(_opcode) (((_opcode) >> 2) & 0x07)
    #define OPCODE_Z(_opcode) ((_opcode) >> 5)
    #define OPCODE_Q(_opcode) (((_opcode) >> 4) & 0x01)
    #define OPCODE_P(_opcode) (((_opcode) >> 2) & 0x03)
#endif // __BIG_ENDIAN__

struct instruction fetch_instruction(u8 opcode);
void display_instruction(struct instruction in);

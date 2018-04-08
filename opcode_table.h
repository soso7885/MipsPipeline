#ifndef _OPCODE_TABLE_H
#define _OPCODE_TABLE_H

#define R_FORMAT	0
#define I_FORMAT	1
#define J_FORMAT	2

#define SZ_WORD		4
#define SZ_HALFWORD	2
#define SZ_BYTE		1

//XXX: Opmized it, MR. boring!
#define BELOW_ALU(i) (i == OP_SLL 	|| i == OP_SRL	|| \
					i == OP_SRA 	|| i == OP_MFHI || \
					i == OP_MFLO 	|| i == OP_MULT	|| \
					i == OP_MULTU 	|| i == OP_DIV	|| \
					i == OP_DIVU	|| i == OP_ADD	|| \
					i == OP_ADDU	|| i == OP_SUB	|| \
					i == OP_SUBU	|| i == OP_AND	|| \
					i == OP_OR		|| i == OP_XOR	|| \
					i == OP_NOR		|| i == OP_SLT	|| \
					i == OP_SLTU	|| i == OP_ADDI || \
					i == OP_ADDIU	|| i == OP_SLTI || \
					i == OP_SLTIU	|| i == OP_ANDI || \
					i == OP_ORI		|| i == OP_LUI	|| \
					i == OP_LW		|| i == OP_LBU	|| \
					i == OP_LHU 	|| i == OP_SB	|| \
					i == OP_SB		|| i == OP_SH	|| \
					i == OP_SW)

#define BELOW_JUMP(i) (i == OP_JR || i == OP_JUMP || i == OP_JAL)

#define BELOW_BRANCH(i) (i == OP_BNE || i == OP_BEQ)

#define LOAD_STORE_DATA_SIZE(i) ((i == OP_LW || i == OP_SW) ? SZ_WORD :	\
							((i == OP_LHU || i == OP_SH) ? SZ_HALFWORD: SZ_BYTE))

#define GET_MEM_DIRECTION(i) ((i == OP_LW || i == OP_LBU || i == OP_LHU) ? MEM_LOAD : MEM_STORE)

#define BELOW_LOAD_STORE(i) (i == OP_LW	|| i == OP_LBU	|| \
							i == OP_LHU || i == OP_SB	|| \
							i == OP_SB	|| i == OP_SH	|| \
							i == OP_SW)

enum {
	/*---- R ----*/
	OP_SLL = 0,
	OP_SRL,
	OP_SRA,
	OP_JR,
	OP_MFHI,
	OP_MFLO,
	OP_MULT,
	OP_MULTU,
	OP_DIV,
	OP_DIVU,
	OP_ADD,
	OP_ADDU,
	OP_SUB,
	OP_SUBU,
	OP_AND,
	OP_OR,
	OP_XOR,
	OP_NOR,
	OP_SLT,
	OP_SLTU,
	/*---- J ----*/
	OP_JUMP,
	OP_JAL,
	/*---- I ----*/
	OP_BEQ,
	OP_BNE,
	OP_ADDI,
	OP_ADDIU,
	OP_SLTI,
	OP_SLTIU,
	OP_ANDI,
	OP_ORI,
	OP_LUI,
	OP_MFC0,	//no support
	OP_LW,
	OP_LBU,
	OP_LHU,
	OP_SB,
	OP_SH,
	OP_SW,
	/*---- END ----*/
	OP_END,
};

struct opcode_lookup_table {
	uint8_t type;
	/* 
	 * The R type opcode is 0, omiited it,
	 * just save funct
	 */
	union {
		uint8_t opcode;
		uint8_t funct;
	} codes;
	const char *mnemonic;
	const char *describe;
	void (*alu_fn) (uint32_t *, uint32_t, uint32_t);
};

static inline void fn_sll(uint32_t *out, uint32_t in1, uint32_t in2)
{
	*out = in1 << in2;
}

static inline void fn_srl(uint32_t *out, uint32_t in1, uint32_t in2)
{
	*out = in1 >> in2;
}

static inline void fn_sra(uint32_t *out, uint32_t in1, uint32_t in2)
{
	/* 
	 * Arithmetic shift right
	 * If signed bit = 1, need fillin 1
	*/
	if(in1 & 0x80000000){
		*out = (in1 >> in2);
		for(int i = 0; i < in2; --i){
			*out |= (1 << (32-i));
		}
	}else{
		*out = in1 >> in2;
	} 
}

static inline void fn_mult(uint32_t *out, uint32_t in1, uint32_t in2)
{
	*out = in1 * in2;
}

static inline void fn_div(uint32_t *out, uint32_t in1, uint32_t in2)
{
	*out = in1 / in2;
}

static inline void fn_add(uint32_t *out, uint32_t in1, uint32_t in2)
{
	*out = in1 + in2;
}

static inline void fn_sub(uint32_t *out, uint32_t in1, uint32_t in2)
{
	*out = in1 - in2;
}

static inline void fn_and(uint32_t *out, uint32_t in1, uint32_t in2)
{
	*out = in1 & in2;
}

static inline void fn_or(uint32_t *out, uint32_t in1, uint32_t in2)
{
	*out = in1 | in2;
}

static inline void fn_xor(uint32_t *out, uint32_t in1, uint32_t in2)
{
	*out = in1 ^ in2;
}

static inline void fn_nor(uint32_t *out, uint32_t in1, uint32_t in2)
{
	*out = ~(in1 | in2);
}

static inline void fn_slt(uint32_t *out, uint32_t in1, uint32_t in2)
{
	*out = in1 < in2 ? 1 : 0;
}

static inline void fn_addi(uint32_t *out, uint32_t in, uint32_t imm)
{
	*out = in + imm;
}

static inline void fn_slti(uint32_t *out, uint32_t in, uint32_t imm)
{
	*out = in < imm ? 1 : 0;
}

static inline void fn_andi(uint32_t *out, uint32_t in, uint32_t imm)
{
	*out = in & imm;
}

static inline void fn_ori(uint32_t *out, uint32_t in, uint32_t imm)
{
	*out = in | imm;
}

static inline void fn_lui(uint32_t *out, uint32_t in, uint32_t imm)
{
	*out = imm * 65536;	// not sure
}

static inline void fn_calc_addr_signed(uint32_t *out, uint32_t in, uint32_t imm)
{
	*out = in + ((int32_t)imm);
}

static inline void fn_calc_addr_unsigned(uint32_t *out, uint32_t in, uint32_t imm)
{
	*out = in + imm;
}

struct opcode_lookup_table Opcode_table[] = {
	[OP_SLL] = {
		.type = R_FORMAT,
		.codes.funct = 0x00,
		.mnemonic = "sll",
		.alu_fn = fn_sll,
		.describe = "Logical shift left",
	},

	[OP_SRL] = {
		.type = R_FORMAT,
		.codes.funct = 0x02,
		.mnemonic = "srl",
		.alu_fn = fn_srl,
		.describe = "Logical shift right (0-extended)",
	},

	[OP_SRA] = {
		.type = R_FORMAT,
		.codes.funct = 0x03,
		.mnemonic = "sra",
		.alu_fn = fn_sra,
		.describe = "Arithmetic shift right (sign-extended)",
	},

	[OP_JR] = {
		.type = R_FORMAT,
		.codes.funct = 0x08,
		.mnemonic = "jr",
		.alu_fn = NULL,
		.describe = "Jump to address in register",
	}, 

	[OP_MFHI] = {
		.type = R_FORMAT,
		.codes.funct = 0x10,
		.mnemonic = "mfhi",
		.alu_fn = NULL,
		.describe = "Move from Hi register",
	},

	[OP_MFLO] = {
		.type = R_FORMAT,
		.codes.funct = 0x12,
		.mnemonic = "mflo",
		.alu_fn = NULL,
		.describe = "Move from Lo register",
	},

	[OP_MULT] = {
		.type = R_FORMAT,
		.codes.funct = 0x18,
		.mnemonic = "mult",
		.alu_fn = fn_mult,
		.describe = "Multiply",
	},

	[OP_MULTU] = {
		.type = R_FORMAT,
		.codes.funct = 0x19,
		.mnemonic = "multu",
		.alu_fn = fn_mult,
		.describe = "Unsigned multiply",
	},

	[OP_DIV] = {
		.type = R_FORMAT,
		.codes.funct = 0x1A,
		.mnemonic = "div",
		.alu_fn = fn_div,
		.describe = "Divide",
	}, 

	[OP_DIVU] = {
		.type = R_FORMAT,
		.codes.funct = 0x1B,
		.mnemonic = "divu",
		.alu_fn = fn_div,
		.describe = "Unsigned divide",
	}, 

	[OP_ADD] = {
		.type = R_FORMAT,
		.codes.funct = 0x20,
		.mnemonic = "add",
		.alu_fn = fn_add,
		.describe = "Add",
	},

	[OP_ADDU] = {
		.type = R_FORMAT,
		.codes.funct = 0x21,
		.mnemonic = "addu",
		.alu_fn = fn_add,
		.describe = "Add unsigned",
	},

	[OP_SUB] = {
		.type = R_FORMAT,
		.codes.funct = 0x22,
		.mnemonic = "sub",
		.alu_fn = fn_sub,
		.describe = "Subtract",
	},

	[OP_SUBU] = {
		.type = R_FORMAT,
		.codes.funct = 0x23,
		.mnemonic = "subu",
		.alu_fn = fn_sub,
		.describe = "Subtract",
	},

	[OP_AND] = {
		.type = R_FORMAT,
		.codes.funct = 0x24,
		.mnemonic = "and",
		.alu_fn = fn_and,
		.describe = "Bitwise AND",
	}, 

	[OP_OR] = {
		.type = R_FORMAT,
		.codes.funct = 0x25,
		.mnemonic = "or",
		.alu_fn = fn_or,
		.describe = "Bitwise OR",
	},

	[OP_XOR] = {
		.type = R_FORMAT,
		.codes.funct = 0x26,
		.mnemonic = "xor",
		.alu_fn = fn_xor,
		.describe = "Bitwise XOR",
	},

	[OP_NOR] = {
		.type = R_FORMAT,
		.codes.funct = 0x27,
		.mnemonic = "nor",
		.alu_fn = fn_nor,
		.describe = "Bitwise NOR",
	},

	[OP_SLT] = {
		.type = R_FORMAT,
		.codes.funct = 0x2A,
		.mnemonic = "slt",
		.alu_fn = fn_slt,
		.describe = "Set to 1 if less than",
	},

	[OP_SLTU] = {
		.type = R_FORMAT,
		.codes.funct = 0x2B,
		.mnemonic = "sltu",
		.alu_fn = fn_slt,
		.describe = "Set to 1 if less than unsigned",
	},

	[OP_JUMP] = {
		.type = J_FORMAT,
		.codes.opcode = 0x02,
		.mnemonic = "j",
		.alu_fn = NULL,
		.describe = "Jump to address",
	},

	[OP_JAL] = {
		.type = J_FORMAT,
		.codes.opcode = 0x03,
		.mnemonic = "jal",
		.alu_fn = NULL,
		.describe = "Jump and link",
	},

	[OP_BEQ] = {
		.type = I_FORMAT,
		.codes.opcode = 0x04,
		.mnemonic = "beq",
		.alu_fn = NULL,
		.describe = "Branch if equal",
	},

	[OP_BNE] = {
		.type = I_FORMAT,
		.codes.opcode = 0x05,
		.mnemonic = "bne",
		.alu_fn = NULL,
		.describe = "Branch if not equal",
	},

	[OP_ADDI] = {
		.type = I_FORMAT,
		.codes.opcode = 0x08,
		.mnemonic = "ADDI",
		.alu_fn = fn_addi,
		.describe = "Add immediate",
	},

	[OP_ADDIU] = {
		.type = I_FORMAT,
		.codes.opcode = 0x09,
		.mnemonic = "addiu",
		.alu_fn = fn_addi,
		.describe = "Add unsigned immediate",
	},

	[OP_SLTI] = {
		.type = I_FORMAT,
		.codes.opcode = 0x0A,
		.mnemonic = "slti",
		.alu_fn = fn_slti,
		.describe = "Set to 1 if less than immediate",
	},

	[OP_SLTIU] = {
		.type = I_FORMAT,
		.codes.opcode = 0x0B,
		.mnemonic = "sltiu",
		.alu_fn = fn_slti,
		.describe = "Set to 1 if less than unsigned immediate",
	},

	[OP_ANDI] = {
		.type = I_FORMAT,
		.codes.opcode = 0x0C,
		.mnemonic = "andi",
		.alu_fn = fn_andi,
		.describe = "Bitwise unsinged immediate",
	},

	[OP_ORI] = {
		.type = I_FORMAT,
		.codes.opcode = 0x0D,
		.mnemonic = "ori",
		.alu_fn = fn_ori,
		.describe = "Bitwise OR immediate",
	},

	[OP_LUI] = {
		.type = I_FORMAT,
		.codes.opcode = 0x0F,
		.mnemonic = "lui",
		.alu_fn = fn_lui,
		.describe = "Load upper immediate",
	},

	[OP_MFC0] = {	// no support
		.type = R_FORMAT,
		.codes.opcode = 0x10,
		.mnemonic = "mfc0",
		.alu_fn = NULL,
		.describe = "Move from Coprocessor 0 (no support now)"
	},

	[OP_LW] = {
		.type = I_FORMAT,
		.codes.opcode = 0x23,
		.mnemonic = "lw",
		.alu_fn = fn_calc_addr_signed,
		.describe = "Load word(4-byte)",
	},

	[OP_LBU] = {
		.type = I_FORMAT,
		.codes.opcode = 0x024,
		.mnemonic = "lbu",
		.alu_fn = fn_calc_addr_unsigned,
		.describe = "Load one byte unsigned",
	},

	[OP_LHU] = {
		.type = I_FORMAT,
		.codes.opcode = 0x025,
		.mnemonic = "lhu",
		.alu_fn = fn_calc_addr_unsigned,
		.describe = "Load halfword(2-byte) unsigned",
	},

	[OP_SB] = {
		.type = I_FORMAT,
		.codes.opcode = 0x28,
		.mnemonic = "sb",
		.alu_fn = fn_calc_addr_signed,
		.describe = "Store one byte",
	},

	[OP_SH] = {
		.type = I_FORMAT,
		.codes.opcode = 0x29,
		.mnemonic = "sh",
		.alu_fn = fn_calc_addr_signed,
		.describe = "Store halfword(2-byte)",
	},

	[OP_SW] = {
		.type = I_FORMAT,
		.codes.opcode = 0x2B,
		.mnemonic = "sw",
		.alu_fn = fn_calc_addr_signed,
		.describe = "Store word(4-byte)",
	},	
};
#endif
	

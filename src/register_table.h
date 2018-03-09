#ifndef _REGISTER_TABLE_H
#define _REGISTER_TABLE_H

enum {
	REG_R0 = 0, 
	REG_R1, 
	REG_R2, 
	REG_R3, 
	REG_R4,
	REG_R5, 
	REG_R6, 
	REG_R7, 
	REG_R8, 
	REG_R9,
	REG_R10, 
	REG_R11, 
	REG_R12, 
	REG_R13, 
	REG_R14,
	REG_R15, 
	REG_R16, 
	REG_R17, 
	REG_R18, 
	REG_R19,
	REG_R20, 
	REG_R21, 
	REG_R22, 
	REG_R23, 
	REG_R24,
	REG_R25, 
	REG_R26, 
	REG_R27, 
	REG_R28, 
	REG_R29,
	REG_R30, 
	REG_R31,
	/*---- END ----*/
	REG_END,
};

struct mips_register_table {
	uint8_t reg_number;
	uint32_t reg_data;
	const char *reg_name;
	const char *reg_describe;
};

struct _mips_registers {
	uint32_t pc;
	struct mips_register_table reg_table[];
};

struct _mips_registers Mips_registers = {
	.pc = 0,
	.reg_table = {
		[REG_R0] = {
			.reg_number = 0x00,
			.reg_data = 0,
			.reg_name = "$zero",
			.reg_describe = "Hard-wired to 0"
		},
		[REG_R1] = {
			.reg_number = 0x01,
			.reg_data = 0,
			.reg_name = "$at",
			.reg_describe = "Reserved for pseudo-instructions"
		},
		[REG_R2] = {
			.reg_number = 0x02,
			.reg_data = 0,
			.reg_name = "$v0",
			.reg_describe = "The register for function return value"
		},
		[REG_R3] = {
			.reg_number = 0x03,
			.reg_data = 0,
			.reg_name = "$v1",
			.reg_describe = "The register for function return value"
		},
		[REG_R4] = {
			.reg_number = 0x04,
			.reg_data = 0,
			.reg_name = "$a0",
			.reg_describe = "The register for function arguments"
		},
		[REG_R5] = {
			.reg_number = 0x05,
			.reg_data = 0,
			.reg_name = "$a1",
			.reg_describe = "The register for function arguments"
		},
		[REG_R6] = {
			.reg_number = 0x06,
			.reg_data = 0,
			.reg_name = "$a2",
			.reg_describe = "The register for function arguments"
		},
		[REG_R7] = {
			.reg_number = 0x07,
			.reg_data = 0,
			.reg_name = "$a3",
			.reg_describe = "The register for function arguments"
		},
		[REG_R8] = {
			.reg_number = 0x08,
			.reg_data = 0,
			.reg_name = "$t0",
			.reg_describe = "The register for temporary data"
		},
		[REG_R9] = {
			.reg_number = 0x09,
			.reg_data = 0,
			.reg_name = "$t1",
			.reg_describe = "The register for temporary data"
		},
		[REG_R10] = {
			.reg_number = 0x0A,
			.reg_data = 0,
			.reg_name = "$t2",
			.reg_describe = "The register for temporary data"
		},
		[REG_R11] = {
			.reg_number = 0x0B,
			.reg_data = 0,
			.reg_name = "$t3",
			.reg_describe = "The register for temporary data"
		},
		[REG_R12] = {
			.reg_number = 0x0C,
			.reg_data = 0,
			.reg_name = "$t4",
			.reg_describe = "The register for temporary data"
		},
		[REG_R13] = {
			.reg_number = 0x0D,
			.reg_data = 0,
			.reg_name = "$t5",
			.reg_describe = "The register for temporary data"
		},
		[REG_R14] = {
			.reg_number = 0x0E,
			.reg_data = 0,
			.reg_name = "$t6",
			.reg_describe = "The register for temporary data"
		},
		[REG_R15] = {
			.reg_number = 0x0F,
			.reg_data = 0,
			.reg_name = "$t7",
			.reg_describe = "The register for temporary data"
		},
		[REG_R16] = {
			.reg_number = 0x10,
			.reg_data = 0,
			.reg_name = "$s0",
			.reg_describe = "The register for saved"
		},
		[REG_R17] = {
			.reg_number = 0x11,
			.reg_data = 0,
			.reg_name = "$s1",
			.reg_describe = "The register for saved"
		},
		[REG_R18] = {
			.reg_number = 0x12,
			.reg_data = 0,
			.reg_name = "$s2",
			.reg_describe = "The register for saved"
		},
		[REG_R19] = {
			.reg_number = 0x13,
			.reg_data = 0,
			.reg_name = "$s3",
			.reg_describe = "The register for saved"
		},
		[REG_R20] = {
			.reg_number = 0x14,
			.reg_data = 0,
			.reg_name = "$s4",
			.reg_describe = "The register for saved"
		},
		[REG_R21] = {
			.reg_number = 0x15,
			.reg_data = 0,
			.reg_name = "$s5",
			.reg_describe = "The register for saved"
		},
		[REG_R22] = {
			.reg_number = 0x16,
			.reg_data = 0,
			.reg_name = "$s6",
			.reg_describe = "The register for saved"
		},
		[REG_R23] = {
			.reg_number = 0x17,
			.reg_data = 0,
			.reg_name = "$s7",
			.reg_describe = "The register for saved",
		},
		[REG_R24] = {
			.reg_number = 0x18,
			.reg_data = 0,
			.reg_name = "$t8",
			.reg_describe = "The register for temporary data",
		},
		[REG_R25] = {
			.reg_number = 0x19,
			.reg_data = 0,
			.reg_name = "$t9",
			.reg_describe = "The register for temporary data",
		},
		[REG_R26] = {
			.reg_number = 0x1A,
			.reg_data = 0,
			.reg_name = "$k0",
			.reg_describe = "Reserved for Kernel"
		},
		[REG_R27] = {
			.reg_number = 0x1B,
			.reg_data = 0,
			.reg_name = "$k1",
			.reg_describe = "reserved for Kernel"
		},
		[REG_R28] = {
			.reg_number = 0x1C,
			.reg_data = 0,
			.reg_name = "$gp",
			.reg_describe = "Global area pointer"
		},
		[REG_R29] = {
			.reg_number = 0x1D,
			.reg_data = 0,
			.reg_name = "$sp",
			.reg_describe = "Stack pointer"
		},
		[REG_R30] = {
			.reg_number = 0x1E,
			.reg_data = 0,
			.reg_name = "$fp",
			.reg_describe = "Frame pointer"
		},
		[REG_R31] = {
			.reg_number = 0x1F,
			.reg_data = 0,
			.reg_name = "$ra",
			.reg_describe = "Return address"
		},
		[REG_END] = {
			.reg_number = 0xff,
			.reg_data = 0,
			.reg_name = "Error!",
			.reg_describe = "cannot recognize register!"
		},
	},
};
#endif

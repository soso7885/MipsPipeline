#ifndef _REGISTER_TABLE_H
#define _REGISTER_TABLE_H

#include <stdio.h>

#define REG_INIT					0
#define REG_NEED_UPDATE_FROM_IE		1
#define REG_NEED_UPDATE_STALL		2
#define REG_NEED_UPDATE_FROM_MEM	3

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
	uint32_t data;
	int state;
	const char *name;
	const char *alias;
	const char *describe;
};

struct _mips_registers {
	uint32_t pc;
	struct mips_register_table reg_table[];
};

struct _mips_registers Mips_registers = {
	.pc = 0,
	.reg_table = {
		[REG_R0] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$zero",
			.alias = "r0",
			.describe = "Hard-wired to 0"
		},
		[REG_R1] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$at",
			.alias = "r1",
			.describe = "Reserved for pseudo-instructions"
		},
		[REG_R2] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$v0",
			.alias = "r2",
			.describe = "The register for function return value"
		},
		[REG_R3] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$v1",
			.alias = "r3",
			.describe = "The register for function return value"
		},
		[REG_R4] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$a0",
			.alias = "r4",
			.describe = "The register for function arguments"
		},
		[REG_R5] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$a1",
			.alias = "r5",
			.describe = "The register for function arguments"
		},
		[REG_R6] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$a2",
			.alias = "r6",
			.describe = "The register for function arguments"
		},
		[REG_R7] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$a3",
			.alias = "r7",
			.describe = "The register for function arguments"
		},
		[REG_R8] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$t0",
			.alias = "r8",
			.describe = "The register for temporary data"
		},
		[REG_R9] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$t1",
			.alias = "r9",
			.describe = "The register for temporary data"
		},
		[REG_R10] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$t2",
			.alias = "r10",
			.describe = "The register for temporary data"
		},
		[REG_R11] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$t3",
			.alias = "r11",
			.describe = "The register for temporary data"
		},
		[REG_R12] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$t4",
			.alias = "r12",
			.describe = "The register for temporary data"
		},
		[REG_R13] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$t5",
			.alias = "r13",
			.describe = "The register for temporary data"
		},
		[REG_R14] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$t6",
			.alias = "r14",
			.describe = "The register for temporary data"
		},
		[REG_R15] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$t7",
			.alias = "r15",
			.describe = "The register for temporary data"
		},
		[REG_R16] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$s0",
			.alias = "r16",
			.describe = "The register for saved"
		},
		[REG_R17] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$s1",
			.alias = "r17",
			.describe = "The register for saved"
		},
		[REG_R18] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$s2",
			.alias = "r18",
			.describe = "The register for saved"
		},
		[REG_R19] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$s3",
			.alias = "r19",
			.describe = "The register for saved"
		},
		[REG_R20] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$s4",
			.alias = "r20",
			.describe = "The register for saved"
		},
		[REG_R21] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$s5",
			.alias = "r21",
			.describe = "The register for saved"
		},
		[REG_R22] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$s6",
			.alias = "r22",
			.describe = "The register for saved"
		},
		[REG_R23] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$s7",
			.alias = "r23",
			.describe = "The register for saved",
		},
		[REG_R24] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$t8",
			.alias = "r24",
			.describe = "The register for temporary data",
		},
		[REG_R25] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$t9",
			.alias = "r25",
			.describe = "The register for temporary data",
		},
		[REG_R26] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$k0",
			.alias = "r26",
			.describe = "Reserved for Kernel"
		},
		[REG_R27] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$k1",
			.alias = "r27",
			.describe = "reserved for Kernel"
		},
		[REG_R28] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$gp",
			.alias = "r28",
			.describe = "Global area pointer"
		},
		[REG_R29] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$sp",
			.alias = "r29",
			.describe = "Stack pointer"
		},
		[REG_R30] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$fp",
			.alias = "r30",
			.describe = "Frame pointer"
		},
		[REG_R31] = {
			.data = 0,
			.state = REG_INIT,
			.name = "$ra",
			.alias = "r31",
			.describe = "Return address"
		},
		[REG_END] = {
			.data = 0,
			.state = REG_INIT,
			.name = "Error!",
			.alias = "Error",
			.describe = "cannot recognize register!"
		},
	},
};

static inline void register_dump(void)
{
	printf("Register dump:\n");
	printf("---------------------------------------------------------------------\n");
	for(int i = 0; i < REG_END; i+=4){
		printf("%s(%s): 0x%0x\t",
					Mips_registers.reg_table[i].name,
					Mips_registers.reg_table[i].alias,
					Mips_registers.reg_table[i].data);
		printf("%s(%s): 0x%0x\t",
					Mips_registers.reg_table[i+1].name,
					Mips_registers.reg_table[i+1].alias,
					Mips_registers.reg_table[i+1].data);
		printf("%s(%s): 0x%0x\t",
					Mips_registers.reg_table[i+2].name,
					Mips_registers.reg_table[i+2].alias,
					Mips_registers.reg_table[i+2].data);
		printf("%s(%s): 0x%0x\n",
					Mips_registers.reg_table[i+3].name,
					Mips_registers.reg_table[i+3].alias,
					Mips_registers.reg_table[i+3].data);

	}
	printf("---------------------------------------------------------------------\n");
}
#endif

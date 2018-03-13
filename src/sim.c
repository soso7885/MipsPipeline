#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>

#include "opcode_table.h"
#include "register_table.h"
#include "linkedlist.h"

#define POOLSIZE	512

#define PIPELINE_BYPASS			0
#define PIPELINE_GOTHROW_MEM	1
#define PIPELINE_GOTHROW_WB		2

/*
 * Get the function address to be the data
*/
uint64_t dummy_data[] = {
	(uint64_t)&exit,
	(uint64_t)&scanf,
	(uint64_t)&fopen,
	(uint64_t)&fclose,
	(uint64_t)&fputc,
	(uint64_t)&fgetc,
	(uint64_t)&perror,
	(uint64_t)&fflush,
};

//XXX: use linkedlist to implement it, MR. Boring
uint8_t pool[POOLSIZE] = {0};

/* The memory dump linkedlist */
struct node *head = NULL;

int nr_inst = 0;
volatile bool done = false;

union instruction {
	uint32_t raw;
	/*
	 * In GCC-4.4, we have to set the
	 * no-alignment key word to compiler,
	 * because shamt/rd/rt/rs is odd bits
	*/
	struct {	// 6+5+5+5+5+6 = 32
		uint8_t funct : 6;
		uint8_t shamt : 5;
		uint8_t rd : 5;
		uint8_t rt : 5;
		uint8_t rs : 5;
		uint8_t opcode : 6;
	}__attribute__((packed)) r_inst;
	struct {	// 16+5+5+6 = 32
		uint16_t imm;		
		uint8_t rt : 5;
		uint8_t rs : 5;
		uint8_t opcode : 6;
	}__attribute__((packed)) i_inst;
	struct {	// 26+6 = 32
		uint32_t addr : 26;
		uint8_t opcode : 6;
	}j_inst;
};

struct fetch_buffer {
	bool stop;
	union instruction inst;
};
 
struct decode_buffer {
	int type;
	bool stop;
	/* 
	 * In realworld, should use signal to decide which opration in ALU 
	 * not use optable_index
	*/
	int optable_index;
	uint8_t opcode;
	uint32_t rsval;	
	uint32_t rtval;
	uint32_t imm;
	uint8_t rd;	// destination register;
	uint8_t shamt;
	uint8_t funct;
	uint32_t j_addr; // jump address
};

#define SZ_WORD	8
#define SZ_HALFWORD	4
#define SZ_BYTE	1

struct excute_buffer {
	int type;
	bool stop;
	int signal;		// to decide the pipeline path
	int sz_access;	// the sizeof data which in memory
	bool direction;
	uint8_t rd;		// destination register;
	uint32_t result;
	uint32_t mem_addr;
};

struct memory_buffer {
	int type;
	bool stop;
	int signal;
	uint8_t rd;
	uint32_t result;
};

struct pipeline_buffer {
	struct fetch_buffer IFbuf;
	struct decode_buffer IDbuf;
	struct excute_buffer IEbuf;
	struct memory_buffer MEMbuf;
} pipeline_buf;

static void _ignore_comment(char *buf, size_t buflen, FILE **fp)
{
	do{
		for(int i = 0; i < buflen; ++i){
			if(buf[i] == '\n') return;
		}
	}while(fgets(buf, buflen, *fp));
}

static void parser(const char *path)
{
	uint32_t *inst;
	char tmp[32] = {0};
	FILE *fin;
	
	fin = fopen(path, "r");
	assert(fin);

	while(fgets(tmp, sizeof(tmp), fin)){
		// to ignore comment
		if(tmp[0] == '#' || tmp[0] == '\n' || tmp[0] == ' '){
			_ignore_comment(tmp, sizeof(tmp), &fin);
			memset(tmp, 0, sizeof(tmp));
			continue;
		}

		inst = (uint32_t *)&pool[(nr_inst*sizeof(uint32_t))];
		sscanf(tmp, "%u", inst);
		memset(tmp, 0, sizeof(tmp));
		nr_inst++;
	}

	fclose(fin);
}

static int _lookup_opcode(union instruction *inst)
{
	int targ;
	// the opcode bitwise is same in every format
	uint8_t opcode = inst->r_inst.opcode;

	if(opcode == 0){	// R type
		uint8_t funct = inst->r_inst.funct;

		for(targ = 0; targ < OP_END; ++targ){
			if(Opcode_table[targ].codes.funct == funct)
				break;
		}
	}else{	// I/J type
		for(targ = OP_JUMP; targ < OP_END; ++targ){	// the I/J table start at OP_JUMP 
			if(Opcode_table[targ].codes.opcode == opcode)
				break;
		}
	}
	
	return targ;
}

static inline const char *_get_rs(union instruction *inst)
{
	for(int i = 0; i < REG_END; ++i){
		if(inst->r_inst.rs == Mips_registers.reg_table[i].reg_number)
			return Mips_registers.reg_table[i].reg_name;
	}
	
	return "NULL";
}

static inline const char *_get_rt(union instruction *inst)
{
	for(int i = 0; i < REG_END; ++i){
		if(inst->r_inst.rt == Mips_registers.reg_table[i].reg_number)
			return Mips_registers.reg_table[i].reg_name;
	}
	
	return "NULL";
}

static inline const char *_get_rd(union instruction *inst)
{
	for(int i = 0; i < REG_END; ++i){
		if(inst->r_inst.rd == Mips_registers.reg_table[i].reg_number)
			return Mips_registers.reg_table[i].reg_name;
	}
	
	return "NULL";
}

#define GET_REGNAME(inst, reg) _get_##reg(inst)
#define GET_REGVAL(i) Mips_registers.reg_table[i].reg_data

/*
 * -- Fetch --
 * 		fecthing the instruction from file to memory
*/
static void STAGE_fetch(void)
{
	union instruction *inst;
	struct fetch_buffer *pIFbuf = &pipeline_buf.IFbuf;
	uint32_t *pc = &Mips_registers.pc;

	if(nr_inst == 0){
		pIFbuf->stop = true;
		return;
	}

	inst = (union instruction *)&pool[*pc];
	*pc += sizeof(union instruction);
	nr_inst--;

	printf("<IF> instruction(%d), ", inst->raw);

	/* for debug, in real world, fetch will not distinguish the format */
	if(inst->r_inst.opcode == 0){
		printf("R-type: opcode(%x) rs(%x) rt(%x) rd(%x) shmat(%x) funct(%x)\n",
				inst->r_inst.opcode, inst->r_inst.rs, inst->r_inst.rt,
				inst->r_inst.rd, inst->r_inst.shamt, inst->r_inst.funct);
	}else if(inst->r_inst.opcode == 0x02 || inst->r_inst.opcode == 0x03){
		printf("J-type: opcode(%x), address(%x)\n",
				inst->j_inst.opcode, inst->j_inst.addr);	
	}else{
		printf("I-type: opcode(%x) rs(%x) rt(%x) imm(%x)\n",
				inst->i_inst.opcode, inst->i_inst.rs,
				inst->i_inst.rt, inst->i_inst.imm);
	}

	memcpy(&pIFbuf->inst, inst, sizeof(union instruction));
}

#define COPYDATA_FROM_PREV_STAGE_BUFFER(dest, src, size) memcpy(dest, &pipeline_buf.src, size)

/*
 * -- Decode --
 * 		decode the instruction and grab the data from register
*/
static void STAGE_decode(void)
{
	int index;
	struct fetch_buffer copied;
	struct decode_buffer *pIDbuf = &pipeline_buf.IDbuf;

	COPYDATA_FROM_PREV_STAGE_BUFFER(&copied, IFbuf, sizeof(copied));
	
	if(copied.stop){
		pIDbuf->stop = true;
		return;
	}
	/*
	 * Do previous pipeline
	*/
	STAGE_fetch();

	// start to decode
	index = _lookup_opcode(&copied.inst);
	if(index == OP_END){
		fprintf(stderr, "%s: cannot recognize instruction(%d)\n",
						__func__, copied.inst.raw);
		return;
	}

	int type = Opcode_table[index].type;
	// store into register

	pIDbuf->type = type;
	pIDbuf->optable_index = index;

	printf("<ID> ");
	switch(type){
		case R_FORMAT:
			pIDbuf->opcode = copied.inst.r_inst.opcode;
			pIDbuf->rsval = GET_REGVAL(copied.inst.r_inst.rs);
			pIDbuf->rtval = GET_REGVAL(copied.inst.r_inst.rt);
			pIDbuf->rd = copied.inst.r_inst.rd;
			pIDbuf->shamt = copied.inst.r_inst.shamt;
			pIDbuf->funct = copied.inst.r_inst.funct;
			printf("[R] %s %s %s %s\n",
						Opcode_table[index].mnemonic,
						GET_REGNAME(&copied.inst, rd), 
						GET_REGNAME(&copied.inst, rs),
						GET_REGNAME(&copied.inst, rt));
			break;
		case I_FORMAT:
			pIDbuf->opcode = copied.inst.i_inst.opcode;
			pIDbuf->rsval = GET_REGVAL(copied.inst.i_inst.rs);
			pIDbuf->rtval = GET_REGVAL(copied.inst.i_inst.rt);
			pIDbuf->imm = copied.inst.i_inst.imm;
			pIDbuf->rd = copied.inst.i_inst.rt;
			printf("[I] %s %s %s, %d\n", 
						Opcode_table[index].mnemonic, 
						GET_REGNAME(&copied.inst, rt), 
						GET_REGNAME(&copied.inst, rs), 
						copied.inst.i_inst.imm);
			break;
		case J_FORMAT:
			pIDbuf->opcode = copied.inst.j_inst.opcode;
			pIDbuf->j_addr = copied.inst.j_inst.addr;
			printf("[J] %s 0x%0x\n",
						Opcode_table[index].mnemonic,
						copied.inst.j_inst.addr);
			break;
	}
}

static uint32_t ALU(uint32_t in1, uint32_t in2, int index)
{
	uint32_t ans = 0;

	if(Opcode_table[index].alu_fn){
		Opcode_table[index].alu_fn(&ans, in1, in2);
	}else{
		printf("Sorry, %s is not support yet!\n",
					Opcode_table[index].mnemonic);
	}
	
	return ans;
}

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

/* Load/Store instruction also need ALU */
#define BELOW_LOAD_STORE(i) (i == OP_LW	|| i == OP_LBU	|| \
							i == OP_LHU || i == OP_SB	|| \
							i == OP_SB	|| i == OP_SH	|| \
							i == OP_SW)

#define LOAD_STORE_DATA_SIZE(i) ((i == OP_LW || i == OP_SW) ? SZ_WORD :	\
							((i == OP_LHU || i == OP_SH) ? SZ_HALFWORD: SZ_BYTE))

#define GET_MEM_DIRECTION(i) ((i == OP_LW || i == OP_LBU || i == OP_LHU) ? MEM_LOAD : MEM_STORE)
/*
 * -- Excute --
 * 		ALU, jump or branch
*/
static void STAGE_excute(void)
{
	struct decode_buffer copied;
	struct excute_buffer *pIEbuf = &pipeline_buf.IEbuf;

	COPYDATA_FROM_PREV_STAGE_BUFFER(&copied, IDbuf, sizeof(copied));
	if(copied.stop){
		pIEbuf->stop = true;
		return;
	}

	/* Do previous stage */
	STAGE_decode();

	printf("<IE> ");

	// clean signal
	pIEbuf->signal = PIPELINE_BYPASS;
	if(BELOW_ALU(copied.optable_index)){	// need ALU
		uint32_t res;

		if(copied.type == R_FORMAT){
			res = ALU(copied.rsval, copied.rtval, copied.optable_index);
		}else{
			res = ALU(copied.rsval, copied.imm, copied.optable_index);
		}

		pIEbuf->signal |= PIPELINE_GOTHROW_WB;
		pIEbuf->rd = copied.rd;

		/* Load/Store method, need to update memory access address */
		if(BELOW_LOAD_STORE(copied.optable_index)){
			pIEbuf->signal |= PIPELINE_GOTHROW_MEM;
			pIEbuf->mem_addr = res;
			pIEbuf->sz_access = LOAD_STORE_DATA_SIZE(copied.optable_index);
			pIEbuf->direction = GET_MEM_DIRECTION(copied.optable_index);
			/* For store inst, stored the rt val into memory */
			pIEbuf->result = copied.rtval;
			printf("do Load/Store memory\n");
		}else{
			pIEbuf->result = res;
			printf("do ALU\n");
		}
	}else if(BELOW_JUMP(copied.optable_index)){	// Jump method
		uint32_t jump_targ = Mips_registers.pc;

		switch(copied.optable_index){
			case OP_JR:
				jump_targ = copied.rsval;
				
				break;
			case OP_JUMP:
				jump_targ += (sizeof(union instruction) + copied.j_addr/4);

				break;
			case OP_JAL:
				jump_targ += (sizeof(union instruction) + copied.j_addr/4);
				pIEbuf->result = copied.j_addr;
				pIEbuf->rd = REG_R31;
				pIEbuf->signal |= PIPELINE_GOTHROW_WB;

				break;
		}
		Mips_registers.pc = jump_targ;	// update PC
		printf("do jump\n");
	}else if(BELOW_BRANCH(copied.optable_index)){	// branch method
		if(copied.optable_index == OP_BEQ){
			if(copied.rsval == copied.rtval){
				Mips_registers.pc += (sizeof(union instruction)+(copied.imm << 2));
			}
		}else{
			if(copied.rsval != copied.rtval){
				Mips_registers.pc += (sizeof(union instruction)+(copied.imm << 2));
			}
		}
		printf("do branch\n");
	}else{
		printf("Sorry, %s (%s) is not support now!\n",
				Opcode_table[copied.optable_index].mnemonic,
				Opcode_table[copied.optable_index].describe);
		// TODO: flushing
	}
}

#define GET_WORD_DUMMY_DATA(addr) (dummy_data[(addr % 7)] & 0xffffffff)
#define GET_HALFWORD_DUMMY_DATA(addr) (dummy_data[(addr % 7)] & 0x0000ffff)
#define GET_BYTE_DUMMY_DATA(addr) (dummy_data[(addr % 7)] & 0x000000ff)

#define GET_DUMMY_DATA(addr, size)	\
	(size == 8 ? GET_WORD_DUMMY_DATA(addr) : (size == 4 ? GET_HALFWORD_DUMMY_DATA(addr) : GET_BYTE_DUMMY_DATA(addr)))

/*
 * --- Memory access ---
 *		load/store data from/to memory,
*/
static void STAGE_memory(void)
{
	struct excute_buffer copied;
	struct memory_buffer *pMEMbuf = &pipeline_buf.MEMbuf;

	COPYDATA_FROM_PREV_STAGE_BUFFER(&copied, IEbuf, sizeof(copied));
	if(copied.stop){
		pMEMbuf->stop = true;
		return;
	}

	/* do previous pipeline stage */
	STAGE_excute();
	
	pMEMbuf->type = copied.type;
	pMEMbuf->signal = copied.signal;
	pMEMbuf->rd = copied.rd;
	pMEMbuf->result = copied.result;

	if(!(copied.signal & PIPELINE_GOTHROW_MEM))
		return;	

	printf("<MEM> ");

	if(copied.direction == MEM_LOAD){	// load
		pMEMbuf->result = GET_DUMMY_DATA(copied.mem_addr, copied.sz_access);
		ll_add_node(&head, copied.mem_addr, pMEMbuf->result, MEM_LOAD);
		printf("Load 0x%0x from 0x%0x\n", pMEMbuf->result, copied.mem_addr);
	}else{	// store
		ll_add_node(&head, copied.mem_addr, pMEMbuf->result, MEM_STORE);
		printf("Store 0x%0x to 0x%0x\n", pMEMbuf->result, copied.mem_addr);
	}
}

/*
 * --- Write back ---
 *		Write the result to target register
*/
static void STAGE_writeback(void)
{
	struct memory_buffer copied;
	
	COPYDATA_FROM_PREV_STAGE_BUFFER(&copied, MEMbuf, sizeof(copied));

	if(copied.stop){
		done = true;
		return;
	}
	/* do previous pipeline stage */
	STAGE_memory();
	
	if(!(copied.signal & PIPELINE_GOTHROW_WB))
		return;

	printf("<WB> ");
	
	Mips_registers.reg_table[copied.rd].reg_data = copied.result;	
	printf("write %s = 0x%0x\n", 
		Mips_registers.reg_table[copied.rd].reg_name,
		Mips_registers.reg_table[copied.rd].reg_data);
}

#define DUMP_MEMORY()	ll_show_all(head)

int main(int argc, char **argv)
{
	if(argc < 2){
		printf("Usage: %s [input file]\n", argv[0]);
		return -1;
	}

	parser(argv[1]);

	STAGE_fetch();
	STAGE_decode();
	STAGE_excute();
	STAGE_memory();
	STAGE_writeback();
	
	while(!done)
		STAGE_writeback();

	DUMP_MEMORY();

	ll_free_all_node(head);
	
	return 0;
}

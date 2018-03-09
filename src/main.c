#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>

#include "opcode_table.h"
#include "register_table.h"

#define POOLSIZE	512

#define DEBUG_PRINTFN() printf("-- %s --\n", __func__);

//TODO: use linkedlist to implement it
uint8_t pool[POOLSIZE] = {0};

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
	union instruction inst;
};
 
struct decode_buffer {
	int type;
	/* 
	 * In realworld, should use signal to decide which opration in ALU 
	 * not use optable_index
	*/
	int optable_index;
	uint8_t opcode;
	uint32_t rsval;
	uint32_t rtval;
	uint32_t rdval;
	uint8_t save_destination;
	uint8_t shamt;
	uint8_t funct;
	uint16_t imm;
	uint32_t addr;	// jump address
};

struct excute_buffer {
	int type;
	bool signal;	// to decide enter pipeline 'memory' or not
	uint8_t save_destination;
	uint32_t result;
};

struct pipeline_buffer {
	struct fetch_buffer IFbuf;
	struct decode_buffer IDbuf;
	struct excute_buffer IEbuf;
} pipeline_buf;

static void _ignore_comment(char *buf, size_t buflen, FILE **fp)
{
	do{
		for(int i = 0; i < buflen; ++i){
			if(buf[i] == '\n')
				return;
		}
	}while(fgets(buf, buflen, *fp));
}

static void parser(const char *path)
{
	uint32_t *inst;
	int ofst;
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

		inst = (uint32_t *)&pool[ofst];
		sscanf(tmp, "%u", inst);
		memset(tmp, 0, sizeof(tmp));
		ofst += sizeof(uint32_t);
	}

	fclose(fin);
}

static int _lookup_opcode(union instruction *inst)
{
	int targ;
	// the opcode bitwise is same in every format
	uint8_t opcode = inst->r_inst.opcode;

	if(opcode == 0){	//R type
		uint8_t funct = inst->r_inst.funct;

		for(targ = 0; targ < OP_END; ++targ){
			if(Opcode_table[targ].codes.funct == funct)
				break;
		}
	}else{	//I/J type
		for(targ = 0; targ < OP_END; ++targ){
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

static void fetch(void)
{
	union instruction *inst;
	uint32_t *pc = &Mips_registers.pc;

	DEBUG_PRINTFN();

	inst = (union instruction *)&pool[*pc];
	*pc += sizeof(union instruction);

	printf("fetch instruction: %d\n", inst->raw);

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

	memcpy(&(pipeline_buf.IFbuf), inst, sizeof(union instruction));
}

#define COPYDATA_FROM_PREV_STAGE_BUFFER(dest, src, size)	memcpy(dest, &pipeline_buf.src, size)

static void decode(void)
{
	int index;
	struct fetch_buffer copied;
	struct decode_buffer *pIDbuf = &pipeline_buf.IDbuf;

	DEBUG_PRINTFN();

	COPYDATA_FROM_PREV_STAGE_BUFFER(&copied, IFbuf, sizeof(copied));

	//TODO: add fetch here

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

	switch(type){
		case R_FORMAT:
			pIDbuf->opcode = copied.inst.r_inst.opcode;
			pIDbuf->rsval = GET_REGVAL(copied.inst.r_inst.rs);
			pIDbuf->rtval = GET_REGVAL(copied.inst.r_inst.rt);
			pIDbuf->rdval = GET_REGVAL(copied.inst.r_inst.rd);
			pIDbuf->save_destination = copied.inst.r_inst.rd;
			pIDbuf->shamt = copied.inst.r_inst.shamt;
			pIDbuf->funct = copied.inst.r_inst.funct;
			printf("<Decode>: [R] %s %s %s %s\n",
					Opcode_table[index].mnemonic,
					GET_REGNAME(&copied.inst, rd), GET_REGNAME(&copied.inst, rs),
					GET_REGNAME(&copied.inst, rt));
			break;
		case I_FORMAT:
			pIDbuf->opcode = copied.inst.i_inst.opcode;
			pIDbuf->rsval = GET_REGVAL(copied.inst.i_inst.rs);
			pIDbuf->rtval = GET_REGVAL(copied.inst.i_inst.rt);
			pIDbuf->save_destination = copied.inst.i_inst.rt;
			pIDbuf->imm = copied.inst.i_inst.imm;
			printf("<Decode>: [I] %s %s %s\n",
					Opcode_table[index].mnemonic,
					GET_REGNAME(&copied.inst, rt), GET_REGNAME(&copied.inst, rs));
			break;
		case J_FORMAT:
			pIDbuf->opcode = copied.inst.j_inst.opcode;
			pIDbuf->addr = copied.inst.j_inst.addr;
			printf("<Decode>: [J] %s 0x%0x\n",
					Opcode_table[index].mnemonic,
					copied.inst.j_inst.addr);
			break;
	}
}

static uint32_t ALU(uint32_t in1, uint32_t in2, int index)
{
	uint32_t ans = 0;

	DEBUG_PRINTFN();

	// do the caculation
	if(Opcode_table[index].alu_fn){
		Opcode_table[index].alu_fn(&ans, in1, in2);
	}else{
		printf("Sorry, %s is not support yet!\n",
				Opcode_table[index].mnemonic);
	}
	
	return ans;
}

#define I_FORMAT_USE_ALU(i) (i == OP_ADDI || i == OP_ADDIU || \
							i == OP_SLTI || i == OP_SLTIU || \
							i == OP_ANDI || i == OP_ORI)
static void excute(void)
{
	struct decode_buffer copied;
	struct excute_buffer *pIEbuf = &pipeline_buf.IEbuf;

	DEBUG_PRINTFN();

	COPYDATA_FROM_PREV_STAGE_BUFFER(&copied, IDbuf, sizeof(copied));

	//TODO: must copy data from buffer first
	switch(copied.type){
		case R_FORMAT:
		{
			if(copied.optable_index == OP_JR){
				Mips_registers.pc += copied.rsval;
				pIEbuf->signal = false;
			}else{
				uint32_t out = ALU(copied.rsval, copied.rtval, copied.optable_index);

				pIEbuf->signal = true;
				pIEbuf->result = out;
				pIEbuf->save_destination = copied.save_destination;
			}
			break;
		}
		case I_FORMAT:
		{
			if(I_FORMAT_USE_ALU(copied.optable_index)){
				uint32_t out = ALU(copied.rsval, copied.imm, copied.optable_index);
				
				pIEbuf->signal = true;
				pIEbuf->result = out;
				pIEbuf->save_destination = copied.save_destination;
			}else if(copied.optable_index == OP_BEQ){	//branch
				if(copied.rsval == copied.rtval){
					Mips_registers.pc += (sizeof(union instruction) + (copied.imm << 2));
					pIEbuf->signal = false;
				}
			}else if(copied.optable_index == OP_BNE){	//branch
				if(copied.rsval != copied.rtval){
					Mips_registers.pc += (sizeof(union instruction) + (copied.imm << 2));
					pIEbuf->signal = false;
				}
			}else{
				printf("Load/store not implement yet!\n");
			}
		}
		case J_FORMAT:
		{
			if(copied.optable_index == OP_JUMP){
				Mips_registers.pc += (sizeof(union instruction) + copied.addr/4);
				pIEbuf->signal = false;
			}else{	//OP_JAL
				Mips_registers.pc += (sizeof(union instruction) + copied.addr/4);
				pIEbuf->result = copied.addr;
				pIEbuf->save_destination = REG_R31;
				pIEbuf->signal = true;
			}
		}
	}
}

static void memory(void){}
static void writeback(void){}

int main(int argc, char **argv)
{
	if(argc < 2){
		printf("Usage: %s [input file]\n", argv[0]);
		return -1;
	}

	parser(argv[1]);

	fetch();
	decode();
	excute();
	
	return 0;
}

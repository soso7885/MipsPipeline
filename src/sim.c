#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include "opcode_table.h"
#include "register_table.h"
#include "linkedlist.h"

#define POOLSIZE	512

#define PIPELINE_BYPASS			0
#define PIPELINE_GOTHROW_MEM	1
#define PIPELINE_GOTHROW_WB		2

#define MR_BORING_HW

/*
 * Get the function address to be the data
*/
#ifdef MR_BORING_HW
uint32_t dummy_data[] = {
	0x00,
	0x01,
	0x02,
	0x04,
	0x08,
	0x05,
	0x05,
	0x06,
	0x04,
// ---------------------------
	0x02,
	0x03,
	0x04,
	0x0A,
	0x07,
	0x07,
	0x08,
	0x04,
	0x06
};
#else
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
#endif
//XXX: use linkedlist to implement it, MR. Boring
uint8_t pool[POOLSIZE] = {0};

/* The memory dump linkedlist */
struct node *head = NULL;

int clock = 0;
volatile bool done = false;

//XXX: bad solution
uint32_t IE_pipe;
uint32_t MEM_pipe;

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
	bool stall;
	bool flush;
	union instruction inst;
};
 
struct decode_buffer {
	bool stop;
	bool stall;
	bool flush;
	int signal;	// control signal
	/* 
	 * In realworld, should use signal to decide which opration in ALU 
	 * not use optable_index
	*/
	int optable_index;
	uint8_t opcode;
	uint32_t s1val;
	uint32_t s2val;
	uint32_t rtval;
	uint8_t rd;	// destination register;
	uint8_t shamt;
	uint8_t funct;
	uint32_t j_addr; // jump address
};

struct excute_buffer {
	bool stop;
	bool stall;
	bool flush;
	int signal;
	int sz_access;	// the sizeof data which in memory
	bool direction;
	uint8_t rd;		// destination register;
	uint32_t result;
	uint32_t mem_addr;
};

struct memory_buffer {
	bool stop;
	bool stall;
	bool flush;
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
	int ret = 0;
	uint32_t *inst;
	char tmp[32] = {0};
	FILE *fin;
	
	fin = fopen(path, "r");
	if(!fin){
		fprintf(stderr, "%s: %s\n", path, strerror(errno));
		exit(0);
	}

	while(fgets(tmp, sizeof(tmp), fin)){
		// to ignore comment
		if(tmp[0] == '#' || tmp[0] == '\n' || tmp[0] == ' '){
			_ignore_comment(tmp, sizeof(tmp), &fin);
			memset(tmp, 0, sizeof(tmp));
			continue;
		}

		inst = (uint32_t *)&pool[(ret*sizeof(uint32_t))];
		sscanf(tmp, "%u", inst);
		memset(tmp, 0, sizeof(tmp));
		ret++;
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

static inline const char *_get_rs(int i)
{
	if(i > REG_END || i < 0)
		return "NULL";
	
	return Mips_registers.reg_table[i].name;
}

static inline const char *_get_rt(int i)
{
	if(i > REG_END || i < 0)
		return "NULL";
	
	return Mips_registers.reg_table[i].name;
}

static inline const char *_get_rd(int i)
{
	if(i > REG_END || i < 0)
		return "NULL";
	
	return Mips_registers.reg_table[i].name;
}

#define GET_REGNAME(inst, reg) _get_##reg((int)inst.r_inst.reg)

/*
 * -- Fetch --
 * 		fecthing the instruction from file to memory
*/
static void STAGE_fetch(void)
{
	uint32_t end = 0xffffffff;
	union instruction *inst;
	struct fetch_buffer *pIFbuf = &pipeline_buf.IFbuf;
	uint32_t *pc = &Mips_registers.pc;

	if(pIFbuf->flush){
		printf("<IF> flushing\n");
		return;
	}

	if(pIFbuf->stall){
		pIFbuf->stall = false;
		printf("<IF> stalling\n");
		return;
	}

	inst = (union instruction *)&pool[*pc];
	if(inst->raw == end){
		pIFbuf->stop = true;
		return;
	}

	*pc += sizeof(union instruction);
	printf("<IF> instruction(%u), ", inst->raw);

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

#define GET_FROM_REGISTER	0
#define GET_FROM_IE_PIPE	1
#define GET_FROM_MEM_PIPE	2
#define NEED_STALL			3

#define CHECK_STALL(res, i) \
	if(res == GET_FROM_IE_PIPE) printf("%s(%d) forwarding from IE, ", Mips_registers.reg_table[(int)i].name, IE_pipe);	\
	else if(res == GET_FROM_MEM_PIPE) printf("%s(%d) forwarding from MEM, ", Mips_registers.reg_table[(int)i].name, MEM_pipe);	\
	else if(res == NEED_STALL) break;

static int _get_regval(int i, uint32_t *targ)
{
	if(Mips_registers.reg_table[i].state == REG_NEED_UPDATE_FROM_IE){
		*targ = IE_pipe;
		return GET_FROM_IE_PIPE;
	}

	if(Mips_registers.reg_table[i].state == REG_NEED_UPDATE_STALL){
		*targ = 0;
		return NEED_STALL;
	}

	if(Mips_registers.reg_table[i].state == REG_NEED_UPDATE_FROM_MEM){
		*targ = MEM_pipe;
		return GET_FROM_MEM_PIPE;
	}

	*targ = Mips_registers.reg_table[i].data;
	return GET_FROM_REGISTER;
}
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

	if(copied.flush){
		pipeline_buf.IFbuf.flush = false;
		pIDbuf->flush = true;
		goto out;
	}

	printf("<ID> ");

	// start to decode
	index = _lookup_opcode(&copied.inst);
	if(index == OP_END){
		fprintf(stderr, "%s: cannot recognize instruction(%d)\n",
						__func__, copied.inst.raw);
		exit(0);
	}

	int res;
	int type = Opcode_table[index].type;

	pIDbuf->optable_index = index;
	pIDbuf->signal = PIPELINE_BYPASS;
	
	switch(type){
		case R_FORMAT:
			pIDbuf->opcode = copied.inst.r_inst.opcode;
			pIDbuf->rd = copied.inst.r_inst.rd;
			pIDbuf->shamt = copied.inst.r_inst.shamt;
			pIDbuf->funct = copied.inst.r_inst.funct;
			pIDbuf->signal |= PIPELINE_GOTHROW_WB;
			res = _get_regval((int)copied.inst.r_inst.rs, &pIDbuf->s1val);
			CHECK_STALL(res, copied.inst.r_inst.rs);
			res = _get_regval((int)copied.inst.r_inst.rt, &pIDbuf->s2val);
			CHECK_STALL(res, copied.inst.r_inst.rt);
			Mips_registers.reg_table[pIDbuf->rd].state = REG_NEED_UPDATE_FROM_IE;
			/* 
			 * For R type instruction, using forwarding data from
			 * IE, so mark the destation register state to NEED_UPDATE_FROM_IE 
			*/	
			printf("[R] %s %s %s %s",
						Opcode_table[index].mnemonic,
						GET_REGNAME(copied.inst, rd), 
						GET_REGNAME(copied.inst, rs),
						GET_REGNAME(copied.inst, rt));
			break;
		case I_FORMAT:
		{
			pIDbuf->opcode = copied.inst.i_inst.opcode;
			res = _get_regval((int)copied.inst.i_inst.rs, &pIDbuf->s1val);
			CHECK_STALL(res, copied.inst.i_inst.rs);
			pIDbuf->s2val = copied.inst.i_inst.imm;
			pIDbuf->rd = copied.inst.i_inst.rt;

			/* load/store instruction specific */
			if(BELOW_LOAD_STORE(index)){
				res = _get_regval((int)copied.inst.i_inst.rt, &pIDbuf->rtval);
				CHECK_STALL(res, copied.inst.i_inst.rt);
				pIDbuf->signal |= (PIPELINE_GOTHROW_MEM | PIPELINE_GOTHROW_WB);
				Mips_registers.reg_table[pIDbuf->rd].state = REG_NEED_UPDATE_FROM_IE;
				/* 
				 * For load/store instruction, need stalling 1 stage,
				 * somark the destation register state to NEED_UPDATE_STALL 
				*/
				printf("[I] %s %s, %u(%s)", 
								Opcode_table[index].mnemonic, 
								GET_REGNAME(copied.inst, rt),
								copied.inst.i_inst.imm,
								GET_REGNAME(copied.inst, rs));
			}else if(BELOW_BRANCH(index)){
				res = _get_regval((int)copied.inst.i_inst.rt, &pIDbuf->rtval);
				CHECK_STALL(res, copied.inst.i_inst.rt);
				pipeline_buf.IFbuf.flush = true;	// IF flushing
				printf("[I] %s %s %s, %u", 
								Opcode_table[index].mnemonic, 
								GET_REGNAME(copied.inst, rt), 
								GET_REGNAME(copied.inst, rs), 
								copied.inst.i_inst.imm);
			}else{
				pIDbuf->signal |= PIPELINE_GOTHROW_WB;
				/* 
				 * For I type instruction, using forwarding data from IE,
				 * so mark the destation register state to NEED_UPDATE_FROM_IE
				*/
				Mips_registers.reg_table[pIDbuf->rd].state = REG_NEED_UPDATE_FROM_IE;
				printf("[I] %s %s %s, %u", 
								Opcode_table[index].mnemonic, 
								GET_REGNAME(copied.inst, rt), 
								GET_REGNAME(copied.inst, rs), 
								copied.inst.i_inst.imm);
			}
			break;
		}
		case J_FORMAT:
		{
			pIDbuf->opcode = copied.inst.j_inst.opcode;
			pIDbuf->j_addr = copied.inst.j_inst.addr;
			if(index == OP_JAL){
				pIDbuf->signal |= PIPELINE_GOTHROW_WB;
				pIDbuf->rd = REG_R31;
				Mips_registers.reg_table[pIDbuf->rd].state = REG_NEED_UPDATE_FROM_IE;
			}
			printf("[J] %s 0x%0x",
						Opcode_table[index].mnemonic,
						copied.inst.j_inst.addr);
			break;
		}
	}

	if(res == NEED_STALL){
		printf("stalling");
		pIDbuf->stall = true;
		pipeline_buf.IFbuf.stall = true;
	}

	printf("\n");
out:
	/* Do the previous pipeline */
	STAGE_fetch();
}

static uint32_t ALU(uint32_t in1, uint32_t in2, int index)
{
	uint32_t ans = 0;

	if(Opcode_table[index].alu_fn){
		Opcode_table[index].alu_fn(&ans, in1, in2);
	}else{
		printf("\n\nSorry, %s is not support yet!\n\n",
					Opcode_table[index].mnemonic);
		exit(0);
	}
	
	return ans;
}

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

	do{
		if(copied.flush){
			pipeline_buf.IDbuf.flush = false;
			pIEbuf->flush = true;
			break;
		}

		printf("<IE> ");

		if(copied.stall){
			printf("stalling\n");
			pIEbuf->stall = true;
			pipeline_buf.IDbuf.stall = false;
			break;
		}

		if(BELOW_ALU(copied.optable_index)){	// need ALU
			uint32_t res = ALU(copied.s1val, copied.s2val, copied.optable_index);
	
			pIEbuf->rd = copied.rd;
			/* Load/Store method, need to update memory access address */
			if(BELOW_LOAD_STORE(copied.optable_index)){
				pIEbuf->mem_addr = res;
				pIEbuf->sz_access = LOAD_STORE_DATA_SIZE(copied.optable_index);
				pIEbuf->direction = GET_MEM_DIRECTION(copied.optable_index);
				/* For store inst, stored the rt val into memory */
				pIEbuf->result = copied.rtval;
				printf("ALU: address = 0x%0x\n", res);
			}else{
				pIEbuf->result = res;
				printf("ALU: res = %u(forward)\n", res);
				IE_pipe = pIEbuf->result;	// forwarding data
			}
		}else if(BELOW_JUMP(copied.optable_index)){	// Jump method
			uint32_t jump_targ = Mips_registers.pc;

			switch(copied.optable_index){
				case OP_JR:
					jump_targ = copied.s1val;
					break;
				case OP_JUMP:
					jump_targ += (sizeof(union instruction) + copied.j_addr/4);
					break;
				case OP_JAL:
					jump_targ += (sizeof(union instruction) + copied.j_addr/4);
					pIEbuf->result = copied.j_addr;
					break;
			}
			Mips_registers.pc = jump_targ;	// update PC
			printf("jump to 0x%0x\n", jump_targ);
		}else if(BELOW_BRANCH(copied.optable_index)){	// branch method
			if(copied.optable_index == OP_BEQ){	// beq
				if(copied.s1val == copied.rtval){
					Mips_registers.pc += (sizeof(union instruction)+(copied.s2val << 2));
					printf("BEQ holds, pc = %d\n", Mips_registers.pc);
				}else{
					printf("BEQ not holds\n");
				}
			}else{	// bne
				if(copied.s1val != copied.rtval){
					Mips_registers.pc += (sizeof(union instruction)+(copied.s2val << 2));
					printf("BNE holds, pc = %d\n", Mips_registers.pc);
				}else{
					printf("BNE not holds\n");
				}
			}
		}else{
			printf("Sorry, %s (%s) is not support now!\n",
					Opcode_table[copied.optable_index].mnemonic,
					Opcode_table[copied.optable_index].describe);
			// TODO: flushing
		}
		pIEbuf->signal = copied.signal;
	}while(0);
	
	/* Do the previous stage */
	STAGE_decode();
}

#ifdef MR_BORING_HW
#define DUMMY_OFFSET(i)	((i%4)*9 + (i/4))
#else
#define DUMMY_OFFSET(i)	(i%8)
#endif	

#define GET_WORD_DUMMY_DATA(addr) (dummy_data[DUMMY_OFFSET(addr)] & 0xffffffff)
#define GET_HALFWORD_DUMMY_DATA(addr) (dummy_data[DUMMY_OFFSET(addr)] & 0x0000ffff)
#define GET_BYTE_DUMMY_DATA(addr) (dummy_data[DUMMY_OFFSET(addr)] & 0x000000ff)

#define GET_DUMMY_DATA(addr, size)	\
	(size == SZ_WORD ? GET_WORD_DUMMY_DATA(addr) : \
	(size == SZ_HALFWORD ? GET_HALFWORD_DUMMY_DATA(addr) : GET_BYTE_DUMMY_DATA(addr)))
#define SET_WORD_DUMMY_DATA(addr, val) (dummy_data[DUMMY_OFFSET(addr)] =  val&0xffffffff)
#define SET_HALFWORD_DUMMY_DATA(addr, val) (dummy_data[DUMMY_OFFSET(addr)] = val&0x0000ffff)
#define SET_BYTE_DUMMY_DATA(addr, val) (dummy_data[DUMMY_OFFSET(addr)] = val&0x000000ff)

#define SET_DUMMY_DATA(addr, val, size)	\
	(size == SZ_WORD ? SET_WORD_DUMMY_DATA(addr, val) : \
	(size == SZ_HALFWORD ? SET_HALFWORD_DUMMY_DATA(addr, val) : SET_BYTE_DUMMY_DATA(addr, val)))

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

	do{
		if(copied.flush){
			pipeline_buf.IEbuf.flush = false;
			pMEMbuf->flush = true;
			break;
		}

		printf("<MEM> ");

		if(copied.stall){
			printf("stalling\n");
			pipeline_buf.IEbuf.stall = false;
			pMEMbuf->stall = true;
			break;
		}

		pMEMbuf->signal = copied.signal;
		pMEMbuf->rd = copied.rd;
		pMEMbuf->result = copied.result;

		if(!(copied.signal & PIPELINE_GOTHROW_MEM)){
			printf("do nothing\n");
			break;
		}

		if(copied.direction == MEM_LOAD){	// load
			pMEMbuf->result = GET_DUMMY_DATA(copied.mem_addr, copied.sz_access);
			MEM_pipe = pMEMbuf->result;	// forwarding data
			ll_add_node(&head, copied.mem_addr, pMEMbuf->result, MEM_LOAD);
			printf("Load 0x%0x from 0x%0x\n", pMEMbuf->result, copied.mem_addr);
			Mips_registers.reg_table[copied.rd].state = REG_NEED_UPDATE_FROM_MEM;
		}else{	// store
			SET_DUMMY_DATA(copied.mem_addr,copied.result, copied.sz_access);
			ll_add_node(&head, copied.mem_addr, copied.result, MEM_STORE);
			printf("Store 0x%0x to 0x%0x\n", copied.result, copied.mem_addr);
		}
	}while(0);

	/* do the previous pipeline stage */
	STAGE_excute();
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

	do{
		if(copied.flush){
			pipeline_buf.MEMbuf.flush = false;
			break;
		}

		printf("<WB> ");

		if(copied.stall){
			printf("stalling\n");
			pipeline_buf.MEMbuf.stall = false;
			break;
		}

		if(!(copied.signal & PIPELINE_GOTHROW_WB)){
			printf("do nothing\n");
			break;
		}
	
		Mips_registers.reg_table[copied.rd].data = copied.result;
		/*
		 * XXX: ugly fix, it's a special case 
		 * if register in WB == register in IE == register in ID, 
		 * don't reset the state of register, should use forwarding state
		*/
		if(copied.rd != pipeline_buf.IDbuf.rd && 
			pipeline_buf.IDbuf.rd != pipeline_buf.IFbuf.inst.r_inst.rs &&
			pipeline_buf.IFbuf.inst.r_inst.rs != copied.rd)
		{
			Mips_registers.reg_table[copied.rd].state = REG_INIT;
		}

		printf("write %s = 0x%0x\n", 
				Mips_registers.reg_table[copied.rd].name,
				Mips_registers.reg_table[copied.rd].data);
	}while(0);
	
	/* do the previous pipeline stage */
	STAGE_memory();
}

#define DUMP_MEMORY()	ll_show_all(head)
#define DUMP_REGISTER()	register_dump()
#define FREE_DUMP()		ll_free_all_node(head);

#if 1
#define CLOCK_CYCLE()	\
do{\
	if(done) break;	\
	printf("\n=== Clock cycle %d: ===\n", ++clock);\
	DUMP_MEMORY();	\
	DUMP_REGISTER();	\
	printf("\n");	\
}while(0);
#else
#define CLOCK_CYCLE()	\
do{\
	if(done) break;	\
	printf("\n");	\
}while(0);
#endif

int main(int argc, char **argv)
{
	if(argc < 2){
		printf("Usage: %s [input file]\n", argv[0]);
		return -1;
	}

#ifdef MR_BORING_HW
	printf("Running Mr. Boring's fucking homework!\n");
#endif

	memset(&pool, 0xff, sizeof(pool));

	parser(argv[1]);

	STAGE_fetch();
	CLOCK_CYCLE();
	STAGE_decode();
	CLOCK_CYCLE();
	STAGE_excute();
	CLOCK_CYCLE();
	STAGE_memory();
	CLOCK_CYCLE();
	STAGE_writeback();
	CLOCK_CYCLE();
	while(!done){
		STAGE_writeback();
		CLOCK_CYCLE();
	}

	FREE_DUMP();
	
	return 0;
}

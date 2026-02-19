#ifndef BLOCK_H_
#define BLOCK_H_

#include "resources.h"


#define BLOCK_INSTR_NOP 			0
#define BLOCK_INSTR_MADD			1
#define BLOCK_INSTR_ARSH 			2
#define BLOCK_INSTR_LSH 			3
#define BLOCK_INSTR_RSH 			4
#define BLOCK_INSTR_ABS				5
#define BLOCK_INSTR_MIN			    6
#define BLOCK_INSTR_MAX			    7
#define BLOCK_INSTR_CLAMP		    8
#define BLOCK_INSTR_MOV_ACC			9
#define BLOCK_INSTR_MOV_LACC		10
#define BLOCK_INSTR_MOV_UACC		11
#define BLOCK_INSTR_MACZ			12
#define BLOCK_INSTR_UMACZ			13
#define BLOCK_INSTR_MAC				14
#define BLOCK_INSTR_UMAC			15
#define BLOCK_INSTR_LUT_READ		16
#define BLOCK_INSTR_DELAY_READ 		17
#define BLOCK_INSTR_DELAY_WRITE 	18
#define BLOCK_INSTR_MEM_READ 		19
#define BLOCK_INSTR_MEM_WRITE		20

#define INSTR_MAX_ARGS 4

struct m_dsp_resource;

#define BLOCK_OPERAND_TYPE_C 0
#define BLOCK_OPERAND_TYPE_R 1

typedef struct {
	int type;
	int addr;
} m_block_operand;

#define ZERO_REGISTER_ADDR 		2
#define POS_ONE_REGISTER_ADDR  	3
#define NEG_ONE_REGISTER_ADDR  	4

m_block_operand operand_const_zero();
m_block_operand operand_const_one();
m_block_operand operand_const_minus_one();

#define M_ASM_ARG_CHANNEL 	0
#define M_ASM_ARG_EXPR 		1
#define M_ASM_ARG_RES  		2
#define M_ASM_ARG_INT  		3

struct m_derived_quantity;

typedef struct {
	int type;
	int addr;
	int val;
	struct m_derived_quantity *dq;
	m_dsp_resource *res;
} m_asm_arg;

typedef struct
{
	int format;
	int active;
	struct m_derived_quantity *dq;
} m_block_reg_val;

typedef struct {
	int instr;
	
	m_block_operand arg_a;
	m_block_operand arg_b;
	m_block_operand arg_c;
	
	int dest;
	
	m_block_reg_val reg_0;
	m_block_reg_val reg_1;
	
	int shift;
	int shift_set;
	int saturate_disable;
	
	m_dsp_resource *res;
} m_block;

typedef struct m_block_pll {
	m_block *data;
	struct m_block_pll *next;
} m_block_pll;

int m_block_pll_safe_append(m_block_pll **list_ptr, m_block *x);

#endif

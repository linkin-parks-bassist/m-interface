#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m_int.h"

IMPLEMENT_LINKED_PTR_LIST(m_block);

m_block_operand operand_const_zero()
{
	m_block_operand res;
	res.type = BLOCK_OPERAND_TYPE_R;
	res.addr = ZERO_REGISTER_ADDR;
	return res;
}

m_block_operand operand_const_one()
{
	m_block_operand res;
	res.type = BLOCK_OPERAND_TYPE_R;
	res.addr = POS_ONE_REGISTER_ADDR;
	return res;
}

m_block_operand operand_const_minus_one()
{
	m_block_operand res;
	res.type = BLOCK_OPERAND_TYPE_R;
	res.addr = NEG_ONE_REGISTER_ADDR;
	return res;
}

int m_init_block(m_block *block)
{
	if (!block)
		return ERR_NULL_PTR;
	
	block->instr = BLOCK_INSTR_NOP;
	
	block->arg_a.type = BLOCK_OPERAND_TYPE_C;
	block->arg_a.addr = 0;
	block->arg_b.type = BLOCK_OPERAND_TYPE_C;
	block->arg_b.addr = 0;
	block->arg_c.type = BLOCK_OPERAND_TYPE_C;
	block->arg_c.addr = 0;
	
	block->dest = 0;
	
	block->reg_0.format = 0;
	block->reg_0.active = 0;
	block->reg_0.expr = NULL;
	
	block->reg_1.format = 0;
	block->reg_1.active = 0;
	block->reg_1.expr = NULL;
	
	block->shift = 0;
	block->shift_set = 0;
	block->saturate_disable = 0;
	
	block->res = NULL;
	
	return NO_ERROR;
}

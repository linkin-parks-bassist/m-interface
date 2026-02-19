#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "m_int.h"

int m_fpga_block_opcode_format(int opcode)
{
	return (opcode == BLOCK_INSTR_LUT_READ
		 || opcode == BLOCK_INSTR_DELAY_READ
		 || opcode == BLOCK_INSTR_DELAY_WRITE
		 || opcode == BLOCK_INSTR_MEM_READ
		 || opcode == BLOCK_INSTR_MEM_WRITE)
		 ? INSTR_FORMAT_B : INSTR_FORMAT_A;
}

int m_block_instr_format(m_block *block)
{
	if (!block) return 0;
	
	return m_fpga_block_opcode_format(block->instr);
}

uint32_t m_encode_dsp_block_instr_type_a(m_block *block)
{
	if (!block) return 0;
	
	return place_bits( 5,  0, block->instr)
	 | place_bits( 9,  6, block->arg_a.addr) | ((!!(block->arg_a.type == M_ASM_ARG_EXPR)) << 10)
	 | place_bits(14, 11, block->arg_b.addr) | ((!!(block->arg_b.type == M_ASM_ARG_EXPR)) << 15)
	 | place_bits(19, 16, block->arg_c.addr) | ((!!(block->arg_c.type == M_ASM_ARG_EXPR)) << 20)
	 | place_bits(24, 21, block->dest)
	 | place_bits(29, 25, block->shift) | ((!!block->saturate_disable) << 30) | ((block->shift == 255) << 31);
}

uint32_t m_encode_dsp_block_instr_type_b(m_block *block, int res_handle)
{
	return place_bits(5, 0, block->instr) | (1 << 5)
			 | place_bits( 9,  6, block->arg_a.addr) | ((!!(block->arg_a.type == M_ASM_ARG_EXPR)) << 10)
			 | place_bits(14, 11, block->arg_b.addr) | ((!!(block->arg_b.type == M_ASM_ARG_EXPR)) << 15)
			 | place_bits(19, 16, block->dest)
			 | place_bits(31, 20, res_handle);
}

uint32_t m_block_instr_encode_resource_aware(m_block *block, const m_eff_resource_report *res)
{
	if (!block)
		return 0;
	
	int res_handle = 0;
	
	if (m_block_instr_format(block))
	{
		if (!block->res)
			return 0;
		
		switch (block->res->type)
		{
			case M_DSP_RESOURCE_MEM:
				res_handle = block->res->handle + (res ? res->memory : 0);
				break;
				
			case M_DSP_RESOURCE_DELAY:
				res_handle = block->res->handle + (res ? res->delays : 0);
				break;
		}
		
		return m_encode_dsp_block_instr_type_b(block, res_handle);
	}
	else
	{
		return m_encode_dsp_block_instr_type_a(block);
	}
}

int m_fpga_batch_append_block_instr(m_fpga_transfer_batch *batch, m_block *block, int pos, const m_eff_resource_report *res)
{
	if (!batch || !block)
		return ERR_NULL_PTR;
	
	uint32_t instr;
	
	m_fpga_batch_append(batch, COMMAND_WRITE_BLOCK_INSTR);
	m_fpga_batch_append_block_number(batch, pos);
	m_fpga_batch_append_32(batch, m_block_instr_encode_resource_aware(block, res));
	
	return NO_ERROR;
}

int m_fpga_batch_append_block_regs(m_fpga_transfer_batch *batch, m_block *block, int pos, m_parameter_pll *params)
{
	if (!batch || !block)
		return ERR_NULL_PTR;
	
	float v;
	int16_t s;
	
	if (block->reg_0.active && block->reg_0.expr)
	{
		v = m_expression_compute(block->reg_0.expr, params);
		s = float_to_q_nminus1(v, block->reg_0.format);
		
		m_fpga_batch_append(batch, COMMAND_WRITE_BLOCK_REG);
		m_fpga_batch_append_block_number(batch, pos);
		m_fpga_batch_append_16(batch, s);
	}
	
	if (block->reg_1.active && block->reg_1.expr)
	{
		v = m_expression_compute(block->reg_1.expr, params);
		s = float_to_q_nminus1(v, block->reg_1.format);
		
		m_fpga_batch_append(batch, COMMAND_WRITE_BLOCK_REG);
		m_fpga_batch_append_block_number(batch, pos);
		m_fpga_batch_append_16(batch, s);
	}
	
	return NO_ERROR;
}

int m_fpga_batch_append_block(m_fpga_transfer_batch *batch, m_block *block, const m_eff_resource_report *res, int pos, m_parameter_pll *params)
{
	if (!batch || !block)
		return ERR_NULL_PTR;
	
	m_fpga_batch_append_block_instr(batch, block, pos, res);
	m_fpga_batch_append_block_regs(batch, block, pos, params);
	
	return NO_ERROR;
}

int m_fpga_batch_append_blocks(m_fpga_transfer_batch *batch, m_block_pll *blocks, const m_eff_resource_report *res, int pos, m_parameter_pll *params)
{
	if (!batch || !blocks)
		return ERR_NULL_PTR;
	
	m_block_pll *current = blocks;
	
	int i = 0;
	
	int ret_val;
	while (current)
	{
		if (current->data && (ret_val = m_fpga_batch_append_block(batch, current->data, res, pos + i, params)) != NO_ERROR)
			return ret_val;
		
		current = current->next;
		i++;
	}
	
	return NO_ERROR;
}

int m_fpga_batch_append_resource(m_fpga_transfer_batch *batch, m_dsp_resource *res, const m_eff_resource_report *rpt, m_parameter_pll *params)
{
	if (!batch || !res || !rpt)
		return ERR_NULL_PTR;
	
	switch (res->type)
	{
		case M_DSP_RESOURCE_DELAY:
			m_fpga_batch_append(batch, COMMAND_ALLOC_DELAY);
			m_fpga_batch_append_24(batch, res->size);
			m_fpga_batch_append_24(batch, res->delay);
			break;
	}
	
	return NO_ERROR;
}

int m_fpga_batch_append_resources(m_fpga_transfer_batch *batch, m_dsp_resource_pll *list, const m_eff_resource_report *rpt, m_parameter_pll *params)
{
	if (!batch || !list || !rpt)
		return ERR_NULL_PTR;
	
	m_dsp_resource_pll *current = list;
	
	while (current)
	{
		m_fpga_batch_append_resource(batch, current->data, rpt, params);
		current = current->next;
	}
	
	return NO_ERROR;
}

int m_fpga_batch_append_eff_desc(m_fpga_transfer_batch *batch, m_effect_desc *eff, const m_eff_resource_report *res, int pos, m_parameter_pll *params)
{
	if (!batch || !eff || !res)
		return ERR_NULL_PTR;
	
	m_fpga_batch_append_resources(batch, eff->resources, res, params);
	m_fpga_batch_append_blocks(batch, eff->blocks, res, pos, params);
	
	return NO_ERROR;
}

int m_fpga_batch_append_transformer(m_fpga_transfer_batch *batch, m_transformer *trans, m_eff_resource_report *res, int *pos)
{
	if (!batch || !trans || !res || !pos)
		return ERR_NULL_PTR;
	
	if (!trans->eff)
		return ERR_BAD_ARGS;
	
	m_parameter_pll *params = trans->parameters;
	
	trans->block_position = *pos;
	
	m_fpga_batch_append_eff_desc(batch, trans->eff, res, *pos, params);
	
	res->memory += trans->eff->res_rpt.memory;
	res->delays += trans->eff->res_rpt.delays;
	
	*pos += trans->eff->res_rpt.blocks;
	
	return NO_ERROR;
}

int m_fpga_batch_append_transformers(m_fpga_transfer_batch *batch, m_transformer_pll *list, m_eff_resource_report *res, int *pos)
{
	if (!batch || !list || !res || !pos)
		return ERR_NULL_PTR;
	
	m_transformer_pll *current = list;
	
	while (current)
	{
		m_fpga_batch_append_transformer(batch, current->data, res, pos);
		current = current->next;
	}
	
	return NO_ERROR;
}

char *m_block_opcode_to_string(uint32_t opcode)
{
	switch (opcode)
	{
		case BLOCK_INSTR_NOP: 			return (char*)"BLOCK_INSTR_NOP";
		case BLOCK_INSTR_LSH: 			return (char*)"BLOCK_INSTR_LSH";
		case BLOCK_INSTR_RSH: 			return (char*)"BLOCK_INSTR_RSH";
		case BLOCK_INSTR_ARSH: 			return (char*)"BLOCK_INSTR_ARSH";
		case BLOCK_INSTR_MADD: 			return (char*)"BLOCK_INSTR_MADD";
		case BLOCK_INSTR_ABS: 			return (char*)"BLOCK_INSTR_ABS";
		case BLOCK_INSTR_LUT_READ:		return (char*)"BLOCK_INSTR_LUT_READ";
		case BLOCK_INSTR_DELAY_READ: 	return (char*)"BLOCK_INSTR_DELAY_READ";
		case BLOCK_INSTR_DELAY_WRITE: 	return (char*)"BLOCK_INSTR_DELAY_WRITE";
		case BLOCK_INSTR_MEM_WRITE:		return (char*)"BLOCK_INSTR_MEM_WRITE";
		case BLOCK_INSTR_MEM_READ:		return (char*)"BLOCK_INSTR_MEM_READ";
		case BLOCK_INSTR_MIN: 			return (char*)"BLOCK_INSTR_MIN";
		case BLOCK_INSTR_MAX: 			return (char*)"BLOCK_INSTR_MAX";
		case BLOCK_INSTR_CLAMP: 		return (char*)"BLOCK_INSTR_CLAMP";
		case BLOCK_INSTR_MACZ: 			return (char*)"BLOCK_INSTR_MACZ";
		case BLOCK_INSTR_MAC: 			return (char*)"BLOCK_INSTR_MAC";
		case BLOCK_INSTR_MOV_ACC: 		return (char*)"BLOCK_INSTR_MOV_ACC";
		case BLOCK_INSTR_MOV_UACC: 		return (char*)"BLOCK_INSTR_MOV_UACC";
		case BLOCK_INSTR_MOV_LACC: 		return (char*)"BLOCK_INSTR_MOV_LACC";
	}
	
	return NULL;
}

void print_instruction_format_a(uint32_t instr)
{
	int opcode = range_bits(instr, 5, 0);
	
	int src_a 	 = range_bits(instr, 4, 6);
	int src_a_reg = !!(instr & (1 << 10));
	
	int src_b 	 = range_bits(instr, 4, 11);
	int src_b_reg = !!(instr & (1 << 15));
	
	int src_c 	 = range_bits(instr, 4, 16);
	int src_c_reg = !!(instr & (1 << 20));
	
	int dest = range_bits(instr, 4, 21);
	int shift = range_bits(instr, 5, 25);
	int sat = !!(instr & (1 << 30));
	int no_shift = !!(instr & (1 << 31));
	
	printf("%s(%d, %d, %d, %d, %d, %d, %d, %d, %d)",
		m_block_opcode_to_string(opcode),
		src_a, src_a_reg, src_b, src_b_reg, src_c, src_c_reg, dest, shift, sat);
}

void print_instruction_format_b(uint32_t instr)
{
	int opcode = range_bits(instr, 5, 0);
	
	int src_a 	 = range_bits(instr, 4, 6);
	int src_a_reg = !!(instr & (1 << 10));
	
	int src_b 	 = range_bits(instr, 4, 11);
	int src_b_reg = !!(instr & (1 << 15));
	
	int no_shift = 0;
	
	int src_c = 0;
	int src_c_reg = 0;
	
	int dest = range_bits(instr, 4, 16);
	
	int res_addr = range_bits(instr, 8, 20);
	
	printf("%s(%d, %d, %d, %d, %d, %d)",
		m_block_opcode_to_string(opcode),
		src_a, src_a_reg, src_b, src_b_reg, dest, res_addr);
}

void print_instruction(uint32_t instr)
{
	int format = !!(instr & (1 << 5));
	
	if (format)
		print_instruction_format_b(instr);
	else
		print_instruction_format_a(instr);
}

int m_fpga_batch_print(m_fpga_transfer_batch seq)
{
	int n = seq.len;
	
	printf("Reading out FPGA transfer batch %p (length %d)\n", seq.buf, n);
	
	if (!seq.buf)
	{
		printf("Buffer is NULL!\n");
	}
	
	if (n < 1)
	{
		printf("Batch has no bytes!\n");
	}
	
	int i = 0;
	
	int state = 0;
	int ret_state;
	int skip = 0;
	
	int shift;
	
	uint8_t byte;
	
	int ctr = 0;
	int ctr_2 = 0;
	
	uint8_t reg_no = 0;
	m_fpga_block_addr_t block = 0;
	m_fpga_sample_t value = 0;
	uint32_t instruction = 0;
	
	while (i < n)
	{
		byte = seq.buf[i];
		
		printf("\tByte %s%d: 0x%02X. ", (n > 9 && i < 10) ? " " : "", i, byte);
		
		switch (state)
		{
			case 0: // expecting a command
				switch (byte)
				{
					case COMMAND_WRITE_BLOCK_INSTR:
						printf("Command WRITE_BLOCK_INSTR");
						state = 1;
						ret_state = 2;
						ctr = 0;
						instruction = 0;
						break;

					case COMMAND_WRITE_BLOCK_REG_0:
						printf("Command WRITE_BLOCK_REG");
						state = 1;
						ret_state = 4;
						value = 0;
						ctr = 0;
						break;

					case COMMAND_UPDATE_BLOCK_REG_0:
						printf("Command UPDATE_BLOCK_REG");
						state = 1;
						ret_state = 4;
						value = 0;
						ctr = 0;
						break;

					case COMMAND_WRITE_BLOCK_REG_1:
						printf("Command WRITE_BLOCK_REG");
						state = 1;
						ret_state = 4;
						value = 0;
						ctr = 0;
						break;

					case COMMAND_UPDATE_BLOCK_REG_1:
						printf("Command UPDATE_BLOCK_REG");
						state = 1;
						ret_state = 4;
						value = 0;
						ctr = 0;
						break;

					case COMMAND_ALLOC_DELAY:
						printf("Command ALLOC_DELAY");
						state = 5;
						value = 0;
						ctr = 0;
						ctr_2 = 0;
						break;

					case COMMAND_SWAP_PIPELINES:
						printf("Command SWAP_PIPELINES");
						break;

					case COMMAND_RESET_PIPELINE:
						printf("Command RESET_PIPELINE");
						break;

					case COMMAND_SET_INPUT_GAIN:
						printf("Command SET_INPUT_GAIN");
						break;

					case COMMAND_SET_OUTPUT_GAIN:
						printf("Command SET_OUTPUT_GAIN");
						break;

				}
				break;
			
			case 1: // expecting block number
				block = (block << 8) | byte;
				
				if (ctr == M_FPGA_BLOCK_ADDR_BYTES - 1)
				{
					printf("Block number %d", byte);
					state = ret_state;
					ctr = 0;
				}
				else
				{
					ctr++;
				}
				
				break;
			
			case 2: // expecting instruction
				if (ctr == 3)
				{
					state = 0;
					
					instruction = (instruction << 8) | byte;
					
					printf("Word: %s; ", binary_print_32(instruction));
					print_instruction(instruction);
					
					shift = range_bits(instruction, 5, 25);
				}
				else
				{
					instruction = (instruction << 8) | byte;
					ctr++;
				}
				break;
			
			case 3: // expecting register number then register value
				printf("Register %d", byte);
				state = 4;
				break;
			
			case 4: // expecting a value
				if (ctr == M_FPGA_DATA_BYTES - 1)
				{
					state = 0;
					
					value = (value << 8) | byte;
					printf("Value: %s = %d = %f (in q%d.%d)", binary_print_16(value), value, (float)value / (powf(2.0, 15 - shift)), 1 + shift, 15 - shift);
				}
				else
				{
					value = (value << 8) | byte;
					ctr++;
				}
				break;
			
			case 5: // expecting two 24-bit values. lol!
				if (ctr == 0)
				{
					value = byte;
					ctr++;
				}
				else if (ctr == 2)
				{
					value = (value << 8) | byte;
					printf("Value: %s = %d = %llu", binary_print_24(value), value, (float)value / (powf(2.0, 15)));
					
					ctr = 0;
					
					if (ctr_2 == 1)
						state = 0;
					else
						ctr_2 = 1;
				}
				else
				{
					value = (value << 8) | byte;
					ctr++;
				}
				break;
			
			default:
				printf("Unknown :(\n");
				return 1;
		}
		
		printf("\n");
		
		i++;
	}
	
	return 0;
}

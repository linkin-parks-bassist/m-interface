#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m_int.h"

#define CH_DELIMITER 'c'
#define DQ_DELIMITER "["
#define RS_DELIMITER "$"
#define DQ_CODELIMITER "]"

int m_parse_asm_arg(m_eff_parsing_state *ps, m_asm_arg *arg)
{
	if (!ps)
		return ERR_NULL_PTR;
	
	m_token_ll *current = ps->current_token;
	m_token_ll *tok;
	
	m_dsp_resource_pll *current_res;
	int resource_found;
	
	int ret_val = NO_ERROR;
	
	int n;
	int valid;
	
	if (!current || !current->data)
	{
		ret_val = ERR_BAD_ARGS;
		goto asm_parse_arg_fin;
	}
	
	if (current->data[0] == CH_DELIMITER)
	{
		if (arg) arg->type = M_ASM_ARG_CHANNEL;
		
		valid = 1;
		n = 0;
		for (int i = 1; current->data[i] != 0; i++)
		{
			if (i > 2)
				valid = 0;
			
			if ('0' <= current->data[i] && current->data[i] <= '9')
			{
				n = 10 * n + current->data[i] - '0';
			}
			else if ('a' <= current->data[i] && current->data[i] <= 'f')
			{
				n = 16 * n + current->data[i] - 'a' + 10;
			}
			else if ('A' <= current->data[i] && current->data[i] <= 'F')
			{
				n = 16 * n + current->data[i] - 'A' + 10;
			}
			else
			{
				valid = 0;
			}
			
			if (!valid)
			{
				m_parser_error_at(ps, current, "Invalid argument \"%s\"");
				ret_val = ERR_BAD_ARGS;
				goto asm_parse_arg_fin;
			}
		}
		
		if (arg) arg->addr = n;
	}
	else if (strcmp(current->data, DQ_DELIMITER) == 0)
	{
		if (arg) arg->type = M_ASM_ARG_EXPR;
		
		tok = current;
		
		do 
		{
			tok = tok->next;
			
			if (!tok || strcmp(tok->data, "\n") == 0)
			{
				m_parser_error_at(ps, current, "Missing \"%s\"", DQ_CODELIMITER);
				ret_val = ERR_BAD_ARGS;
				goto asm_parse_arg_fin;
			}
			
		} while (strcmp(tok->data, DQ_CODELIMITER) != 0);
		
		if (arg)
		{
			arg->expr = m_parse_expression(ps, current->next, tok);
			if (!arg->expr)
			{
				ret_val = ERR_BAD_ARGS;
				goto asm_parse_arg_fin;
			}
		}
		
		if (tok)
			current = tok;
		else
			current = NULL;
	}
	else if (strcmp(current->data, RS_DELIMITER) == 0)
	{
		if (arg) arg->type = M_ASM_ARG_RES;
		
		current = current->next;
		
		if (!current || !current->data || strcmp(current->data, "\n") == 0)
		{
			m_parser_error_at(ps, current, "Missing resource identifier");
			ret_val = ERR_BAD_ARGS;
			goto asm_parse_arg_fin;
		}
		
		current_res = ps->resources;
		
		resource_found = 0;
		while (current_res && !resource_found)
		{
			if (current_res->data && current_res->data->name)
			{
				if (strcmp(current->data, current_res->data->name) == 0)
				{
					resource_found = 1;
					if (arg) arg->res = current_res->data;
				}
			}
			
			current_res = current_res->next;
		}
		
		if (resource_found && arg)
		{
			printf("Resource found; name \"%s\", type %d, size %d, handle %d\n",
				arg->res->name, arg->res->type, arg->res->size, arg->res->handle);
		}
		else
		{
			m_parser_error_at(ps, current, "Resource \"%s\" not found", current->data);
			ret_val = ERR_BAD_ARGS;
			goto asm_parse_arg_fin;
		}
	}
	else if (token_is_int(current->data))
	{
		if (arg)
		{
			arg->type = M_ASM_ARG_INT;
			arg->val = strtol(current->data, NULL, 10);
		}
	}
	else 
	{
		printf("Syntax error: \"%s\"\n", current->data);
		ret_val = ERR_BAD_ARGS;
		goto asm_parse_arg_fin;
	}
	
asm_parse_arg_fin:

	if (current)
		current = current->next;

	ps->current_token = current;
	
	return ret_val;
}

int m_parse_asm_line(m_eff_parsing_state *ps)
{
	if (!ps)
		return ERR_NULL_PTR;
	
	m_token_ll *current = ps->current_token;
	
	if (!current)
		return ERR_BAD_ARGS;
	
	if (!current->data)
		return ERR_BAD_ARGS;
	
	m_block *block = m_alloc(sizeof(m_block));
	
	if (!block)
		return ERR_ALLOC_FAIL;
	
	memset(block, 0, sizeof(m_block));
	
	int pos = 0;
	int n_args_expected = 3;
	
	int dest_pos  =  0;
	int arg_a_pos =  1;
	int arg_b_pos =  2;
	int arg_c_pos = -1;
	int res_pos   = -1;
	int res_type  =  0;
	int shift_pos = -1;
	
	
	int reg_0_taken = 0;
	int reg_1_taken = 0;
	
	int shift_mode = 0;
	
	if (current->data[0] == 's' && current->data[1] == 'h' && current->data[2] == '_')
	{
		shift_mode = 1;
		current->data = &current->data[3];
	}
	
	int ret_val = NO_ERROR;
	
	const char *instr_char = current->data;
	
	m_asm_arg args[INSTR_MAX_ARGS];
	
	int args_cont = 1;
	int line_fin = 0;
	int n_args_read = 0;
	int arg_ret_val;
	
	if (strcmp(current->data, "nop") == 0)
	{
		block->instr = BLOCK_INSTR_NOP;
		
		dest_pos  = -1;
		arg_a_pos = -1;
		arg_b_pos = -1;
		
		n_args_expected = 0;
	}
	else if (strcmp(current->data, "mov") == 0)
	{
		block->instr = BLOCK_INSTR_MADD;
		block->arg_b = operand_const_one();
		block->arg_c = operand_const_zero();
		block->shift = 1;
		
		arg_b_pos = -1;
		
		n_args_expected = 2;
	}
	else if (strcmp(current->data, "add") == 0)
	{
		block->instr = BLOCK_INSTR_MADD;
		block->arg_b = operand_const_one();
		block->shift = 1;
		
		arg_b_pos = -1;
		arg_c_pos =  2;
	}
	else if (strcmp(current->data, "sub") == 0)
	{
		block->instr = BLOCK_INSTR_MADD;
		block->arg_b = operand_const_minus_one();
		
		arg_b_pos = -1;
		arg_c_pos =  2;
	}
	else if (strcmp(current->data, "mul") == 0)
	{
		block->instr = BLOCK_INSTR_MADD;
		
		block->arg_c = operand_const_zero();
		block->shift = 0;
		
		arg_b_pos =  2;
	}
	else if (strcmp(current->data, "madd") == 0)
	{
		block->instr = BLOCK_INSTR_MADD;
		
		arg_c_pos = 3;
		
		n_args_expected = 4;
	}
	else if (strcmp(current->data, "arsh") == 0)
	{
		block->instr = BLOCK_INSTR_MADD;
		block->arg_b = operand_const_one();
		block->shift = 1;
		shift_mode = 0;
		
		arg_b_pos = -1;
		shift_pos =  2;
	}
	else if (strcmp(current->data, "lsh") == 0)
	{
		block->instr = BLOCK_INSTR_LSH;
		shift_mode = 0;
		
		arg_b_pos = -1;
		shift_pos =  2;
	}
	else if (strcmp(current->data, "rsh") == 0)
	{
		block->instr = BLOCK_INSTR_RSH;
		shift_mode = 0;
		
		arg_b_pos = -1;
		shift_pos =  2;
	}
	else if (strcmp(current->data, "abs") == 0)
	{
		block->instr = BLOCK_INSTR_RSH;
		
		arg_b_pos = -1;
		n_args_expected = 2;
	}
	else if (strcmp(current->data, "min") == 0)
	{
		block->instr = BLOCK_INSTR_MIN;
	}
	else if (strcmp(current->data, "max") == 0)
	{
		block->instr = BLOCK_INSTR_MAX;
	}
	else if (strcmp(current->data, "clamp") == 0)
	{
		block->instr = BLOCK_INSTR_CLAMP;
		
		arg_c_pos = 3;
		
		n_args_expected = 4;
	}
	else if (strcmp(current->data, "mov_acc") == 0)
	{
		block->instr = BLOCK_INSTR_MOV_ACC;
		
		arg_a_pos = -1;
		arg_b_pos = -1;
		
		n_args_expected = 1;
	}
	else if (strcmp(current->data, "mov_lacc") == 0)
	{
		block->instr = BLOCK_INSTR_MOV_LACC;
		
		arg_a_pos = -1;
		arg_b_pos = -1;
		
		n_args_expected = 1;
	}
	else if (strcmp(current->data, "mov_uacc") == 0)
	{
		block->instr = BLOCK_INSTR_MOV_UACC;
		
		arg_a_pos = -1;
		arg_b_pos = -1;
		
		n_args_expected = 1;
	}
	else if (strcmp(current->data, "macz") == 0)
	{
		block->instr = BLOCK_INSTR_MACZ;
		n_args_expected = 2;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "macz_noshift") == 0)
	{
		block->instr = BLOCK_INSTR_MACZ;
		
		block->shift = 15;
		n_args_expected = 2;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "umacz") == 0)
	{
		block->instr = BLOCK_INSTR_UMACZ;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
		
		n_args_expected = 2;
	}
	else if (strcmp(current->data, "umacz_noshift") == 0)
	{
		block->instr = BLOCK_INSTR_UMACZ;
		
		block->shift = 15;
		n_args_expected = 2;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "macz_unsat") == 0)
	{
		block->instr = BLOCK_INSTR_MACZ;
		block->saturate_disable = 1;
		n_args_expected = 2;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "macz_unsat_noshift") == 0)
	{
		block->instr = BLOCK_INSTR_MACZ;
		block->saturate_disable = 1;
		n_args_expected = 2;
		
		block->shift = 15;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "umacz_unsat") == 0)
	{
		block->instr = BLOCK_INSTR_UMACZ;
		block->saturate_disable = 1;
		n_args_expected = 2;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "umacz_unsat_noshift") == 0)
	{
		block->instr = BLOCK_INSTR_UMACZ;
		block->saturate_disable = 1;
		n_args_expected = 2;
		
		block->shift = 15;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "mac") == 0)
	{
		block->instr = BLOCK_INSTR_MAC;
		n_args_expected = 2;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "mac_noshift") == 0)
	{
		block->instr = BLOCK_INSTR_MAC;
		
		n_args_expected = 2;
		block->shift = 15;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "umac") == 0)
	{
		block->instr = BLOCK_INSTR_UMAC;
		n_args_expected = 2;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "umac_noshift") == 0)
	{
		block->instr = BLOCK_INSTR_UMAC;
		n_args_expected = 2;
		
		block->shift = 15;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "mac_unsat") == 0)
	{
		block->instr = BLOCK_INSTR_MAC;
		n_args_expected = 2;
		block->saturate_disable = 1;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "mac_unsat_noshift") == 0)
	{
		block->instr = BLOCK_INSTR_MAC;
		n_args_expected = 2;
		block->saturate_disable = 1;
		
		block->shift = 15;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "umac_unsat") == 0)
	{
		block->instr = BLOCK_INSTR_UMAC;
		block->saturate_disable = 1;
		n_args_expected = 2;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "umac_unsat_noshift") == 0)
	{
		block->instr = BLOCK_INSTR_UMAC;
		block->saturate_disable = 1;
		n_args_expected = 2;
		
		block->shift = 15;
		
		arg_a_pos = 0;
		arg_b_pos = 1;
		dest_pos = -1;
	}
	else if (strcmp(current->data, "delay_read") == 0)
	{
		block->instr = BLOCK_INSTR_DELAY_READ;
		
		arg_a_pos = -1;
		arg_b_pos = -1;
		res_pos = 1;
		
		n_args_expected = 2;
	}
	else if (strcmp(current->data, "delay_write") == 0)
	{
		block->instr = BLOCK_INSTR_DELAY_WRITE;
		
		arg_a_pos = 1;
		arg_b_pos = -1;
		dest_pos = -1;
		res_pos = 0;
		
		block->arg_b.type = BLOCK_OPERAND_TYPE_R;
		block->arg_b.addr = ZERO_REGISTER_ADDR;
		
		n_args_expected = 2;
	}
	else if (strcmp(current->data, "delay_mwrite") == 0)
	{
		block->instr = BLOCK_INSTR_DELAY_WRITE;
		
		res_pos = 0;
		dest_pos = -1;
		
		n_args_expected = 3;
	}
	else if (strcmp(current->data, "mem_read") == 0)
	{
		block->instr = BLOCK_INSTR_MEM_READ;
		
		arg_a_pos = -1;
		arg_b_pos = -1;
		res_pos = 1;
		
		n_args_expected = 2;
	}
	else if (strcmp(current->data, "mem_write") == 0)
	{
		block->instr = BLOCK_INSTR_MEM_WRITE;
		
		dest_pos = -1;
		arg_b_pos = -1;
		res_pos = 0;
		
		n_args_expected = 2;
	}
	else
	{
		printf("Error: unknown instruction \"%s\"\n", current->data);
		ret_val = ERR_BAD_ARGS;
		goto asm_line_parse_fin;
	}
	
	if (shift_mode)
	{
		n_args_expected++;
		shift_pos = dest_pos;
		
		if (arg_a_pos > shift_pos)
			shift_pos = arg_a_pos;
		if (arg_b_pos > shift_pos)
			shift_pos = arg_b_pos;
		if (arg_c_pos > shift_pos)
			shift_pos = arg_c_pos;
		if (arg_c_pos > res_pos)
			shift_pos = arg_c_pos;
		
		shift_pos++;
	}
	
	current = current->next;
	
	ps->current_token = current;
	
	for (int i = 0; ; i++)
	{
		if (!current || !current->data || current->data[0] == '\n')
			break;
		
		if ((arg_ret_val = m_parse_asm_arg(ps, (i < INSTR_MAX_ARGS) ? &args[i] : NULL)) != NO_ERROR)
		{
			current = ps->current_token;
			ret_val = arg_ret_val;
			goto asm_line_parse_fin;
		}
		
		n_args_read++;
		
		current = ps->current_token;
	}
	
	if (n_args_read < n_args_expected)
	{
		printf("Error: too few arguments for instruction \"%s\" (expected %d, given %d)\n", instr_char, n_args_expected, n_args_read);
		ret_val = ERR_BAD_ARGS;
		goto asm_line_parse_fin;
	}
	else if (n_args_read > n_args_expected)
	{
		printf("Error: too many arguments for instruction \"%s\" (expected %d, given %d)\n", instr_char, n_args_expected, n_args_read);
		ret_val = ERR_BAD_ARGS;
		goto asm_line_parse_fin;
	}
	
	if (dest_pos != -1)
	{
		if (args[dest_pos].type != M_ASM_ARG_CHANNEL)
		{
			printf("Error: destination must be a channel\n");
			ret_val = ERR_BAD_ARGS;
			goto asm_line_parse_fin;
		}
		else
		{
			block->dest = args[dest_pos].addr;
		}
	}
	
	if (shift_pos != -1)
	{
		if (args[shift_pos].type != M_ASM_ARG_INT || args[shift_pos].val < 0 || args[shift_pos].val > 15)
		{
			printf("Error: shift value must be an integer between 0 and 15\n");
			ret_val = ERR_BAD_ARGS;
			goto asm_line_parse_fin;
		}
		else
		{
			block->shift = args[shift_pos].val;
		}
	}
	
	if (arg_a_pos != -1)
	{
		switch (args[arg_a_pos].type)
		{
			case M_ASM_ARG_CHANNEL:
				block->arg_a.type = BLOCK_OPERAND_TYPE_C;
				block->arg_a.addr = args[arg_a_pos].addr;
				break;
			
			case M_ASM_ARG_EXPR:
				block->arg_a.type = BLOCK_OPERAND_TYPE_R;
				block->arg_a.addr = 0;
				reg_0_taken = 1;
				
				block->reg_0.expr = args[arg_a_pos].expr;
				block->reg_0.active = 1;
				break;
			
			default:
				printf("Error: wrong type given for arg a\n");
				break;
		}
	}
	
	if (arg_b_pos != -1)
	{
		switch (args[arg_b_pos].type)
		{
			case M_ASM_ARG_CHANNEL:
				block->arg_b.type = BLOCK_OPERAND_TYPE_C;
				block->arg_b.addr = args[arg_b_pos].addr;
				break;
			
			case M_ASM_ARG_EXPR:
				block->arg_b.type = BLOCK_OPERAND_TYPE_R;
				
				if (!reg_0_taken)
				{
					block->arg_b.addr = 0;
					reg_0_taken = 1;
					
					block->reg_0.expr = args[arg_b_pos].expr;
					block->reg_0.active = 1;
				}
				else
				{
					block->arg_b.addr = 1;
					reg_1_taken = 1;
					
					block->reg_1.expr = args[arg_b_pos].expr;
					block->reg_1.active = 1;
				}
				break;
			
			default:
				printf("Error: wrong type given for arg b\n");
				ret_val = ERR_BAD_ARGS;
				goto asm_line_parse_fin;
				break;
		}
	}
	
	if (arg_c_pos != -1)
	{
		switch (args[arg_c_pos].type)
		{
			case M_ASM_ARG_CHANNEL:
				block->arg_c.type = BLOCK_OPERAND_TYPE_C;
				block->arg_c.addr = args[arg_c_pos].addr;
				break;
			
			case M_ASM_ARG_EXPR:
				block->arg_c.type = BLOCK_OPERAND_TYPE_R;
				if (!reg_0_taken)
				{
					block->arg_c.addr = 0;
					reg_0_taken = 1;
					
					block->reg_0.expr = args[arg_c_pos].expr;
					block->reg_0.active = 1;
				}
				else if (!reg_1_taken)
				{
					block->arg_c.addr = 1;
					reg_1_taken = 1;
					
					block->reg_1.expr = args[arg_c_pos].expr;
					block->reg_1.active = 1;
				}
				else
				{
					printf("Error: too many constants\n");
					ret_val = ERR_BAD_ARGS;
					goto asm_line_parse_fin;
				}
				break;
			
			default:
				printf("Error: wrong type given for arg c\n");
				ret_val = ERR_BAD_ARGS;
				goto asm_line_parse_fin;
				break;
		}
	}
	
	if (res_pos != -1)
	{
		if (args[res_pos].type != M_ASM_ARG_RES)
		{
			printf("Error: resource must be a resource\n");
			ret_val = ERR_BAD_ARGS;
			goto asm_line_parse_fin;
		}
		else
		{
			block->res = args[res_pos].res;
		}
	}
	
	block->shift_set = shift_mode;
	
	m_block_pll_safe_append(&ps->blocks, block);
	
asm_line_parse_fin:
	if (current)
		current = current->next;
	
	ps->current_token = current;
	
	return ret_val;
}

int m_parse_asm(m_eff_parsing_state *ps)
{
	if (!ps)
		return ERR_NULL_PTR;
	
	m_token_ll *current = ps->current_token;
	
	int line_start;
	int comment;
	
	m_token_ll_skip_ws(&current);
	while (current)
	{
		line_start = 0;
		comment = 0;
		
		// Advance to the next non-whitespace token
		ps->current_token = current;
		m_parse_asm_line(ps);
		
		current = ps->current_token;
		m_token_ll_skip_ws(&current);
	}
	
	return NO_ERROR;
}

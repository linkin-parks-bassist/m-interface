#include <stdio.h>

#include "kest_int.h"

//#ifndef PRINTLINES_ALLOWED
#define PRINTLINES_ALLOWED 0
//#endif

static const char *FNAME = "kest_reg_format.c";

int shift_policy_determine_shift(int shift_policy, int format_a, int format_b, int format_c, int shift_given)
{
	switch (shift_policy)
	{
		case SHIFT_POLICY_1: 	return 1;
		case SHIFT_POLICY_FA: 	return format_a;
		case SHIFT_POLICY_FB:	return format_b;
		case SHIFT_POLICY_FC: 	return format_c;
		case SHIFT_POLICY_SFAB: return format_a + format_b;
		case SHIFT_POLICY_MFAB: return format_b > format_a ? format_b : format_a;
		case SHIFT_POLICY_SET: 	return shift_given;
	}
	
	return 0;
}

int kest_compute_register_formats(kest_block_pll *blocks, kest_scope *scope)
{
	KEST_PRINTF("kest_compute_register_formats\n");
	if (!blocks)
	{
		KEST_PRINTF("kest_compute_register_formats: no blocks!\n");
		return NO_ERROR;
	}
	
	kest_block_pll *current = blocks;
	
	kest_interval range;
	float min, max;
	float abs_min, abs_max;
	float max_abs;
	int format_0, format_1;
	int format_a, format_b, format_c;
	int shift_set = 0;
	float p2;
	
	kest_string string;
	kest_string_init(&string);
	char *str = NULL;
	
	int i = 0;
	while (current)
	{
		if (current->data && !current->data->shift_set)
		{
			format_0 = 0;
			format_1 = 0;
			
			format_a = 0;
			format_b = 0;
			format_c = 0;
			
			kest_string_appendf(&string, "kest_compute_register_formats(current->data = %p), shift_set = %d, reg_0_active: %d, reg_1_active: %d\n",
				current->data, shift_set, current->data->reg_0.active, current->data->reg_1.active);
			
			if (current->data->reg_0.active)
			{
				if (current->data->shift_policy == SHIFT_POLICY_F0)
					format_0 = 0;
				else if (current->data->reg_0.expr)
					format_0 = kest_expression_compute_format(current->data->reg_0.expr, scope, 8, KEST_FPGA_DATA_WIDTH);
				
				current->data->reg_0.format = format_0;
				
				if (current->data->arg_a.type == BLOCK_OPERAND_TYPE_R && current->data->arg_a.addr == 0) format_a = format_0;
				if (current->data->arg_b.type == BLOCK_OPERAND_TYPE_R && current->data->arg_b.addr == 0) format_b = format_0;
				if (current->data->arg_c.type == BLOCK_OPERAND_TYPE_R && current->data->arg_c.addr == 0) format_c = format_0;
			}
			
			kest_string_appendf(&string, "register 0 format: %d\n", format_0);
			
			if (current->data->reg_1.active)
			{
				if (current->data->shift_policy == SHIFT_POLICY_F0)
					format_1 = 0;
				else if (current->data->reg_1.expr)
					format_1 = kest_expression_compute_format(current->data->reg_1.expr, scope, 8, KEST_FPGA_DATA_WIDTH);
				
				current->data->reg_1.format = format_1;
				
				if (current->data->arg_a.type == BLOCK_OPERAND_TYPE_R && current->data->arg_a.addr == 1) format_a = format_1;
				if (current->data->arg_b.type == BLOCK_OPERAND_TYPE_R && current->data->arg_b.addr == 1) format_b = format_1;
				if (current->data->arg_c.type == BLOCK_OPERAND_TYPE_R && current->data->arg_c.addr == 1) format_c = format_1;
			}
			
			kest_string_appendf(&string, "register 1 format: %d\n", format_1);
			
			current->data->shift = shift_policy_determine_shift(current->data->shift_policy, format_a, format_b, format_c, current->data->shift);
			
			str = kest_string_to_native(&string);
			KEST_PRINTF("%s", str);
			kest_free(str);
			kest_string_drain(&string);
		}
		
		current = current->next;
		i++;
	}
	
	kest_string_destroy(&string);
	
	KEST_PRINTF("kest_compute_register_formats done\n");
	
	return NO_ERROR;
}

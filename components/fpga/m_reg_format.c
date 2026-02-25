#include <stdio.h>

#include "m_int.h"

int m_compute_register_formats(m_block_pll *blocks, m_parameter_pll *parameters)
{
	printf("m_compute_register_formats\n");
	if (!blocks) return NO_ERROR;
	
	m_block_pll *current = blocks;
	
	m_interval range;
	float min, max;
	float abs_min, abs_max;
	float max_abs;
	int format;
	int shift_set;
	float p2;
	
	int i = 0;
	while (current)
	{
		if (current->data)
		{
			shift_set = current->data->shift_set;
			format = current->data->shift;
			
			if (current->data->reg_0.active)
			{
				if (!shift_set && current->data->reg_0.expr)
				{
					range = m_expression_compute_range(current->data->reg_0.expr, parameters);
					min = range.a;
					max = range.b;
					
					printf("Block %d register 0 has range [%.06f, %.06f], so ", i, min, max);
					
					abs_min = (min < 0) ? -min : min;
					abs_max = (max < 0) ? -max : max;
					
					max_abs = abs_min > abs_max ? abs_min : abs_max;
					
					printf("max absolute value: %f; ", max_abs);
					
					format = 0;
					p2 = 1.0;
					
					while (p2 < max_abs && format < 8)
					{
						p2 *= 2.0;
						format++;
					}
					
					printf("needed format: q%d.%d\n", format + 1, 16 - format - 1);
					
					current->data->shift = format;
				}
				
				current->data->reg_0.format = format;
			}
			
			if (current->data->reg_1.active)
			{
				if (!shift_set && current->data->reg_1.expr)
				{
					range = m_expression_compute_range(current->data->reg_1.expr, parameters);
					min = range.a;
					max = range.b;
					
					printf("Block %d register 1 has min %f and maximum %f, so ", i, min, max);
					
					abs_min = (min < 0) ? -min : min;
					abs_max = (max < 0) ? -max : max;
					
					max_abs = abs_min > abs_max ? abs_min : abs_max;
					
					printf("max absolute value: %f; ", max_abs);
					
					format = 0;
					p2 = 1.0;
					
					while (p2 < max_abs && format < 8)
					{
						p2 *= 2.0;
						format++;
					}
					
					printf("needed format: q%d.%d\n", format + 1, M_FPGA_DATA_WIDTH - format - 1);
				}
				
				current->data->reg_1.format = format;
			}
		}
		
		current = current->next;
		i++;
	}
	
	return NO_ERROR;
}

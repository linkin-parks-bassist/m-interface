#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "m_int.h"

IMPLEMENT_LINKED_PTR_LIST(m_dsp_resource);

int m_init_dsp_resource(m_dsp_resource *res)
{
	if (!res)
		return ERR_NULL_PTR;
	
	res->name = NULL;
	res->type = M_DSP_RESOURCE_NOTHING;
	res->size = NULL;
	res->delay = NULL;
	res->handle = 0;
	res->data = NULL;
	res->mem_size = 0;
	
	return NO_ERROR;
}

int string_to_resource_type(const char *type_str)
{
	if ((strcmp(type_str, "delay_buffer") == 0) || (strcmp(type_str, "delay") == 0))
	{
		return M_DSP_RESOURCE_DELAY;
	}
	else if ((strcmp(type_str, "mem") == 0) || (strcmp(type_str, "memory") == 0))
	{
		return M_DSP_RESOURCE_MEM;
	}
	
	return M_DSP_RESOURCE_NOTHING;
}

int m_resources_assign_handles(m_dsp_resource_pll *list)
{
	int next_delay_handle = 0;
	int next_mem_handle = 0;
	
	m_dsp_resource_pll *current = list;
	
	int i = 0;
	while (current)
	{
		if (current->data)
		{
			switch (current->data->type)
			{
				case M_DSP_RESOURCE_DELAY:
					printf("Assigning \"%s\" handle %d...\n", current->data->name, next_delay_handle);
					current->data->handle = next_delay_handle;
					next_delay_handle += 1;
					break;
				
				case M_DSP_RESOURCE_MEM:
					current->data->handle = next_mem_handle;
					next_mem_handle += current->data->mem_size;
					break;
			}
		}
		
		current = current->next;
	}
	
	return NO_ERROR;
}

m_eff_resource_report empty_m_eff_resource_report()
{
	m_eff_resource_report result;
	
	memset(&result, 0, sizeof(result));
	
	return result;
}

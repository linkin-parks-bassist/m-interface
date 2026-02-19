#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "resources.h"
#include "tokenizer.h"
#include "block.h"
#include "dq.h"
#include "m_parser.h"
#include "asm.h"
#include "dictionary.h"

int m_dsp_resource_init(m_dsp_resource *res)
{
	if (!res)
		return ERR_NULL_PTR;
	
	res->name = NULL;
	res->type = M_DSP_RESOURCE_NOTHING;
	res->size = 0;
	res->delay = 0;
	res->handle = 0;
	res->data = NULL;
	
	return NO_ERROR;
}

int m_dsp_resource_pll_safe_append(m_dsp_resource_pll **list_ptr, m_dsp_resource *x)
{
	if (!list_ptr)
		return ERR_NULL_PTR;
	
	m_dsp_resource_pll *node = m_alloc(sizeof(m_dsp_resource_pll));
	
	if (!node)
		return ERR_ALLOC_FAIL;
	
	node->data = x;
	node->next = NULL;
	
	if (*list_ptr)
	{
		m_dsp_resource_pll *current = *list_ptr;
		
		while (current->next)
			current = current->next;
		
		current->next = node;
	}
	else
	{
		*list_ptr = node;
	}
	
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

int m_extract_delay_buffer_from_dict(m_dictionary *dict, m_dsp_resource *res)
{
	if (!dict || !res)
		return ERR_NULL_PTR;
	
	int ret_val;
	
	float buf_size;
	float delay;
	int size_found = 0;
	
	ret_val = m_dictionary_lookup_float(dict, "size", (void*)&buf_size);
	
	if (ret_val == NO_ERROR)
	{
		size_found = 1;
		res->size = (int)ceilf(buf_size * M_FPGA_SAMPLE_RATE * 0.001);
	}
	
	ret_val = m_dictionary_lookup_float(dict, "delay", (void*)&delay);
		
	if (ret_val == NO_ERROR)
	{
		res->delay = (int)roundf(delay * M_FPGA_SAMPLE_RATE * 0.001);
	}
	else
	{
		printf("Could not find attribute \"delay\" for delay buffer \"%s\"; error code %d\n", res->name, ret_val);
		return ERR_BAD_ARGS;
	}
	
	if (!size_found)
		res->size = (int)ceilf(delay * M_FPGA_SAMPLE_RATE * 0.001);
	
	if (res->size < res->delay)
		res->size = res->delay;
	
	res->size -= 1;
	res->size |= res->size >> 1;
	res->size |= res->size >> 2;
	res->size |= res->size >> 4;
	res->size |= res->size >> 8;
	res->size |= res->size >> 16;
	res->size += 1;
	
	printf("Found delay \"%s\", size %d, delay %d\n", res->name, res->size, res->delay);
	
	return NO_ERROR;
}

int m_extract_mem_from_dict(m_dictionary *dict, m_dsp_resource *res)
{
	if (!dict || !res)
		return ERR_NULL_PTR;
	
	int ret_val;
	
	int size = 1;
	
	ret_val = m_dictionary_lookup_int(dict, "size", (void*)&size);
	
	res->size = size;
	
	return NO_ERROR;
}

m_dsp_resource *m_extract_resource_from_dict(m_dictionary *dict)
{
	if (!dict)
		return NULL;
	
	int ret_val;
	float delay_len;
	char *type_str = NULL;
	m_dsp_resource *res = m_alloc(sizeof(m_dsp_resource));
	
	if (!res) return NULL;
	
	m_dsp_resource_init(res);
	
	res->name = m_strndup(dict->name, 128);
	
	if (!res->name)
		goto resource_extract_abort;
	
	ret_val = m_dictionary_lookup_str(dict, "type", (void*)&type_str);
	
	if (ret_val != NO_ERROR || !type_str)
	{
		printf("Could not find attribute \"type\" for resource \"%s\"\n", dict->name);
		goto resource_extract_abort;
	}
	
	res->type = string_to_resource_type(type_str);
	
	if (res->type == M_DSP_RESOURCE_NOTHING)
	{
		printf("Error: resource type \"%s\" unrecognised\n", type_str);
		goto resource_extract_abort;
	}
	else if (res->type == M_DSP_RESOURCE_DELAY)
	{
		m_extract_delay_buffer_from_dict(dict, res);
	}
	else if (res->type == M_DSP_RESOURCE_MEM)
	{
		m_extract_mem_from_dict(dict, res);
	}
	
	return res;
	
resource_extract_abort:
	if (res)
	{
		if (res->name)
			m_free(res->name);
		
		m_free(res);
	}
	
	return NULL;
}

int m_extract_resources(m_dsp_resource_pll **list, m_ast_node *sect)
{
	if (!list || !sect)
		return ERR_NULL_PTR;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)sect->data;
	
	if (!sec) return ERR_BAD_ARGS;
	
	m_dictionary *dict = sec->dict;
	m_dsp_resource *res= NULL;
	
	if (!dict)
		return ERR_BAD_ARGS;
	
	for (int i = 0; i < dict->n_entries; i++)
	{
		if (dict->entries[i].type == DICT_ENTRY_TYPE_SUBDICT)
		{
			res = m_extract_resource_from_dict(dict->entries[i].value.val_dict);
			
			if (res)
				m_dsp_resource_pll_safe_append(list, res);
		}
	}
	
	return NO_ERROR;
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
					next_mem_handle += current->data->size;
					break;
			}
		}
		
		current = current->next;
	}
	
	return NO_ERROR;
}

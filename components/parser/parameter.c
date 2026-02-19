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
#include "parameter.h"


m_parameter_pll *m_parameter_pll_append(m_parameter_pll *pll, m_parameter *param)
{
	m_parameter_pll *nl = m_alloc(sizeof(m_parameter_pll));
	nl->data = param;
	nl->next = NULL;
	
	if (!pll)
		return nl;
	
	m_parameter_pll *current = pll;
	
	while (current->next)
		current = current->next;
	
	current->next = nl;
	
	return pll;
}

m_parameter *new_m_parameter_wni(const char *name, char *name_internal, float value, float min, float max)
{
	m_parameter *param = m_alloc(sizeof(m_parameter));
	
	param->name_internal = name_internal;
	param->value = value;
	
	return param;
}

int m_parameter_pll_safe_append(m_parameter_pll **list_ptr, m_parameter *x)
{
	if (!list_ptr)
		return ERR_NULL_PTR;
	
	m_parameter_pll *node = m_alloc(sizeof(m_parameter_pll));
	
	if (!node)
		return ERR_ALLOC_FAIL;
	
	node->data = x;
	node->next = NULL;
	
	if (*list_ptr)
	{
		m_parameter_pll *current = *list_ptr;
		
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

m_parameter *m_extract_parameter_from_dict(m_dictionary *dict)
{
	if (!dict)
		return NULL;
	
	int ret_val;
	m_parameter *param = m_alloc(sizeof(m_parameter));
	
	if (!param) return NULL;
	
	param->name = NULL;
	param->name_internal = NULL;
	param->value = 1.0;
	param->min = 0.0;
	param->min_dq = NULL;
	param->max = 100.0;
	param->max_dq = NULL;
	param->scale = PARAMETER_SCALE_LINEAR;
	param->max_velocity = 0.1;
	param->updated = 1;
	param->widget_type = PARAM_WIDGET_VIRTUAL_POT;
	param->group = -1;
	
	param->name_internal = m_strndup(dict->name, 128);
	
	if (!param->name_internal)
		goto parameter_extract_abort;
	
	char *str;
	float v;
	int i;
	m_derived_quantity *dq;
	
	if ((ret_val = m_dictionary_lookup_str(dict, "name", (void*)&str)) == NO_ERROR)
	{
		param->name = m_strndup(dict->name, 128);
	}
	else
	{
		printf("Error: could not find mandatory attribute \"name\" for parameter \"%s\"\n", param->name_internal);
		goto parameter_extract_abort;
	}
	
	if ((ret_val = m_dictionary_lookup_float(dict, "default", (void*)&v)) == NO_ERROR)
	{
		param->value = v;
	}
	else
	{
		printf("Error: could not find mandatory attribute \"default\" for parameter \"%s\"\n", param->name_internal);
		goto parameter_extract_abort;
	}
	
	if ((ret_val = m_dictionary_lookup_float(dict, "min", (void*)&v)) == NO_ERROR)
	{
		param->min = v;
		param->min_dq = new_m_derived_quantity_const_float(v);
	}
	else if ((ret_val = m_dictionary_lookup_dq(dict, "min", (void*)&dq)) == NO_ERROR)
	{
		param->min_dq = dq;
	}
	
	if ((ret_val = m_dictionary_lookup_float(dict, "max", (void*)&v)) == NO_ERROR)
	{
		param->max = v;
		param->max_dq = new_m_derived_quantity_const_float(v);
	}
	else if ((ret_val = m_dictionary_lookup_dq(dict, "max", (void*)&dq)) == NO_ERROR)
	{
		param->max_dq = dq;
	}
	
	if ((ret_val = m_dictionary_lookup_str(dict, "scale", (void*)&str)) == NO_ERROR)
	{
		if (strcmp(str, "linear") == 0 || strcmp(str, "flat") == 0)
		{
			param->scale = PARAMETER_SCALE_LINEAR;
		}
		else if (strcmp(str, "log")  == 0 || strcmp(str, "logarithmic") == 0
		      || strcmp(str, "exp")  == 0 || strcmp(str, "exponential") == 0
		      || strcmp(str, "freq") == 0 || strcmp(str, "octave") == 0)
		{
			param->scale = PARAMETER_SCALE_LOGARITHMIC;
		}
		else
		{
			printf("Warning: unknown scale \"%s\" given to parameter \"%s\". Defaulting to linear.\n", str, param->name_internal);
		}
	}
	
	if ((ret_val = m_dictionary_lookup_float(dict, "max_velocity", (void*)&v)) == NO_ERROR)
	{
		param->max_velocity = v;
	}
	
	if ((ret_val = m_dictionary_lookup_str(dict, "widget_type", (void*)&str)) == NO_ERROR)
	{
		if (strcmp(str, "dial") == 0 || strcmp(str, "potentiometer") == 0)
		{
			param->widget_type = PARAM_WIDGET_VIRTUAL_POT;
		}
		else if (strcmp(str, "slider")  == 0 || strcmp(str, "slider_horizontal") == 0
		      || strcmp(str, "hslider")  == 0)
		{
			param->widget_type = PARAM_WIDGET_HSLIDER;
		}
		else if (strcmp(str, "slider_vertical") == 0
		      || strcmp(str, "vslider")  == 0)
		{
			param->widget_type = PARAM_WIDGET_VSLIDER;
		}
		else if (strcmp(str, "slider_tall_vertical")  == 0 || strcmp(str, "slider_vertical_tall") == 0
		      || strcmp(str, "vslider_tall")  == 0)
		{
			param->widget_type = PARAM_WIDGET_VSLIDER_TALL;
		}
		else
		{
			printf("Warning: unknown widget type \"%s\" given to parameter \"%s\". Defaulting to dial.\n", str, param->name_internal);
		}
	}
	
	if ((ret_val = m_dictionary_lookup_int(dict, "group", (void*)&i)) == NO_ERROR)
	{
		param->group = i;
	}
	
	if (param->value < param->min)
		param->value = param->min;
	
	if (param->value > param->max)
		param->value = param->max;
	
	/*
	printf("Extracted a parameter;\n");
	printf("\tname: \"%s\"\n", param->name);
	printf("\tname_internal: \"%s\"\n", param->name_internal);
	printf("\tvalue: %f\n", param->value);
	printf("\tmin: %f\n", param->min);
	printf("\tmin_dq: %p\n", param->min_dq);
	printf("\tmax: %f\n", param->max);
	printf("\tmax_dq: %p\n", param->max_dq);
	printf("\tscale: %d\n", param->scale);
	printf("\tmax_velocity: %f\n", param->max_velocity);
	printf("\twidget_type: %d\n", param->widget_type);
	printf("\tgroup: %d\n", param->group);
	*/
	
	return param;
	
parameter_extract_abort:
	if (param)
	{
		if (param->name)
			m_free(param->name);
		
		m_free(param);
	}
	
	return NULL;
}

int m_extract_parameters(m_parameter_pll **list, m_ast_node *sect)
{
	if (!list || !sect)
		return ERR_NULL_PTR;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)sect->data;
	
	if (!sec) return ERR_BAD_ARGS;
	
	m_dictionary *dict = sec->dict;
	m_parameter *param = NULL;
	
	if (!dict)
		return ERR_BAD_ARGS;
	
	for (int i = 0; i < dict->n_entries; i++)
	{
		if (dict->entries[i].type == DICT_ENTRY_TYPE_SUBDICT)
		{
			param = m_extract_parameter_from_dict(dict->entries[i].value.val_dict);
			
			if (param)
				m_parameter_pll_safe_append(list, param);
		}
	}
	
	return NO_ERROR;
}

int m_parameters_assign_ids(m_parameter_pll *list)
{
	int next_parameter_id = 0;
	
	m_parameter_pll *current = list;
	
	while (current)
	{
		if (current->data)
			current->data->id.parameter_id = next_parameter_id;
		
		current = current->next;
	}
	
	return NO_ERROR;
}

void clone_parameter(m_parameter *dest, m_parameter *src)
{
	if (!dest || !src)
		return;
	
	dest->value = src->value;
	dest->min = src->min;
	dest->min_dq = src->min_dq;
	dest->max = src->max;
	dest->max_dq = src->max_dq;
	
	
	dest->widget_type = src->widget_type;
	dest->name 	= src->name;
	dest->name_internal = src->name_internal;
	dest->units = src->units;
	
	dest->max_velocity = src->max_velocity;
	
	dest->scale = src->scale;
	
	dest->group = src->group;
}

m_parameter *m_parameter_make_clone(m_parameter *src)
{
	if (!src)
		return NULL;
	
	m_parameter *param = m_alloc(sizeof(m_parameter));
	
	if (!param)
		return NULL;
	
	clone_parameter(param, src);
	
	return param;
}

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "m_int.h"

m_dictionary *m_new_dictionary()
{
	m_dictionary *dict = m_alloc(sizeof(m_dictionary));
	
	if (!dict)
		return NULL;
	
	dict->entries = m_alloc(sizeof(m_dictionary_entry) * 8);
	
	if (!dict->entries)
	{
		m_free(dict);
		return NULL;
	}
	
	dict->n_entries = 0;
	dict->entry_array_length = 8;
	
	return dict;
}

int m_dictionary_ensure_capacity(m_dictionary *dict)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	m_dictionary_entry *na;
	
	if (dict->n_entries == dict->entry_array_length)
	{
		na = malloc(sizeof(m_dictionary_entry) * dict->entry_array_length * 2);
		
		if (!na) return ERR_ALLOC_FAIL;
		
		dict->entries = na;
		dict->entry_array_length *= 2;
	}
	
	return NO_ERROR;
}

int m_dictionary_add_entry(m_dictionary *dict, m_dictionary_entry entry)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name = strndup(entry.name, 32);
	dict->entries[dict->n_entries].type = entry.type;
	dict->entries[dict->n_entries].value = entry.value;

	dict->n_entries++;
	
	return ret_val;
}

int m_dictionary_add_entry_str(m_dictionary *dict, const char *name, const char *value)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	if (!name || !value)
		return ERR_BAD_ARGS;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name  = name;
	dict->entries[dict->n_entries].type = DICT_ENTRY_TYPE_STR;
	dict->entries[dict->n_entries].value.val_string = value;

	dict->n_entries++;
	
	return NO_ERROR;
}

int m_dictionary_add_entry_int(m_dictionary *dict, const char *name, int value)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	if (!name || !value)
		return ERR_BAD_ARGS;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name  = name;
	dict->entries[dict->n_entries].type = DICT_ENTRY_TYPE_INT;
	dict->entries[dict->n_entries].value.val_int = value;

	dict->n_entries++;
	
	return NO_ERROR;
}

int m_dictionary_add_entry_float(m_dictionary *dict, const char *name, float value)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	if (!name || !value)
		return ERR_BAD_ARGS;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name  = name;
	dict->entries[dict->n_entries].type = DICT_ENTRY_TYPE_FLOAT;
	dict->entries[dict->n_entries].value.val_float = value;

	dict->n_entries++;
	
	return NO_ERROR;
}

int m_dictionary_add_entry_expr(m_dictionary *dict, const char *name, m_expression *value)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	if (!name || !value)
		return ERR_BAD_ARGS;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name  = name;
	dict->entries[dict->n_entries].type = DICT_ENTRY_TYPE_DQ;
	dict->entries[dict->n_entries].value.val_expr = value;

	dict->n_entries++;
	
	return NO_ERROR;
}

int m_dictionary_add_entry_dict(m_dictionary *dict, const char *name, m_dictionary *value)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	if (!name || !value)
		return ERR_BAD_ARGS;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name  = name;
	dict->entries[dict->n_entries].type = DICT_ENTRY_TYPE_SUBDICT;
	dict->entries[dict->n_entries].value.val_dict = value;

	dict->n_entries++;
	
	return NO_ERROR;
}

int m_dictionary_lookup(m_dictionary *dict, const char *name, void *result, int type)
{
	if (!dict || !result || !name)
		return ERR_NULL_PTR;
	
	for (int i = 0; i < dict->n_entries; i++)
	{
		if (strcmp(dict->entries[i].name, name) == 0)
		{
			if (dict->entries[i].type == type)
			{
				switch (type)
				{
					case DICT_ENTRY_TYPE_STR:
						*((const char**)result) = dict->entries[i].value.val_string;
						break;
					case DICT_ENTRY_TYPE_FLOAT:
						*((float*)result) = dict->entries[i].value.val_float;
						break;
					case DICT_ENTRY_TYPE_INT:
						*((int*)result) = dict->entries[i].value.val_int;
						break;
					case DICT_ENTRY_TYPE_DQ:
						*((m_expression**)result) = dict->entries[i].value.val_expr;
						break;
					case DICT_ENTRY_TYPE_SUBDICT:
						*((m_dictionary**)result) = dict->entries[i].value.val_dict;
						break;
				}
			}
			else if (type == DICT_ENTRY_TYPE_FLOAT && dict->entries[i].type == DICT_ENTRY_TYPE_INT)
			{
				*((float*)result) = (float)dict->entries[i].value.val_int;
			}
			else if (type == DICT_ENTRY_TYPE_INT && dict->entries[i].type == DICT_ENTRY_TYPE_FLOAT)
			{
				*((int*)result) = (int)roundf(dict->entries[i].value.val_float);
			}
			else
			{
				return ERR_WRONG_TYPE;
			}
			
			return NO_ERROR;
		}
	}
	
	return ERR_NOT_FOUND;
}

int m_dictionary_lookup_str(m_dictionary *dict, const char *name, const char **result)
{
	return m_dictionary_lookup(dict, name, result, DICT_ENTRY_TYPE_STR);
}

int m_dictionary_lookup_float(m_dictionary *dict, const char *name, float *result)
{
	return m_dictionary_lookup(dict, name, result, DICT_ENTRY_TYPE_FLOAT);
}

int m_dictionary_lookup_int(m_dictionary *dict, const char *name, int *result)
{
	return m_dictionary_lookup(dict, name, result, DICT_ENTRY_TYPE_INT);
}

int m_dictionary_lookup_expr(m_dictionary *dict, const char *name, m_expression **result)
{
	return m_dictionary_lookup(dict, name, result, DICT_ENTRY_TYPE_DQ);
}

int m_dictionary_lookup_dict(m_dictionary *dict, const char *name, m_dictionary **result)
{
	return m_dictionary_lookup(dict, name, result, DICT_ENTRY_TYPE_SUBDICT);
}

void print_dict_entry(m_dictionary_entry *entry)
{
	if (!entry)
	{
		printf("(null)");
		return;
	}
	printf("%s: ", entry->name);
	
	switch (entry->type)
	{
		case DICT_ENTRY_TYPE_STR:
			printf("\"%s\"", entry->value.val_string);
			break;
		
		case DICT_ENTRY_TYPE_INT:
			printf("%d", entry->value.val_int);
			break;
		
		case DICT_ENTRY_TYPE_FLOAT:
			printf("%f", entry->value.val_float);
			break;
		
		case DICT_ENTRY_TYPE_DQ:
			printf("(expression)");
			break;
			
		default:
			printf("mangled !");
			break;
	}
}

void print_dict(m_dictionary *dict)
{
	printf("Dictionary ");
	if (!dict)
	{
		printf("(null)\n");
		return;
	}
	
	printf("\"%s\" (%d entries):\n", dict->name, dict->n_entries);
	
	for (int i = 0; i < dict->n_entries; i++)
	{
		printf("\t");
		print_dict_entry(&dict->entries[i]);
		printf("\n");
	}
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
	param->min_expr = NULL;
	param->max = 100.0;
	param->max_expr = NULL;
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
	m_expression *expr;
	
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
		param->min_expr = new_m_expression_const(v);
	}
	else if ((ret_val = m_dictionary_lookup_expr(dict, "min", (void*)&expr)) == NO_ERROR)
	{
		param->min_expr = expr;
	}
	
	if ((ret_val = m_dictionary_lookup_float(dict, "max", (void*)&v)) == NO_ERROR)
	{
		param->max = v;
		param->max_expr = new_m_expression_const(v);
	}
	else if ((ret_val = m_dictionary_lookup_expr(dict, "max", (void*)&expr)) == NO_ERROR)
	{
		param->max_expr = expr;
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
	printf("\tmin_expr: %p\n", param->min_expr);
	printf("\tmax: %f\n", param->max);
	printf("\tmax_expr: %p\n", param->max_expr);
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
	
	m_init_dsp_resource(res);
	
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

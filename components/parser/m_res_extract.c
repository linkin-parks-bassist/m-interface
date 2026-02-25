#include "m_int.h"

m_parameter *m_extract_parameter_from_dict(m_eff_parsing_state *ps, m_ast_node *dict_node, m_dictionary *dict)
{
	if (!dict)
		return NULL;
	
	int ret_val;
	m_parameter *param = m_alloc(sizeof(m_parameter));
	
	if (!param) return NULL;
	
	init_parameter_str(param);
	
	param->name_internal = m_strndup(dict->name, 128);
	
	if (!param->name_internal)
		goto parameter_extract_abort;
	
	char *str;
	float v;
	int i;
	m_expression *expr;
	
	if ((ret_val = m_dictionary_lookup_str(dict, "name", (void*)&str)) == NO_ERROR)
	{
		param->name = m_strndup(str, 128);
		printf("Obtained parameter name; \"%s\"\n", param->name);
	}
	else
	{
		m_parser_error_at_node(ps, dict_node, "Could not find mandatory attribute \"name\" for parameter \"%s\"", param->name_internal);
		goto parameter_extract_abort;
	}
	
	if ((ret_val = m_dictionary_lookup_expr(dict, "default", &expr)) == NO_ERROR)
	{
		if (!m_expression_is_constant(expr))
		{
			m_parser_error_at_node(ps, dict_node, "Default value must be constant");
			goto parameter_extract_abort;
		}
		
		param->value = m_expression_evaluate(expr, NULL);
	}
	else
	{
		m_parser_error_at_node(ps, dict_node, "Could not find mandatory attribute \"default\" for parameter \"%s\"", param->name_internal);
		goto parameter_extract_abort;
	}
	
	if ((ret_val = m_dictionary_lookup_float(dict, "min", (void*)&v)) == NO_ERROR)
	{
		param->min = v;
	}
	
	if ((ret_val = m_dictionary_lookup_expr(dict, "min_expr", (void*)&expr)) == NO_ERROR)
		param->min_expr = expr;
	//else
	//	param->min_expr = new_m_expression_const(v);
	
	if ((ret_val = m_dictionary_lookup_float(dict, "max", (void*)&v)) == NO_ERROR)
	{
		param->max = v;
	}
	
	if ((ret_val = m_dictionary_lookup_expr(dict, "max_expr", (void*)&expr)) == NO_ERROR)
		param->max_expr = expr;
	//else
	//	param->max_expr = new_m_expression_const(v);
	
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
			m_parser_warn_at_node(ps, dict_node, "Unknown scale \"%s\" given to parameter \"%s\". Defaulting to linear.", str, param->name_internal);
		}
	}
	
	if ((ret_val = m_dictionary_lookup_str(dict, "units", (void*)&str)) == NO_ERROR)
	{
		param->units = m_strndup(str, 128);
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
			m_parser_warn_at_node(ps, dict_node, "Unknown widget type \"%s\" given to parameter \"%s\". Defaulting to dial.", str, param->name_internal);
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

int m_extract_delay_buffer_from_dict(m_eff_parsing_state *ps, m_ast_node *dict_node, m_dictionary *dict, m_dsp_resource *res)
{
	if (!dict || !res)
		return ERR_NULL_PTR;
	
	int ret_val;
	
	m_expression *expr;
	
	ret_val = m_dictionary_lookup_expr(dict, "size", &expr);
	
	if (ret_val == NO_ERROR)
	{
		res->size = expr;
	}
	else
	{
		res->size = NULL;
	}
	
	ret_val = m_dictionary_lookup_expr(dict, "delay", &expr);
		
	if (ret_val == NO_ERROR)
	{
		res->delay = expr;
	}
	else
	{
		m_parser_error_at_node(ps, dict_node, "Could not find mandatory attribute \"delay\" for delay buffer \"%s\"", res->name);
		return ERR_BAD_ARGS;
	}

	printf("Found delay \"%s\", size %p, delay %p\n", res->name, res->size, res->delay);
	
	return NO_ERROR;
}

int m_extract_mem_from_dict(m_eff_parsing_state *ps, m_ast_node *dict_node, m_dictionary *dict, m_dsp_resource *res)
{
	if (!dict || !res)
		return ERR_NULL_PTR;
	
	int ret_val;
	
	int size = 1;
	
	m_expression *expr;
	
	ret_val = m_dictionary_lookup_expr(dict, "size", &expr);
	
	if (ret_val == NO_ERROR)
	{
		size = (int)(roundf(fabs(m_expression_evaluate(expr, NULL))));
		
		if (size == 0)
			size = 1;
		
		res->mem_size = size;
	}
	else
	{
		res->mem_size = 1;
	}
	
	return NO_ERROR;
}

m_dsp_resource *m_extract_resource_from_dict(m_eff_parsing_state *ps, m_ast_node *dict_node, m_dictionary *dict)
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
		m_parser_error_at_node(ps, dict_node, "Could not find attribute \"type\" for resource \"%s\"", dict->name);
		goto resource_extract_abort;
	}
	
	res->type = string_to_resource_type(type_str);
	
	if (res->type == M_DSP_RESOURCE_NOTHING)
	{
		m_parser_error_at_node(ps, dict_node, "Resource type \"%s\" unrecognised", type_str);
		goto resource_extract_abort;
	}
	else if (res->type == M_DSP_RESOURCE_DELAY)
	{
		m_extract_delay_buffer_from_dict(ps, dict_node, dict, res);
	}
	else if (res->type == M_DSP_RESOURCE_MEM)
	{
		m_extract_mem_from_dict(ps, dict_node, dict, res);
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


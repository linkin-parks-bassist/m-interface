#include "m_int.h"

int get_section_start_score(char *str, int current_score)
{
	if (!str)
		return 0;
	
	if (token_is_char(str, '\n')) return 1;
	
	switch (current_score)
	{
		case 0: return 0;
		case 1: return (token_is_char(str, '.')) 		  ? 2 : 0;
		case 2: return (token_is_valid_section_name(str)) ? 3 : 0;
	}
	
	return 0;
}

int m_parameters_section_extract(m_parameter_pll **list, m_ast_node *sect)
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

int m_resources_section_extract(m_dsp_resource_pll **list, m_ast_node *sect)
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

int m_dictionary_section_lookup_str(m_ast_node *section, const char *name, const char **result)
{
	if (!section) return ERR_NULL_PTR;
	if (section->type != M_AST_NODE_SECTION) return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	if (!sec || !sec->dict) return ERR_BAD_ARGS;
	
	return m_dictionary_lookup_str(sec->dict, name, result);
}

int m_dictionary_section_lookup_float(m_ast_node *section, const char *name, float *result)
{
	if (!section) return ERR_NULL_PTR;
	if (section->type != M_AST_NODE_SECTION) return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	if (!sec || !sec->dict) return ERR_BAD_ARGS;
	
	return m_dictionary_lookup_float(sec->dict, name, result);
}

int m_dictionary_section_lookup_int(m_ast_node *section, const char *name, int *result)
{
	if (!section) return ERR_NULL_PTR;
	if (section->type != M_AST_NODE_SECTION) return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	if (!sec || !sec->dict) return ERR_BAD_ARGS;
	
	return m_dictionary_lookup_int(sec->dict, name, result);
}

int m_dictionary_section_lookup_expr(m_ast_node *section, const char *name, m_expression **result)
{
	if (!section) return ERR_NULL_PTR;
	if (section->type != M_AST_NODE_SECTION) return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	if (!sec || !sec->dict) return ERR_BAD_ARGS;
	
	return m_dictionary_lookup_expr(sec->dict, name, result);
}

int m_dictionary_section_lookup_dict(m_ast_node *section, const char *name, m_dictionary **result)
{
	if (!section) return ERR_NULL_PTR;
	if (section->type != M_AST_NODE_SECTION) return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	if (!sec || !sec->dict) return ERR_BAD_ARGS;
	
	return m_dictionary_lookup_dict(sec->dict, name, result);
}

#ifndef M_EFF_DESC_FILE_SECTIONS_H_
#define M_EFF_DESC_FILE_SECTIONS_H_

typedef struct {
	char *name;
	m_token_ll *tokens;
	struct m_dictionary *dict;
} m_eff_desc_file_section;

int get_section_start_score(char *str, int current_score);

int m_parameters_section_extract(m_eff_parsing_state *ps, m_parameter_pll **list, struct m_ast_node *sect);
int m_resources_section_extract(m_eff_parsing_state *ps, m_dsp_resource_pll **list, struct m_ast_node *sect);

int m_dictionary_section_lookup_str  (m_ast_node *section, const char *name, const char **result);
int m_dictionary_section_lookup_float(m_ast_node *section, const char *name, float *result);
int m_dictionary_section_lookup_int  (m_ast_node *section, const char *name, int *result);
int m_dictionary_section_lookup_expr (m_ast_node *section, const char *name, m_expression **result);
int m_dictionary_section_lookup_dict (m_ast_node *section, const char *name, m_dictionary **result);

#endif

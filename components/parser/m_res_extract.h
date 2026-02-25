#ifndef M_RESOURCE_EXTRACT_H_
#define M_RESOURCE_EXTRACT_H_

m_parameter *m_extract_parameter_from_dict	(m_eff_parsing_state *ps, m_ast_node *dict_node, m_dictionary *dict);
int m_extract_delay_buffer_from_dict		(m_eff_parsing_state *ps, m_ast_node *dict_node, m_dictionary *dict, m_dsp_resource *res);
int m_extract_mem_from_dict					(m_eff_parsing_state *ps, m_ast_node *dict_node, m_dictionary *dict, m_dsp_resource *res);
m_dsp_resource *m_extract_resource_from_dict(m_eff_parsing_state *ps, m_ast_node *dict_node, m_dictionary *dict);

#endif

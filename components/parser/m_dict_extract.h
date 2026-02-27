#ifndef M_DICT_EXTRACT_H_
#define M_DICT_EXTRACT_H_

m_setting      *m_extract_setting_from_dict	 (m_eff_parsing_state *ps, m_ast_node *dict_node, m_dictionary *dict);
m_parameter    *m_extract_parameter_from_dict(m_eff_parsing_state *ps, m_ast_node *dict_node, m_dictionary *dict);
m_dsp_resource *m_extract_resource_from_dict (m_eff_parsing_state *ps, m_ast_node *dict_node, m_dictionary *dict);

#endif

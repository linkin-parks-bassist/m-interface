#ifndef M_EFFECT_H_
#define M_EFFECT_H_

typedef struct {
	const char *name;
	
	m_block_pll *blocks;
	m_parameter_pll *parameters;
	m_dsp_resource_pll *resources;
	
	m_eff_resource_report res_rpt;
	
	m_expr_scope *scope;
} m_effect_desc;

DECLARE_LINKED_PTR_LIST(m_effect_desc);

int m_init_effect_desc(m_effect_desc *eff);
int m_effect_desc_generate_res_rpt(m_effect_desc *eff);

m_expr_scope *m_eff_desc_create_scope(m_effect_desc *eff);

#endif

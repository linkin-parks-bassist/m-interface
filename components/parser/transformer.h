#ifndef TRANSFORMER_H_
#define TRANSFORMER_H_

typedef struct {
	int block_position;
	
	m_parameter_pll *parameters;
	m_effect_desc *eff;
} m_transformer;

typedef struct m_transformer_pll {
	m_transformer *data;
	struct m_transformer_pll *next;
} m_transformer_pll;

int init_transformer(m_transformer *trans);
int m_transformer_pll_safe_append(m_transformer_pll **list_ptr, m_transformer *x);

int init_transformer_from_effect_desc(m_transformer *trans, m_effect_desc *eff);

#endif

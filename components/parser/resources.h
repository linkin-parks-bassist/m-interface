#ifndef M_RESOURCES_H_
#define M_RESOURCES_H_

#define M_DSP_RESOURCE_NOTHING	0
#define M_DSP_RESOURCE_DELAY	1
#define M_DSP_RESOURCE_MEM		2

typedef struct m_dsp_resource {
	char *name;
	int type;
	int handle;
	int size;
	int delay;
	void *data;
} m_dsp_resource;

int m_dsp_resource_init(m_dsp_resource *res);

typedef struct m_dsp_resource_pll {
	m_dsp_resource *data;
	struct m_dsp_resource_pll *next;
} m_dsp_resource_pll;

struct m_ast_node;

int m_extract_resources(m_dsp_resource_pll **list, struct m_ast_node *sect);
int m_resources_assign_handles(m_dsp_resource_pll *list);

#endif

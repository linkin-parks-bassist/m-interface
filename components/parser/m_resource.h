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

int m_init_dsp_resource(m_dsp_resource *res);

int string_to_resource_type(const char *type_str);

DECLARE_LINKED_PTR_LIST(m_dsp_resource);

int m_resources_assign_handles(m_dsp_resource_pll *list);

typedef struct
{
	unsigned int blocks;
	unsigned int memory;
	unsigned int delays;
} m_eff_resource_report;

#endif

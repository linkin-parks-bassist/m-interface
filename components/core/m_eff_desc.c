#include "m_int.h"

IMPLEMENT_LINKED_PTR_LIST(m_effect_desc);

int m_init_effect_desc(m_effect_desc *eff)
{
	if (!eff) return ERR_NULL_PTR;
	
	eff->parameters = NULL;
	eff->resources = NULL;
	eff->blocks = NULL;
	
	return NO_ERROR;
}

int m_effect_desc_generate_res_rpt(m_effect_desc *eff)
{
	if (!eff)
		return ERR_NULL_PTR;
	
	unsigned int blocks = 0;
	unsigned int memory = 0;
	unsigned int delays = 0;
	
	m_block_pll *cb = eff->blocks;
	
	while (cb)
	{
		blocks++;
		cb = cb->next;
	}
	
	m_dsp_resource_pll *cr = eff->resources;
	
	while (cr)
	{
		if (cr->data)
		{
			switch (cr->data->type)
			{
				case M_DSP_RESOURCE_MEM:
					memory += cr->data->size;
					break;
				case M_DSP_RESOURCE_DELAY:
					delays += 1;
					break;
			}
		}
		cr = cr->next;
	}
	
	eff->res_rpt.blocks = blocks;
	eff->res_rpt.memory = memory;
	eff->res_rpt.delays = delays;
	
	return NO_ERROR;
}


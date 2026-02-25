#include "m_int.h"

int init_m_pipeline(m_pipeline *pipeline)
{
	if (!pipeline)
		return ERR_NULL_PTR;
	
	pipeline->transformers = NULL;
	
	return NO_ERROR;
}

m_transformer *m_pipeline_append_transformer_eff(m_pipeline *pipeline, m_effect_desc *eff)
{
	if (!pipeline || !eff)
		return NULL;
	
	m_transformer *trans = m_alloc(sizeof(m_transformer));
	
	if (!trans)
		return NULL;
	
	m_transformer_pll *node = m_alloc(sizeof(m_transformer_pll));
	
	if (!node)
		return NULL;
	
	node->data = trans;
	node->next = NULL;
	
	init_transformer_from_effect_desc(trans, eff);
	
	if (!pipeline->transformers)
	{
		trans->id = 0;
		pipeline->transformers = node;
	}
	else
	{
		int least_free_id = 0;
		m_transformer_pll *current = pipeline->transformers;
		
		while (current)
		{
			if (current->data)
			{
				if (current->data->id >= least_free_id)
					least_free_id = current->data->id + 1;
			}
			
			if (current->next)
				current = current->next;
			else
				break;
		}
		
		trans->id = least_free_id;
		current->next = node;
	}
	
	printf("Created transformer with id %d\n", trans->id);
	
	return trans;
}

int m_pipeline_remove_transformer(m_pipeline *pipeline, uint16_t id)
{
	printf("m_pipeline_remove_transformer\n");
	if (!pipeline)
		return ERR_NULL_PTR;
	
	m_transformer_pll *current = pipeline->transformers;
	m_transformer_pll *prev = NULL;
	
	while (current)
	{
		if (current->data && current->data->id == id)
		{
			if (current->data)
				free_transformer(current->data);
			
			if (prev)
				prev->next = current->next;
			else
				pipeline->transformers = current->next;
			
			m_free(current);
			
			printf("m_pipeline_remove_transformer found and vanquished the transformer\n");
			return NO_ERROR;
		}
		
		prev = current;
		current = current->next;
	}
	
	
	printf("m_pipeline_remove_transformer finished without finding the transformer\n");
	return ERR_INVALID_TRANSFORMER_ID;
}

int m_pipeline_move_transformer(m_pipeline *pipeline, int new_pos, int old_pos)
{
	if (!pipeline)
		return ERR_NULL_PTR;
	
	if (!pipeline->transformers)
		return ERR_BAD_ARGS;
	
	m_transformer_pll *target  = NULL;
	
	int i = 0;
	m_transformer_pll *current = pipeline->transformers;
	m_transformer_pll *prev    = NULL;
	
	while (current && i < old_pos)
	{
		prev = current;
		current = current->next;
		i++;
	}
	
	if (!current)
		return ERR_BAD_ARGS;
	
	target = current;
	
	if (prev)
		prev->next = target->next;
	else
		pipeline->transformers = target->next;

	i = 0;
	prev = NULL;
	current = pipeline->transformers;
	
	while (current && i < new_pos)
	{
		prev = current;
		current = current->next;
		i++;
	}
	
	target->next = current;
	
	if (!prev)
		pipeline->transformers = target;
	else
		prev->next = target;
	
	return NO_ERROR;
}

int m_pipeline_get_n_transformers(m_pipeline *pipeline)
{
	if (!pipeline)
		return -ERR_NULL_PTR;
	
	int n = 0;
	
	m_transformer_pll *current = pipeline->transformers;
	
	while (current)
	{
		if (current->data)
			n++;
		current = current->next;
	}
	
	return n;
}

int clone_pipeline(m_pipeline *dest, m_pipeline *src)
{
	if (!src || !dest)
		return ERR_NULL_PTR;
	
	printf("Cloning pipeline...\n");
	
	m_transformer_pll *current = src->transformers;
	m_transformer_pll *nl;
	m_transformer *trans = NULL;
	
	int i = 0;
	while (current)
	{
		printf("Cloning transformer %d... current = %p, current->next = %p\n", i, current, current->next);
		if (current->data)
		{
			trans = m_alloc(sizeof(m_transformer));
			
			if (!trans)
				return ERR_ALLOC_FAIL;
			
			clone_transformer(trans, current->data);
			
			nl = m_transformer_pll_append(dest->transformers, trans);
		
			if (nl)
				dest->transformers = nl;
		}
		
		current = current->next;
		i++;
	}
	
	return NO_ERROR;
}

void gut_pipeline(m_pipeline *pipeline)
{
	if (!pipeline)
		return;
	
	destructor_free_m_transformer_pll(pipeline->transformers, free_transformer);
	pipeline->transformers = NULL;
}

int m_pipeline_create_fpga_transfer_batch(m_pipeline *pipeline, m_fpga_transfer_batch *batch)
{
	if (!batch)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	if (!pipeline)
	{
		ret_val = ERR_BAD_ARGS;
		goto return_nothing;
	}
	
	if (!pipeline->transformers)
		goto return_nothing;
	
	m_fpga_transfer_batch result = m_new_fpga_transfer_batch();
	
	m_eff_resource_report rpt = empty_m_eff_resource_report();
	
	int pos = 0;
	ret_val = m_fpga_batch_append_transformers(&result, pipeline->transformers, &rpt, &pos);
	
	if (ret_val != NO_ERROR)
	{
		m_free_fpga_transfer_batch(result);
		goto return_nothing;
	}
	
	*batch = result;
	
	return ret_val;
	
return_nothing:
	batch->buf = NULL;
	batch->buf_len = 0;
	batch->len = 0;
	batch->buffer_owned = 1;
	
	return ret_val;
}

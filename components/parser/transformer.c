#include <stdlib.h>
#include <stdio.h>

#include "tokenizer.h"
#include "block.h"
#include "dq.h"
#include "m_parser.h"
#include "asm.h"
#include "dictionary.h"
#include "reg.h"
#include "transformer.h"
#include "encode.h"

int init_transformer(m_transformer *trans)
{
	if (!trans)
		return ERR_NULL_PTR;
	
	trans->eff = NULL;
	trans->parameters = NULL;
	
	return NO_ERROR;
}

m_transformer_pll *m_transformer_pll_append(m_transformer_pll *list_ptr, m_transformer *x)
{
	if (!list_ptr)
		return NULL;
	
	m_transformer_pll *node = m_alloc(sizeof(m_transformer_pll));
	
	if (!node)
		return NULL;
	
	node->data = x;
	node->next = NULL;
	
	if (list_ptr)
	{
		m_transformer_pll *current = list_ptr;
		
		while (current && current->next)
			current = current->next;
		
		current->next = node;
		
		return list_ptr;
	}
	else
	{
		return node;
	}
	
	return NO_ERROR;
}

int m_transformer_pll_safe_append(m_transformer_pll **list_ptr, m_transformer *x)
{
	if (!list_ptr)
		return ERR_NULL_PTR;
	
	m_transformer_pll *node = m_alloc(sizeof(m_transformer_pll));
	
	if (!node)
		return ERR_ALLOC_FAIL;
	
	node->data = x;
	node->next = NULL;
	
	if (*list_ptr)
	{
		m_transformer_pll *current = *list_ptr;
		
		while (current && current->next)
			current = current->next;
		
		current->next = node;
	}
	else
	{
		*list_ptr = node;
	}
	
	return NO_ERROR;
}

int init_transformer_from_effect_desc(m_transformer *trans, m_effect_desc *eff)
{
	init_transformer(trans);
	trans->eff = eff;
	
	m_parameter_pll *current = eff->parameters;
	
	while (current)
	{
		m_parameter_pll_safe_append(&trans->parameters, m_parameter_make_clone(current->data));
		current = current->next;
	}
	
	return NO_ERROR;
}

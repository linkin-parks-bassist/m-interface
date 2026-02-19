#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "tokenizer.h"
#include "block.h"
#include "dq.h"
#include "m_parser.h"
#include "asm.h"

int m_block_pll_safe_append(m_block_pll **list_ptr, m_block *x)
{
	if (!list_ptr)
		return ERR_NULL_PTR;
	
	m_block_pll *node = m_alloc(sizeof(m_block_pll));
	
	if (!node)
		return ERR_ALLOC_FAIL;
	
	node->data = x;
	node->next = NULL;
	
	if (*list_ptr)
	{
		m_block_pll *current = *list_ptr;
		
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

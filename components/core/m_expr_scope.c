#include "m_int.h"

IMPLEMENT_LINKED_PTR_LIST(m_expr_scope_entry);

m_expr_scope_entry *m_new_expr_scope_entry_expr(const char *name, struct m_expression *expr)
{
	if (!name || !expr)
		return NULL;
	
	m_expr_scope_entry *result = m_alloc(sizeof(m_expr_scope_entry));
	
	if (!result)
		return NULL;
	
	result->type = M_SCOPE_ENTRY_TYPE_EXPR;
	result->name = name;
	result->val.expr = expr;
	
	return result;
}

m_expr_scope_entry *m_new_expr_scope_entry_param(m_parameter *param)
{
	if (!param)
		return NULL;
	
	m_expr_scope_entry *result = m_alloc(sizeof(m_expr_scope_entry));
	
	if (!result)
		return NULL;
	
	result->type = M_SCOPE_ENTRY_TYPE_PARAM;
	result->name = param->name_internal;
	result->val.param = param;
	
	return result;
}

m_expr_scope *m_new_expr_scope()
{
	m_expr_scope *result = m_alloc(sizeof(m_expr_scope));
	
	if (!result)
		return NULL;
	
	result->entries = NULL;
	
	return result;
}

int m_expr_scope_add_expr(m_expr_scope *scope, const char *name, struct m_expression *expr)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	if (!name || !expr)
		return ERR_BAD_ARGS;
	
	m_expr_scope_entry *entry = m_new_expr_scope_entry_expr(name, expr);
	
	if (!entry)
		return ERR_ALLOC_FAIL;
	
	int ret_val = m_expr_scope_entry_pll_safe_append(&scope->entries, entry);
	
	return ret_val;
}

int m_expr_scope_add_param(m_expr_scope *scope, m_parameter *param)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	if (!param)
		return ERR_BAD_ARGS;
	
	m_expr_scope_entry *entry = m_new_expr_scope_entry_param(param);
	
	if (!entry)
		return ERR_ALLOC_FAIL;
	
	int ret_val = m_expr_scope_entry_pll_safe_append(&scope->entries, entry);
	
	return ret_val;
}

m_expr_scope_entry *m_expr_scope_fetch(m_expr_scope *scope, const char *name)
{
	if (!scope || !name)
		return NULL;
	
	m_expr_scope_entry_pll *current = scope->entries;
	
	while (current)
	{
		if (current->data)
		{
			if (current->data->name)
			{
				if (strcmp(current->data->name, name) == 0)
				{
					return current->data;
				}
			}
		}
		
		current = current->next;
	}
	
	return NULL;
}

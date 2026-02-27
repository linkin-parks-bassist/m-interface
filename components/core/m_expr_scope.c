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

m_expr_scope_entry *m_new_expr_scope_entry_setting(struct m_setting *setting)
{
	if (!setting)
		return NULL;
	
	printf("m_new_expr_scope_entry_setting(setting = %p)\n", setting);
	m_expr_scope_entry *result = m_alloc(sizeof(m_expr_scope_entry));
	
	if (!result)
		return NULL;
	
	result->type = M_SCOPE_ENTRY_TYPE_SETTING;
	result->name = setting->name_internal;
	result->val.setting = setting;
	
	printf("\tresult->type = %d\n\tresult->name = \"%s\"\n\tresult->val.setting = %p\n",
		result->type, result->name, result->val.setting);
	
	return result;
}

int m_expr_scope_init(m_expr_scope *scope)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	scope->entries = NULL;
	
	return NO_ERROR;
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

int m_expr_scope_add_setting(m_expr_scope *scope, m_setting *setting)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	if (!setting)
		return ERR_BAD_ARGS;
	
	m_expr_scope_entry *entry = m_new_expr_scope_entry_setting(setting);
	
	if (!entry)
		return ERR_ALLOC_FAIL;
	
	int ret_val = m_expr_scope_entry_pll_safe_append(&scope->entries, entry);
	
	return ret_val;
}

int m_expr_scope_add_params(m_expr_scope *scope, m_parameter_pll *params)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	int ret_val;
	m_parameter_pll *current = params;
	
	while (current)
	{
		if ((ret_val = m_expr_scope_add_param(scope, current->data)) != NO_ERROR)
			return ret_val;
		
		current = current->next;
	}
	
	return NO_ERROR;
}

int m_expr_scope_add_settings(m_expr_scope *scope, m_setting_pll *settings)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	printf("m_expr_scope_add_settings(scope = %p, settings = %p)\n", scope, settings);
	
	int ret_val;
	m_setting_pll *current = settings;
	
	printf("current = %p\n", current);
	while (current)
	{
		printf("current->data = %p\n", current->data);
		printf("Adding setting \"%s\"...\n", current->data ? current->data->name_internal : "(NULL)");
		if ((ret_val = m_expr_scope_add_setting(scope, current->data)) != NO_ERROR)
		{
			
			return ret_val;
		}
		
		current = current->next;
		printf("current = %p\n", current);
	}
	
	return NO_ERROR;
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

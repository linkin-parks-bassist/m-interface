#include "kest_int.h"

#define PRINTLINES_ALLOWED 0

static const char *FNAME = "kest_expr_scope.c";

int kest_scope_entry_init(kest_scope_entry *entry)
{
	if (!entry)
		return ERR_NULL_PTR;
	
	memset(entry, 0, sizeof(kest_scope_entry));
	
	kest_dependent_list_init(&entry->dependents);
	
	return NO_ERROR;
}

int kest_scope_entry_init_expr(kest_scope_entry *entry, kest_expression *expr)
{
	if (!entry)
		return ERR_NULL_PTR;
	
	if (!expr)
		return ERR_BAD_ARGS;
	
	kest_scope_entry_init(entry);
	
	entry->type = KEST_SCOPE_ENTRY_TYPE_EXPR;
	entry->val.expr = expr;
	entry->updated = 0;
	
	return NO_ERROR;
}

int kest_scope_entry_init_param(kest_scope_entry *entry, kest_parameter *param)
{
	if (!entry)
		return ERR_NULL_PTR;
		
	if (!param)
		return ERR_BAD_ARGS;
	
	kest_scope_entry_init(entry);
	
	entry->type = KEST_SCOPE_ENTRY_TYPE_PARAM;
	entry->val.param = param;
	entry->updated = 0;
	
	return NO_ERROR;
}

int kest_scope_entry_init_setting(kest_scope_entry *entry, kest_setting *setting)
{
	if (!entry)
		return ERR_NULL_PTR;
	
	if (!setting)
		return ERR_BAD_ARGS;
	
	kest_scope_entry_init(entry);
	
	entry->type = KEST_SCOPE_ENTRY_TYPE_SETTING;
	entry->val.setting = setting;
	entry->updated = 0;
	
	return NO_ERROR;
}

int kest_scope_entry_init_lfo(kest_scope_entry *entry, kest_lfo *lfo)
{
	if (!entry)
		return ERR_NULL_PTR;
	
	if (!lfo)
		return ERR_BAD_ARGS;
	
	kest_scope_entry_init(entry);
	
	entry->type = KEST_SCOPE_ENTRY_TYPE_LFO;
	entry->val.lfo = lfo;
	lfo->scope_entry = entry; // TODO: don't be like this
	entry->updated = 0;
	
	return NO_ERROR;
}

int kest_scope_entry_init_mem(kest_scope_entry *entry, kest_mem_slot *mem)
{
	if (!entry)
		return ERR_NULL_PTR;
	
	if (!mem)
		return ERR_BAD_ARGS;
	
	kest_scope_entry_init(entry);
	
	entry->type = KEST_SCOPE_ENTRY_TYPE_MEM;
	entry->val.mem = mem;
	entry->updated = 0;
	
	return NO_ERROR;
}

int kest_scope_entry_add_dependent_scope_entry(kest_scope_entry *entry, const char *key)
{
	if (!entry)
		return ERR_NULL_PTR;
	
	if (!key)
		return ERR_BAD_ARGS;
	
	if (!entry->dependents.entries)
		return kest_dependent_list_append(&entry->dependents, kest_dependent_scope_entry(key));
	
	for (size_t i = 0; i < entry->dependents.count; i++)
	{
		if (entry->dependents.entries[i].type == KEST_DEPENDENT_SCOPE_ENTRY
		 && strcmp(entry->dependents.entries[i].data.entry_key, key) == 0)
			return NO_ERROR;
	}
	
	return kest_dependent_list_append(&entry->dependents, kest_dependent_scope_entry(key));
}

int kest_scope_entry_add_driven_parameter(kest_scope_entry *entry, kest_parameter *param)
{
	if (!entry)
		return ERR_NULL_PTR;
	
	if (!param)
		return ERR_BAD_ARGS;
	
	if (!entry->dependents.entries)
		return kest_dependent_list_append(&entry->dependents, kest_dependent_driven_parameter(param));
	
	for (size_t i = 0; i < entry->dependents.count; i++)
	{
		if (entry->dependents.entries[i].type == KEST_DEPENDENT_DRIVEN_PARAMETER
		 && entry->dependents.entries[i].data.param == param)
			return NO_ERROR;
	}
	
	return kest_dependent_list_append(&entry->dependents, kest_dependent_driven_parameter(param));
}

int kest_scope_entry_add_bound_dependent_parameter(kest_scope_entry *entry, kest_parameter *param)
{
	if (!entry)
		return ERR_NULL_PTR;
	
	if (!param)
		return ERR_BAD_ARGS;
	
	if (!entry->dependents.entries)
		return kest_dependent_list_append(&entry->dependents, kest_dependent_bound_parameter(param));
	
	for (size_t i = 0; i < entry->dependents.count; i++)
	{
		if (entry->dependents.entries[i].type == KEST_DEPENDENT_BOUND_PARAMETER
		 && entry->dependents.entries[i].data.param == param)
			return NO_ERROR;
	}
	
	return kest_dependent_list_append(&entry->dependents, kest_dependent_bound_parameter(param));
}

int kest_scope_entry_add_dependent_block_reg(kest_scope_entry *entry, int block, int reg, int format)
{
	if (!entry)
		return ERR_NULL_PTR;
	
	if (!entry->dependents.entries)
		return kest_dependent_list_append(&entry->dependents, kest_dependent_block_reg(block, reg, format));
	
	for (size_t i = 0; i < entry->dependents.count; i++)
	{
		if (entry->dependents.entries[i].type == KEST_DEPENDENT_BLOCK_REG
		 && entry->dependents.entries[i].data.block_reg.block == block
		 && entry->dependents.entries[i].data.block_reg.reg == reg)
			return NO_ERROR;
	}
	
	return kest_dependent_list_append(&entry->dependents, kest_dependent_block_reg(block, reg, format));
}

int kest_scope_entry_add_dependent_filter_coef(kest_scope_entry *entry, int filter, int coef, int format)
{
	if (!entry)
		return ERR_NULL_PTR;
	
	if (!entry->dependents.entries)
		return kest_dependent_list_append(&entry->dependents, kest_dependent_filter_coef(filter, coef, format));
	
	for (size_t i = 0; i < entry->dependents.count; i++)
	{
		if (entry->dependents.entries[i].type == KEST_DEPENDENT_FILTER_COEF
		 && entry->dependents.entries[i].data.filter_coef.filter == filter
		 && entry->dependents.entries[i].data.filter_coef.coef == coef)
			return NO_ERROR;
	}
	
	return kest_dependent_list_append(&entry->dependents, kest_dependent_filter_coef(filter, coef, format));
}

int kest_scope_entry_add_dependent(kest_scope_entry *entry, kest_dependent dep)
{
	if (!entry)
		return ERR_NULL_PTR;
	
	switch (dep.type)
	{
		case KEST_DEPENDENT_SCOPE_ENTRY:
			return kest_scope_entry_add_dependent_scope_entry(entry, dep.data.entry_key);
		case KEST_DEPENDENT_BLOCK_REG:
			return kest_scope_entry_add_dependent_block_reg(entry, dep.data.block_reg.block, dep.data.block_reg.reg, dep.format);
		case KEST_DEPENDENT_FILTER_COEF:
			return kest_scope_entry_add_dependent_filter_coef(entry, dep.data.filter_coef.filter, dep.data.filter_coef.coef, dep.format);
		case KEST_DEPENDENT_BOUND_PARAMETER:
			return kest_scope_entry_add_bound_dependent_parameter(entry, dep.data.param);
		case KEST_DEPENDENT_DRIVEN_PARAMETER:
			return kest_scope_entry_add_driven_parameter(entry, dep.data.param);
	}
	
	return ERR_BAD_ARGS;
}

int kest_scope_entry_print(kest_scope_entry *entry, kest_string *str)
{
	if (!entry || !str)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	switch (entry->type)
	{
		case KEST_SCOPE_ENTRY_TYPE_EXPR:
			kest_string_appendf(str, "Expression \"%s\"", kest_expression_to_string(entry->val.expr));
			break;

		case KEST_SCOPE_ENTRY_TYPE_PARAM:
			kest_string_appendf(str, "Parameter \"%s\"", entry->val.param ? entry->val.param->name : "(NULL)");
			break;

		case KEST_SCOPE_ENTRY_TYPE_SETTING:
			kest_string_appendf(str, "Setting \"%s\"", entry->val.param ? entry->val.param->name : "(NULL)");
			break;

		case KEST_SCOPE_ENTRY_TYPE_MEM:
			kest_string_appendf(str, "Mem slot %d", entry->val.mem ? entry->val.mem->addr : -1);
			break;

		case KEST_SCOPE_ENTRY_TYPE_LFO:
			kest_string_appendf(str, "LFO");
			break;

		default:
			kest_string_appendf(str, "Unknown");
			break;
	}
	
	return ret_val;
}

IMPLEMENT_DICT(kest_scope_entry);

kest_scope *kest_scope_new()
{
	kest_scope *scope = kest_alloc(sizeof(kest_scope));
	
	if (!scope)
		return NULL;
	
	int ret_val = kest_scope_init(scope);
	
	if (ret_val != NO_ERROR)
	{
		kest_free(scope);
		return NULL;
	}
	
	return scope;
}

int kest_scope_init(kest_scope *scope)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	scope->count = 0;
	
	int ret_val = kest_scope_entry_dict_init(&scope->dict, 32);
	
	if (ret_val != NO_ERROR)
		return ret_val;
	
	kest_scope_add_expr(scope, "pi", &kest_expression_pi);
	kest_scope_add_expr(scope, "tau", &kest_expression_2pi);
	kest_scope_add_expr(scope, "e", &kest_expression_e);
	kest_scope_add_expr(scope, "sample_rate", &kest_expression_sample_rate);
	kest_scope_add_expr(scope, "data_width", &kest_expression_data_width);
	
	return NO_ERROR;
}

int kest_scope_add_entry(kest_scope *scope, const char *name, kest_scope_entry entry)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	int ret_val = kest_scope_entry_dict_insert(&scope->dict, name, entry);
	
	if (ret_val == NO_ERROR)
		scope->count++;
	
	return ret_val;
}

int kest_scope_add_mem(kest_scope *scope, const char *name, kest_mem_slot *mem)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	kest_scope_entry entry;
	
	int ret_val = kest_scope_entry_init_mem(&entry, mem);
	
	if (ret_val != NO_ERROR)
		return ret_val;
	
	return kest_scope_add_entry(scope, name, entry);
}

int kest_scope_add_expr(kest_scope *scope, const char *name, struct kest_expression *expr)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	kest_scope_entry entry;
	
	int ret_val = kest_scope_entry_init_expr(&entry, expr);
	
	if (ret_val != NO_ERROR)
		return ret_val;
	
	return kest_scope_add_entry(scope, name, entry);
}

int kest_scope_add_param(kest_scope *scope, kest_parameter *param)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	kest_scope_entry entry;
	
	int ret_val = kest_scope_entry_init_param(&entry, param);
	
	if (ret_val != NO_ERROR)
		return ret_val;
	
	return kest_scope_add_entry(scope, param->name_internal, entry);
}

int kest_scope_add_setting(kest_scope *scope, kest_setting *setting)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	kest_scope_entry entry;
	
	int ret_val = kest_scope_entry_init_setting(&entry, setting);
	
	if (ret_val != NO_ERROR)
		return ret_val;
	
	return kest_scope_add_entry(scope, setting->name_internal, entry);
}

kest_scope_entry *kest_scope_add_entry_return_ptr(kest_scope *scope, const char *name, kest_scope_entry entry)
{
	if (!scope) return NULL;
	
	scope->count++;
	
	return kest_scope_entry_dict_insert_return_ptr(&scope->dict, name, entry);
}

kest_scope_entry *kest_scope_add_lfo_return_entry(kest_scope *scope, const char *name, kest_lfo *lfo)
{
	if (!scope)
		return NULL;
	
	kest_scope_entry entry;
	
	int ret_val = kest_scope_entry_init_lfo(&entry, lfo);
	
	if (ret_val != NO_ERROR)
		return NULL;
	
	return kest_scope_add_entry_return_ptr(scope, name, entry);
}

kest_scope_entry *kest_scope_add_mem_return_entry(kest_scope *scope, const char *name, kest_mem_slot *mem)
{
	if (!scope)
		return NULL;
	
	kest_scope_entry entry;
	
	int ret_val = kest_scope_entry_init_mem(&entry, mem);
	
	if (ret_val != NO_ERROR)
		return NULL;
	
	return kest_scope_add_entry_return_ptr(scope, name, entry);
}

kest_scope_entry *kest_scope_add_expr_return_entry(kest_scope *scope, const char *name, kest_expression *expr)
{
	if (!scope)
		return NULL;
	
	kest_scope_entry entry;
	
	int ret_val = kest_scope_entry_init_expr(&entry, expr);
	
	if (ret_val != NO_ERROR)
		return NULL;
	
	return kest_scope_add_entry_return_ptr(scope, name, entry);
}

kest_scope_entry *kest_scope_add_param_return_entry(kest_scope *scope, kest_parameter *param)
{
	if (!scope)
		return NULL;
	
	kest_scope_entry entry;
	
	int ret_val = kest_scope_entry_init_param(&entry, param);
	
	if (ret_val != NO_ERROR)
		return NULL;
	
	return kest_scope_add_entry_return_ptr(scope, param->name_internal, entry);
}

kest_scope_entry *kest_scope_add_setting_return_entry(kest_scope *scope, kest_setting *setting)
{
	if (!scope)
		return NULL;
	
	kest_scope_entry entry;
	
	int ret_val = kest_scope_entry_init_setting(&entry, setting);
	
	if (ret_val != NO_ERROR)
		return NULL;
	
	return kest_scope_add_entry_return_ptr(scope, setting->name_internal, entry);
}

int kest_scope_add_params(kest_scope *scope, kest_parameter_pll *params)
{
	if (!scope || !params)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	kest_parameter_pll *current = params;
	
	while (current)
	{
		ret_val = kest_scope_add_param(scope, current->data);
		
		if (ret_val != NO_ERROR)
			return ret_val;
		
		current = current->next;
	}
	
	return ret_val;
}

int kest_scope_add_settings(kest_scope *scope, kest_setting_pll *settings)
{
	if (!scope || !settings)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	kest_setting_pll *current = settings;
	
	while (current)
	{
		ret_val = kest_scope_add_setting(scope, current->data);
		
		if (ret_val != NO_ERROR)
			return ret_val;
		
		current = current->next;
	}
	
	return ret_val;
}

kest_scope_entry *kest_scope_fetch(kest_scope *scope, const char *name)
{
	return kest_scope_entry_dict_lookup(&scope->dict, name);
}

kest_scope_entry *kest_scope_lookup(kest_scope *scope, const char *name)
{
	return kest_scope_entry_dict_lookup(&scope->dict, name);
}

size_t kest_scope_count(kest_scope *scope)
{
	if (!scope)
		return 0;
	
	return scope->count;
	return kest_scope_entry_dict_count(&scope->dict);
}

kest_scope_entry *kest_scope_index(kest_scope *scope, size_t n)
{
	if (!scope)
		return NULL;
		
	return kest_scope_entry_dict_index(&scope->dict, n);
}

const char *kest_scope_index_key(kest_scope *scope, size_t n)
{
	if (!scope)
		return NULL;
	
	return kest_scope_entry_dict_index_key(&scope->dict, n);
}

int kest_scope_entry_propagate_updates(kest_scope_entry *entry)
{
	if (!entry)
		return 0;
	
	
	
	return 0;
}

int kest_scope_detect_env_updates(kest_scope *scope)
{
	KEST_PRINTF("kest_scope_detect_env_updates(scope = %p)\n", scope);
	
	if (!scope)
		return ERR_NULL_PTR;
	
	int any_updates = 0;
	
	size_t n = scope->count;
	kest_scope_entry *entry = NULL;
	
	for (size_t i = 0; i < n; i++)
	{
		entry = kest_scope_index(scope, i);
		
		if (!entry)
			continue;
		
		KEST_PRINTF("entry[%d]: \"%s\"\n", i, kest_scope_index_key(scope, i));
		
		switch (entry->type)
		{
			case KEST_SCOPE_ENTRY_TYPE_PARAM:
				KEST_PRINTF("Type: parameter\n");
				
				kest_parameter_if_driven_refresh(entry->val.param);
				entry->updated = entry->val.param ? entry->val.param->updated : 0;
				
				KEST_PRINTF("entry->updated: %d\n", entry->updated);
				break;
				
			case KEST_SCOPE_ENTRY_TYPE_SETTING:
				KEST_PRINTF("Type: setting\n");
				entry->updated = entry->val.setting ? entry->val.setting->updated : 0;
				KEST_PRINTF("entry->updated: %d\n", entry->updated);
				break;
				
			case KEST_SCOPE_ENTRY_TYPE_MEM:
				KEST_PRINTF("Type: mem\n");
				KEST_PRINTF("entry->updated: %d\n", entry->updated);
				break;
				
			default:
				break;
		}
		
		any_updates |= entry->updated;
	}
	
	KEST_PRINTF("kest_scope_detect_env_updates(scope = %p) done\n", scope);
	return any_updates;
}

int kest_scope_propagate_updates(kest_scope *scope)
{
	KEST_PRINTF("kest_scope_propagate_updates(scope = %p)\n", scope);
	
	if (!scope)
		return ERR_NULL_PTR;
	
	kest_scope_entry *entry = NULL;
	kest_scope_entry *ref_entry = NULL;
	size_t n = scope->count;
	const char *current_key;
	int ret_val;
	
	KEST_PRINTF("scope->count = %d\n", n);
	KEST_PRINTF("kest_scope_entry_dict_count(&scope->dict) = %d\n", kest_scope_entry_dict_count(&scope->dict));
	
	int any_updates = 0;
	
	string_list names;
	
	if (!n) return 0;
	
	char_ptr_list_init(&names);
	
	any_updates |= kest_scope_detect_env_updates(scope);
	
	for (size_t i = 0; i < n; i++)
	{
		entry = kest_scope_index(scope, i);
		current_key = kest_scope_index_key(scope, i);
		
		if (!entry)
			continue;
		
		KEST_PRINTF("entry[%d]: \"%s\". entry->updated: %d\n", i, current_key, entry->updated);
		
		if (entry->type == KEST_SCOPE_ENTRY_TYPE_EXPR)
		{
			KEST_PRINTF("... Which is an expression. Specifically, \"%s\".\n", kest_expression_to_string(entry->val.expr));
			
			ret_val = kest_expression_get_references(entry->val.expr, &names);
			
			if (ret_val != NO_ERROR)
			{
				KEST_PRINTF("Hmm, I tried to list out its expressions, but something went wrong: %s\n", kest_error_code_to_string(ret_val));
				char_ptr_list_drain(&names);
				break;
			}
			
			KEST_PRINTF("It refers to: %s\n", (names.count == 0) ? "nothing" : "");
			
			for (int j = 0; j < names.count; j++)
			{
				KEST_PRINTF("\t\"%s\", ", names.entries[j]);
				ref_entry = kest_scope_lookup(scope, names.entries[j]);
				
				if (!ref_entry)
				{
					KEST_PRINTF("which is not found in scope !\n");
					continue;
				}
				
				if (ref_entry->updated)
				{
					KEST_PRINTF("which is updated! Therefore ya boy %s is too.\n", current_key);
					entry->updated = 1;
					any_updates = 1;
					break;
				}
				else
				{
					KEST_PRINTF("which is not updated. Oh well.\n");
				}
			}
			
			char_ptr_list_drain(&names);
		}
		
		any_updates |= entry->updated;
	}
	
	char_ptr_list_destroy(&names);
	
	return any_updates;
}

#define PRINTLINES_ALLOWED 0

int kest_scope_detect_dependencies(kest_scope *scope)
{
	KEST_PRINTF("kest_scope_detect_dependencies(scope = %p)\n", scope);
	
	if (!scope)
		return ERR_NULL_PTR;
	
	size_t n = scope->count;
	
	kest_scope_entry *entry = NULL;
	kest_scope_entry *ref_entry = NULL;
	kest_parameter *param = NULL;
	
	const char *current_key;
	int ret_val;
	
	int any_updates = 0;
	
	string_list names;
	
	if (!n) return 0;
	
	char_ptr_list_init(&names);
	
	for (size_t i = 0; i < n; i++)
	{
		entry = kest_scope_index(scope, i);
		current_key = kest_scope_index_key(scope, i);
		
		if (!entry)
			continue;
		
		KEST_PRINTF("entry[%d] (%p): \"%s\".\n", i, entry, current_key, entry->updated);
		
		if (entry->type == KEST_SCOPE_ENTRY_TYPE_EXPR)
		{
			//KEST_PRINTF("... Which is an expression. Specifically, \"%s\".\n", kest_expression_to_string(entry->val.expr));
			
			ret_val = kest_expression_get_references(entry->val.expr, &names);
			
			if (ret_val != NO_ERROR)
			{
				//KEST_PRINTF("Hmm, I tried to list out its expressions, but something went wrong: %s\n", kest_error_code_to_string(ret_val));
				char_ptr_list_drain(&names);
				break;
			}
			
			//KEST_PRINTF("It refers to: %s\n", (names.count == 0) ? "nothing" : "");
			
			for (int j = 0; j < names.count; j++)
			{
				//KEST_PRINTF_("\t\"%s\",\n", names.entries[j]);
				ref_entry = kest_scope_lookup(scope, names.entries[j]);
				
				if (!ref_entry)
				{
					//KEST_PRINTF_("which is not found in scope !\n");
					continue;
				}
				
				kest_scope_entry_add_dependent_scope_entry(ref_entry, current_key);
			}
			
			char_ptr_list_drain(&names);
		}
		else if (entry->type == KEST_SCOPE_ENTRY_TYPE_PARAM)
		{
			KEST_PRINTF("... Which is a parameter.\n");
			
			param = entry->val.param;
			
			if (!param)
				continue;
			
			if (param->max_expr)
			{
				KEST_PRINTF("Its upper bound is given by %s\n", kest_expression_to_string(param->max_expr));
				kest_expression_get_references(param->max_expr, &names);
				
				KEST_PRINTF("Which refers to: %s\n", (names.count == 0) ? "nothing" : "");
				for (int j = 0; j < names.count; j++)
				{
					KEST_PRINTF_("\t\"%s\",\n", names.entries[j]);
					ref_entry = kest_scope_lookup(scope, names.entries[j]);
					
					if (!ref_entry)
						continue;
					
					kest_scope_entry_add_bound_dependent_parameter(ref_entry, param);
				}
				
				char_ptr_list_drain(&names);
			}
			
			if (param->min_expr)
			{
				KEST_PRINTF("Its lower bound is given by %s\n", kest_expression_to_string(param->min_expr));
				kest_expression_get_references(param->min_expr, &names);
				KEST_PRINTF("Which refers to: %s\n", (names.count == 0) ? "nothing" : "");
				
				for (int j = 0; j < names.count; j++)
				{
					ref_entry = kest_scope_lookup(scope, names.entries[j]);
					
					if (!ref_entry)
						continue;
					
					kest_scope_entry_add_bound_dependent_parameter(ref_entry, param);
				}
				
				char_ptr_list_drain(&names);
			}
		}
	}
	
	char_ptr_list_destroy(&names);
	
	return NO_ERROR;
}

int kest_scope_add_block_reg_dependencies(kest_scope *scope, kest_expression *expr, int block, int reg, int format)
{
	KEST_PRINTF("Adding block %d register %d dependencies to scope %p...\n", block, reg, scope);
	if (!scope || !expr)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("Expression is %s\n", kest_expression_to_string(expr));
	
	int ret_val;
	string_list names;
	kest_scope_entry *ref_entry = NULL;
	
	if ((ret_val = char_ptr_list_init(&names)) != NO_ERROR) return ret_val;
	if ((ret_val = kest_expression_get_references(expr, &names)) != NO_ERROR) return ret_val;
	
	KEST_PRINTF("Obtained %d names\n", names.count);
	
	for (size_t j = 0; j < names.count && ret_val == NO_ERROR; j++)
	{
		KEST_PRINTF("Name %d: \"%s\"\n", j, names.entries[j]);
		ref_entry = kest_scope_lookup(scope, names.entries[j]);
		
		if (!ref_entry) continue;
		KEST_PRINTF("Returned entry: %p\n", ref_entry);
		
		ret_val = kest_scope_entry_add_dependent_block_reg(ref_entry, block, reg, format);
	}
	
	return ret_val;
}

int kest_scope_add_filter_coef_dependencies(kest_scope *scope, kest_expression *expr, int filter, int coef, int format)
{
	KEST_PRINTF("Adding filter %d coefficient %d dependencies to scope %p...\n", filter, coef, scope);
	
	if (!scope || !expr)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("Expression is %s\n", kest_expression_to_string(expr));
	
	int ret_val;
	string_list names;
	kest_scope_entry *ref_entry = NULL;
	
	if ((ret_val = char_ptr_list_init(&names)) != NO_ERROR) return ret_val;
	if ((ret_val = kest_expression_get_references(expr, &names)) != NO_ERROR) return ret_val;
	
	KEST_PRINTF("Obtained %d names\n", names.count);
	
	for (int j = 0; j < names.count && ret_val == NO_ERROR; j++)
	{
		KEST_PRINTF("Name %d: \"%s\"\n", j, names.entries[j]);
		ref_entry = kest_scope_lookup(scope, names.entries[j]);
		
		if (!ref_entry) continue;
		KEST_PRINTF("Returned entry: %p\n", ref_entry);
		
		ret_val = kest_scope_entry_add_dependent_filter_coef(ref_entry, filter, coef, format);
	}
	
	return ret_val;
}



int kest_scope_transitivize_updatable_dependents_rec(kest_scope *scope, kest_scope_entry *entry, int depth)
{
	if (!scope || !entry)
		return ERR_NULL_PTR;
	
	if (depth > KEST_EXPR_REC_MAX_DEPTH)
		return ERR_RECURSION_DEPTH;
	
	kest_string str;
	kest_string_init(&str);
	
	KEST_PRINTF("[Depth %d] Transitivizing updatable dependents for scope entry ", depth);
	kest_scope_entry_print(entry, &str);
	KEST_PUTS_(str);
	kest_string_drain(&str);
	KEST_PRINTF_(". Currently has %d dependents.\n",
		entry->dependents.count);
	
	kest_scope_entry *ref_entry = NULL;
	
	for (size_t i = 0; i < entry->dependents.count; i++)
	{
		KEST_PRINTF("[Depth %d] dependent %d: ", depth, i);
		kest_string_append_dependent(&str, entry->dependents.entries[i]);
		KEST_PUTS_(str);
		kest_string_drain(&str);
		KEST_PRINTF_("\n");
	}
	
	for (size_t i = 0; i < entry->dependents.count; i++)
	{
		ref_entry = NULL;
		if (entry->dependents.entries[i].type == KEST_DEPENDENT_SCOPE_ENTRY)
		{
			ref_entry = kest_scope_lookup(scope, entry->dependents.entries[i].data.entry_key);
		}
		else if (entry->dependents.entries[i].type == KEST_DEPENDENT_DRIVEN_PARAMETER)
		{
			if (entry->dependents.entries[i].data.param)
				ref_entry = kest_scope_lookup(scope, entry->dependents.entries[i].data.param->name_internal);
		}
		
		if (!ref_entry) continue;
		
		kest_scope_transitivize_updatable_dependents_rec(scope, ref_entry, depth + 1);
		
		for (size_t j = 0; j < ref_entry->dependents.count; j++)
		{
			KEST_PRINTF("[Depth %d] Consider adding dependent ", depth);
			kest_string_append_dependent(&str, ref_entry->dependents.entries[j]);
			KEST_PUTS_(str);
			kest_string_drain(&str);
			KEST_PRINTF_(" to scope entry ");
			kest_scope_entry_print(entry, &str);
			KEST_PUTS_(str);
			kest_string_drain(&str);
			if (kest_dependent_is_updatable(ref_entry->dependents.entries[j].type))
			{
				KEST_PRINTF_(". Do it.\n");
				kest_scope_entry_add_dependent(entry, ref_entry->dependents.entries[j]);
			}
			else
			{
				KEST_PRINTF_(". Nevermind..\n");
			}
		}
	}
	
	KEST_PRINTF("[Depth %d] scope entry ", depth);
	kest_scope_entry_print(entry, &str);
	KEST_PUTS_(str);
	kest_string_drain(&str);
	KEST_PRINTF_(" now has %d dependents:\n",
		entry->dependents.count);
	
	for (size_t i = 0; i < entry->dependents.count; i++)
	{
		KEST_PRINTF("[Depth %d] dependent %d: ", depth, i);
		kest_string_append_dependent(&str, entry->dependents.entries[i]);
		KEST_PUTS_(str);
		kest_string_drain(&str);
		KEST_PRINTF_("\n");
	}
	
	kest_string_destroy(&str);
	
	return NO_ERROR;
}

int kest_scope_transitivize_updatable_dependents(kest_scope *scope)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("kest_scope_transitivize_updatable_dependents(scope = %p)\n", scope);
	
	size_t n = scope->count;
	kest_scope_entry *entry = NULL;
	
	for (size_t i = 0; i < n; i++)
	{
		entry = kest_scope_index(scope, i);
		kest_scope_transitivize_updatable_dependents_rec(scope, entry, 0);
	}
	
	return NO_ERROR;
}

#define PRINTLINES_ALLOWED 0

int kest_scope_clear_updates(kest_scope *scope)
{
	if (!scope)
		return ERR_NULL_PTR;
	
	size_t n = kest_scope_count(scope);
	
	if (!n)
		return NO_ERROR;
	
	kest_scope_entry *entry = NULL;
	
	for (size_t i = 0; i < n; i++)
	{
		entry = kest_scope_index(scope, i);
		
		if (entry) entry->updated = 0;
		else continue;
		
		if (entry->type == KEST_SCOPE_ENTRY_TYPE_PARAM)
		{
			if (entry->val.param)
				entry->val.param->updated = 0;
		}
		
		if (entry->type == KEST_SCOPE_ENTRY_TYPE_SETTING)
		{
			if (entry->val.setting)
				entry->val.setting->updated = 0;
		}
	}
	
	return NO_ERROR;
}

int kest_scope_entry_eval_rec(kest_scope_entry *entry, kest_scope *scope, float *dest, int depth)
{
	KEST_PRINTF("kest_scope_entry_eval(entry = %p, scope = %p, dest = %p)\n",
		entry, scope, dest);
	if (!entry || !dest)
		return ERR_NULL_PTR;
	
	kest_parameter *param 	= NULL;
	kest_setting *setting 	= NULL;
	kest_mem_slot *mem_slot = NULL;
	kest_lfo		   *lfo = NULL;
	
	float center;
	float freq;
	float amp;
	float t;
	
	int64_t time_ms;

	switch (entry->type)
	{
		case KEST_SCOPE_ENTRY_TYPE_EXPR:
			KEST_PRINTF("Eval expression %s\n", kest_expression_to_string(entry->val.expr));
			*dest = kest_expression_evaluate_rec(entry->val.expr, scope, depth);
			break;
			
		case KEST_SCOPE_ENTRY_TYPE_PARAM:
			KEST_PRINTF("Eval parameter %s\n", entry->val.param->name);
			param = entry->val.param;
			if (!param)
				return ERR_BAD_ARGS;
			*dest = kest_parameter_evaluate_rec(param, depth);
			break;
		
		case KEST_SCOPE_ENTRY_TYPE_SETTING:
			KEST_PRINTF("Eval setting %s\n", entry->val.setting);
			setting = entry->val.setting;
			if (!setting)
				return ERR_BAD_ARGS;
			*dest = (float)setting->value;
			break;
		
		case KEST_SCOPE_ENTRY_TYPE_MEM:
			KEST_PRINTF("Eval mem slot\n");
			mem_slot = entry->val.mem;
			
			if (!mem_slot)
				return ERR_BAD_ARGS;
			
			*dest = kest_fpga_sample_to_float(mem_slot->value);
			break;
		
		case KEST_SCOPE_ENTRY_TYPE_LFO:
			KEST_PRINTF("Eval LFO\n");
			lfo = entry->val.lfo;
			
			kest_lfo_evaluate(lfo, scope, dest);
			
			break;
		
		default: return ERR_BAD_ARGS;
	}
	
	KEST_PRINTF("kest_scope_entry_eval result: %f\n", *dest);
	
	return NO_ERROR;
}

int kest_scope_entry_eval(kest_scope_entry *entry, kest_scope *scope, float *dest)
{
	return kest_scope_entry_eval_rec(entry, scope, dest, 0);
}

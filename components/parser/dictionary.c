#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "tokenizer.h"
#include "block.h"
#include "dq.h"
#include "m_parser.h"
#include "asm.h"
#include "dictionary.h"

m_dictionary *m_new_dictionary()
{
	m_dictionary *dict = m_alloc(sizeof(m_dictionary));
	
	if (!dict)
		return NULL;
	
	dict->entries = m_alloc(sizeof(m_dict_entry) * 8);
	
	if (!dict->entries)
	{
		m_free(dict);
		return NULL;
	}
	
	dict->n_entries = 0;
	dict->entry_array_length = 8;
	
	return dict;
}

int m_dictionary_ensure_capacity(m_dictionary *dict)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	m_dict_entry *na;
	
	if (dict->n_entries == dict->entry_array_length)
	{
		na = malloc(sizeof(m_dict_entry) * dict->entry_array_length * 2);
		
		if (!na) return ERR_ALLOC_FAIL;
		
		dict->entries = na;
		dict->entry_array_length *= 2;
	}
	
	return NO_ERROR;
}

int m_dictionary_add_entry(m_dictionary *dict, m_dict_entry entry)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name = strndup(entry.name, 32);
	dict->entries[dict->n_entries].type = entry.type;
	dict->entries[dict->n_entries].value = entry.value;

	dict->n_entries++;
	
	return ret_val;
}

int m_dictionary_add_entry_str(m_dictionary *dict, const char *name, const char *value)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	if (!name || !value)
		return ERR_BAD_ARGS;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name  = name;
	dict->entries[dict->n_entries].type = DICT_ENTRY_TYPE_STR;
	dict->entries[dict->n_entries].value.val_string = value;

	dict->n_entries++;
	
	return NO_ERROR;
}

int m_dictionary_add_entry_int(m_dictionary *dict, const char *name, int value)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	if (!name || !value)
		return ERR_BAD_ARGS;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name  = name;
	dict->entries[dict->n_entries].type = DICT_ENTRY_TYPE_INT;
	dict->entries[dict->n_entries].value.val_int = value;

	dict->n_entries++;
	
	return NO_ERROR;
}

int m_dictionary_add_entry_float(m_dictionary *dict, const char *name, float value)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	if (!name || !value)
		return ERR_BAD_ARGS;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name  = name;
	dict->entries[dict->n_entries].type = DICT_ENTRY_TYPE_FLOAT;
	dict->entries[dict->n_entries].value.val_float = value;

	dict->n_entries++;
	
	return NO_ERROR;
}

int m_dictionary_add_entry_dq(m_dictionary *dict, const char *name, m_derived_quantity *value)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	if (!name || !value)
		return ERR_BAD_ARGS;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name  = name;
	dict->entries[dict->n_entries].type = DICT_ENTRY_TYPE_DQ;
	dict->entries[dict->n_entries].value.val_dq = value;

	dict->n_entries++;
	
	return NO_ERROR;
}

int m_dictionary_add_entry_dict(m_dictionary *dict, const char *name, m_dictionary *value)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	if (!name || !value)
		return ERR_BAD_ARGS;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name  = name;
	dict->entries[dict->n_entries].type = DICT_ENTRY_TYPE_SUBDICT;
	dict->entries[dict->n_entries].value.val_dict = value;

	dict->n_entries++;
	
	return NO_ERROR;
}

int m_dictionary_lookup(m_dictionary *dict, const char *name, void *result, int type)
{
	if (!dict || !result || !name)
		return ERR_NULL_PTR;
	
	for (int i = 0; i < dict->n_entries; i++)
	{
		if (strcmp(dict->entries[i].name, name) == 0)
		{
			if (dict->entries[i].type == type)
			{
				switch (type)
				{
					case DICT_ENTRY_TYPE_STR:
						*((const char**)result) = dict->entries[i].value.val_string;
						break;
					case DICT_ENTRY_TYPE_FLOAT:
						*((float*)result) = dict->entries[i].value.val_float;
						break;
					case DICT_ENTRY_TYPE_INT:
						*((int*)result) = dict->entries[i].value.val_int;
						break;
					case DICT_ENTRY_TYPE_DQ:
						*((m_derived_quantity**)result) = dict->entries[i].value.val_dq;
						break;
					case DICT_ENTRY_TYPE_SUBDICT:
						*((m_dictionary**)result) = dict->entries[i].value.val_dict;
						break;
				}
			}
			else if (type == DICT_ENTRY_TYPE_FLOAT && dict->entries[i].type == DICT_ENTRY_TYPE_INT)
			{
				*((float*)result) = (float)dict->entries[i].value.val_int;
			}
			else if (type == DICT_ENTRY_TYPE_INT && dict->entries[i].type == DICT_ENTRY_TYPE_FLOAT)
			{
				*((int*)result) = (int)roundf(dict->entries[i].value.val_float);
			}
			else
			{
				return ERR_WRONG_TYPE;
			}
			
			return NO_ERROR;
		}
	}
	
	return ERR_NOT_FOUND;
}

int m_dictionary_lookup_str(m_dictionary *dict, const char *name, const char **result)
{
	return m_dictionary_lookup(dict, name, result, DICT_ENTRY_TYPE_STR);
}

int m_dictionary_lookup_float(m_dictionary *dict, const char *name, float *result)
{
	return m_dictionary_lookup(dict, name, result, DICT_ENTRY_TYPE_FLOAT);
}

int m_dictionary_lookup_int(m_dictionary *dict, const char *name, int *result)
{
	return m_dictionary_lookup(dict, name, result, DICT_ENTRY_TYPE_INT);
}

int m_dictionary_lookup_dq(m_dictionary *dict, const char *name, m_derived_quantity **result)
{
	return m_dictionary_lookup(dict, name, result, DICT_ENTRY_TYPE_DQ);
}

int m_dictionary_lookup_dict(m_dictionary *dict, const char *name, m_dictionary **result)
{
	return m_dictionary_lookup(dict, name, result, DICT_ENTRY_TYPE_SUBDICT);
}


int m_dict_section_lookup_str(m_ast_node *section, const char *name, const char **result)
{
	if (!section) return ERR_NULL_PTR;
	if (section->type != M_AST_NODE_SECTION) return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	if (!sec || !sec->dict) return ERR_BAD_ARGS;
	
	return m_dictionary_lookup_str(sec->dict, name, result);
}

int m_dict_section_lookup_float(m_ast_node *section, const char *name, float *result)
{
	if (!section) return ERR_NULL_PTR;
	if (section->type != M_AST_NODE_SECTION) return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	if (!sec || !sec->dict) return ERR_BAD_ARGS;
	
	return m_dictionary_lookup_float(sec->dict, name, result);
}

int m_dict_section_lookup_int(m_ast_node *section, const char *name, int *result)
{
	if (!section) return ERR_NULL_PTR;
	if (section->type != M_AST_NODE_SECTION) return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	if (!sec || !sec->dict) return ERR_BAD_ARGS;
	
	return m_dictionary_lookup_int(sec->dict, name, result);
}

int m_dict_section_lookup_dq(m_ast_node *section, const char *name, m_derived_quantity **result)
{
	if (!section) return ERR_NULL_PTR;
	if (section->type != M_AST_NODE_SECTION) return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	if (!sec || !sec->dict) return ERR_BAD_ARGS;
	
	return m_dictionary_lookup_dq(sec->dict, name, result);
}

int m_dict_section_lookup_dict(m_ast_node *section, const char *name, m_dictionary **result)
{
	if (!section) return ERR_NULL_PTR;
	if (section->type != M_AST_NODE_SECTION) return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	if (!sec || !sec->dict) return ERR_BAD_ARGS;
	
	return m_dictionary_lookup_dict(sec->dict, name, result);
}

int m_parse_dictionary_section(m_eff_parsing_state *ps, m_ast_node *section)
{
	if (!ps || !section)
		return ERR_NULL_PTR;
	
	if (section->type != M_AST_NODE_SECTION)
		return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	printf("Parsing dictionary section \"%s\"\n", sec->name);
	
	m_token_ll *tokens = sec->tokens;
	
	if (!tokens)
		return ERR_BAD_ARGS;
	
	ps->current_token = tokens->next;
	
	return m_parse_dictionary(ps, &sec->dict, sec->name);
}

int token_is_entry_delimiter(char *token)
{
	if (!token)
		return 0;
	
	if (strcmp(token, ",") == 0)
		return 1;
	
	if (strcmp(token, "\n") == 0)
		return 1;
	
	if (strcmp(token, ";") == 0)
		return 1;
	
	return 0;
}

int m_parse_dict_val(m_eff_parsing_state *ps, m_dict_entry *result)
{
	if (!ps || !result)
		return ERR_NULL_PTR;
	
	printf("Parsing dictionary entry value\n");
	
	m_token_ll **next_token = &ps->current_token;
	m_token_ll *current = ps->current_token;
	
	if (!current)
		return ERR_BAD_ARGS;
	
	if (!current->data)
		return ERR_BAD_ARGS;
	
	m_token_ll *end = current;
	
	int ret_val = NO_ERROR;
	int len;
	
	int paren_cnt = 0;
	int n_tokens = 0;
	
	while (end)
	{
		if (strcmp(end->data, "(") == 0)
		{
			paren_cnt++;
		}
		else if (strcmp(end->data, ")") == 0)
		{
			if (paren_cnt > 0)
				paren_cnt--;
			else
				break;
		}
		
		if (paren_cnt == 0 && token_is_entry_delimiter(end->data))
			break;
		
		end = end->next;
		n_tokens++;
	}
	if (end)
	
	if (current->data[0] == '"')
	{
		result->type = DICT_ENTRY_TYPE_STR;
		
		len = strlen(current->data) - 2;
		if (n_tokens > 1)
		{
			printf("Syntax error (line %d): excess tokens following string %s\n", current->line, current->data);
			ret_val = ERR_BAD_ARGS;
			goto parse_dict_val_fin;
		}
		
		result->value.val_string = m_strndup(&current->data[1], len);
		
		goto parse_dict_val_fin;
	}
	
	if (strcmp(current->data, "(") == 0)
	{
		result->type = DICT_ENTRY_TYPE_SUBDICT;
		ps->current_token = current->next;
		ret_val = m_parse_dictionary(ps, &result->value.val_dict, result->name);
		
		goto parse_dict_val_fin;
	}
	
	if (n_tokens == 1 && token_is_int(current->data))
	{
		result->type = DICT_ENTRY_TYPE_INT;
		result->value.val_int = strtol(current->data, NULL, 10);
		
		goto parse_dict_val_fin;
	}
	
	if (n_tokens == 1 && token_is_number(current->data))
	{
		result->type = DICT_ENTRY_TYPE_FLOAT;
		result->value.val_float = token_to_float(current->data);
		
		goto parse_dict_val_fin;
	}
	
	if (n_tokens == 2 && strcmp(current->data, "-") == 0 && token_is_int(current->next->data))
	{
		result->type = DICT_ENTRY_TYPE_INT;
		result->value.val_int = -strtol(current->next->data, NULL, 10);
		
		goto parse_dict_val_fin;
	}
	
	if (n_tokens == 2 && strcmp(current->data, "-") == 0 && token_is_number(current->next->data))
	{
		printf("neg number -%s\n", current->next->data);
		result->type = DICT_ENTRY_TYPE_FLOAT;
		result->value.val_float = -token_to_float(current->next->data);
		
		printf("result->value.val_float = %f\n", result->value.val_float);
		
		goto parse_dict_val_fin;
	}
	
	result->type = DICT_ENTRY_TYPE_DQ;
	result->value.val_dq = new_m_derived_quantity_from_tokens(current, end);
	
parse_dict_val_fin:
	
	if (end)
		end = end->next;
	
	if (next_token)
		*next_token = end;
	
	printf("parse_dict_val returning %d\n", ret_val);
	return ret_val;
}

int m_parse_dictionary(m_eff_parsing_state *ps, m_dictionary **result, const char *name)
{
	if (!ps || !result)
		return ERR_NULL_PTR;
	
	printf("Parsing dictionary \"%s\"\n", name);
	
	m_token_ll **next_token = &ps->current_token;
	m_token_ll *current = ps->current_token;
	m_token_ll *nt = NULL;
	
	if (!current)
		return ERR_BAD_ARGS;
	
	m_dict_entry centry;
	
	int ret_val = NO_ERROR;

	m_dictionary *dict = NULL;
	
	dict = m_new_dictionary();
	
	if (!dict)
	{
		ret_val = ERR_ALLOC_FAIL;
		goto parse_dict_fin;
	}
	
	char *cname;
	dict->name = m_strndup(name, 128);
	
	m_token_ll_skip_ws(&current);
	
	while (current)
	{
		if (!token_is_name(current->data))
		{
			printf("Error (line %d): expected name, got \"%s\"\n", current->line, current->data);
			ret_val = ERR_BAD_ARGS;
			goto parse_dict_fin;
		}
		
		printf("Found entry, name \"%s\"\n", current->data);
		
		cname = m_strndup(current->data, 64);
		
		if (!cname)
		{
			ret_val = ERR_ALLOC_FAIL;
			goto parse_dict_fin;
		}
		
		centry.name = cname;
		
		m_token_ll_advance(&current);
		
		if (!current || (strcmp(current->data, ":") != 0 && strcmp(current->data, "=") != 0))
		{
			printf("Syntax error: expected \":\" or \"=\", got \"%s\"\n", current->data);
			m_free(cname);
			ret_val = ERR_BAD_ARGS;
			goto parse_dict_fin;
		}
		m_token_ll_advance(&current);
		
		ps->current_token = current;
		if ((ret_val = m_parse_dict_val(ps, &centry)) != NO_ERROR)
		{
			printf("Error parsing %s.%s\n", dict->name, cname);
			goto parse_dict_fin;
		}
		else
		{
			m_dictionary_add_entry(dict, centry);
		}
		current = ps->current_token;
		
		m_token_ll_skip_ws(&current);
		if (!current || strcmp(current->data, ")") == 0)
			break;
	}
	
	*result = dict;

parse_dict_fin:
	if (current)
		current = current->next;
	
	if (next_token)
		*next_token = current;
	
	printf("parse_dict returning %d\n", ret_val);
	return ret_val;
}

void print_dict_entry(m_dict_entry *entry)
{
	if (!entry)
	{
		printf("(null)");
		return;
	}
	printf("%s: ", entry->name);
	
	switch (entry->type)
	{
		case DICT_ENTRY_TYPE_STR:
			printf("\"%s\"", entry->value.val_string);
			break;
		
		case DICT_ENTRY_TYPE_INT:
			printf("%d", entry->value.val_int);
			break;
		
		case DICT_ENTRY_TYPE_FLOAT:
			printf("%f", entry->value.val_float);
			break;
		
		case DICT_ENTRY_TYPE_DQ:
			printf("(expression)");
			break;
			
		default:
			printf("mangled !");
			break;
	}
}

void print_dict(m_dictionary *dict)
{
	printf("Dictionary ");
	if (!dict)
	{
		printf("(null)\n");
		return;
	}
	
	printf("\"%s\" (%d entries):\n", dict->name, dict->n_entries);
	
	for (int i = 0; i < dict->n_entries; i++)
	{
		printf("\t");
		print_dict_entry(&dict->entries[i]);
		printf("\n");
	}
}

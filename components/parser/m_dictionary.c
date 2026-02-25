#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "m_int.h"

m_dictionary *m_new_dictionary()
{
	m_dictionary *dict = m_parser_alloc(sizeof(m_dictionary));
	
	if (!dict)
		return NULL;
	
	dict->entries = m_parser_alloc(sizeof(m_dictionary_entry) * 8);
	
	if (!dict->entries)
	{
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
	
	m_dictionary_entry *na;
	
	if (dict->n_entries == dict->entry_array_length)
	{
		na = malloc(sizeof(m_dictionary_entry) * dict->entry_array_length * 2);
		
		if (!na) return ERR_ALLOC_FAIL;
		
		dict->entries = na;
		dict->entry_array_length *= 2;
	}
	
	return NO_ERROR;
}

int m_dictionary_add_entry(m_dictionary *dict, m_dictionary_entry entry)
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

int m_dictionary_add_entry_expr(m_dictionary *dict, const char *name, m_expression *value)
{
	if (!dict)
		return ERR_NULL_PTR;
	
	if (!name || !value)
		return ERR_BAD_ARGS;
	
	int ret_val = NO_ERROR;
	
	if ((ret_val = m_dictionary_ensure_capacity(dict)) != NO_ERROR)
		return ret_val;
	
	dict->entries[dict->n_entries].name  = name;
	dict->entries[dict->n_entries].type = DICT_ENTRY_TYPE_EXPR;
	dict->entries[dict->n_entries].value.val_expr = value;

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
					case DICT_ENTRY_TYPE_EXPR:
						*((m_expression**)result) = dict->entries[i].value.val_expr;
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

int m_dictionary_lookup_expr(m_dictionary *dict, const char *name, m_expression **result)
{
	return m_dictionary_lookup(dict, name, result, DICT_ENTRY_TYPE_EXPR);
}

int m_dictionary_lookup_dict(m_dictionary *dict, const char *name, m_dictionary **result)
{
	return m_dictionary_lookup(dict, name, result, DICT_ENTRY_TYPE_SUBDICT);
}

void print_dict_entry(m_dictionary_entry *entry)
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
		
		case DICT_ENTRY_TYPE_EXPR:
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

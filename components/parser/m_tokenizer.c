#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "m_int.h"

IMPLEMENT_LINKED_PTR_LIST(char);

int char_is_letter(char c)
{
	return (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'));
}

int char_is_number(char c)
{
	return ('0' <= c && c <= '9');
}

int char_is_alphanumeric(char c)
{
	return (char_is_letter(c) || char_is_number(c));
}

int char_is_bracket(char c)
{
	return (c == '(' || c == '[' || c == '{' 
	     || c == ')' || c == ']' || c == '}'); 
}

int char_is_in_string(char c, const char *str)
{
	if (!str)
		return 0;
	
	int i = 0;
	
	while (str[i] != c)
	{
		if (str[i++] == 0)
			return 0;
	}
	
	return 1;
}

int token_is_valid_section_name(char *str)
{
	if (!str)
		return 0;
	
	if (strcmp(str, "INFO") 	  == 0) return 1;
	if (strcmp(str, "RESOURCES")  == 0) return 1;
	if (strcmp(str, "PARAMETERS") == 0) return 1;
	if (strcmp(str, "OPTIONS") 	  == 0) return 1;
	if (strcmp(str, "CODE") 	  == 0) return 1;
	
	return 0;
}

int token_is_char(char *str, char c)
{
	return (str && str[0] == c && str[1] == 0);
}

int token_is_newline(char *str)
{
	return token_is_char(str, '\n');
}

int token_is_int(char *token)
{
	if (!token)
		return 0;
	
	int pos = 0;
	
	while (token[pos])
	{
		if (!('0' <= token[pos] && token[pos] <= '9'))
			return 0;
		pos++;
	}
	
	return 1;
}

int token_is_number(char *token)
{
	if (!token)
		return 0;
	
	int len = strlen(token);
	
	char allowed_chars[27] = ".0123456789\0\0\0abcdefABCDEF\0";
	
	if (!char_is_in_string(token[0], allowed_chars))
		return 0;
	
	if (token[0] == '0')
	{
		allowed_chars[11] = 'b';
		allowed_chars[12] = 'x';
	}
	else if (token[0] == '.')
	{
		allowed_chars[0] = '0';
		
		if (len == 1)
			return 0;
	}
	
	for (int i = 1; i < len; i++)
	{
		if (!char_is_in_string(token[i], allowed_chars))
			return 0;
		
		if (token[i] == '.')
			allowed_chars[0] = '0';
		
		if (i == 1)
		{
			if (token[i] == 'x' || token[i] == 'b')
			{				
				allowed_chars[11] = '0';
				allowed_chars[12] = '0';
				
				if (len < 3)
					return 0;
				
				if (token[i] == 'b')
					allowed_chars[3] = 0;
				else
					allowed_chars[13] = '0';
			}
		}
	}
	
	return 1;
}

int token_is_dict_entry_seperator(char *token)
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

float digit_to_float(char c)
{
	if ('0' <= c && c <= '9')
		return (float)(c - '0');
	else if ('a' <= c && c <= 'f')
		return (float)(c - 'a');
	else if ('A' <= c && c <= 'F')
		return (float)(c - 'A');
	
	return 0.0;
}

float token_to_float(char *token)
{
	if (!token)
		return 0;
	
	int len = strlen(token);

	float res = 0.0;
	
	float base = 10.0f;
	int exp = 0;
	int frac = 0;
	int pos = 0;
	
	if (len > 2 && token[0] == '0')
	{
		if (token[1] == 'b')
		{
			base = 2.0f;
			pos = 2;
		}
		else if (token[1] == 'x')
		{
			base = 16.0f;
			pos = 2;
		}
	}
	
	while (pos < len)
	{
		if (token[pos] == '.')
		{
			frac = 1;
		}
		else
		{
			res = res * base + digit_to_float(token[pos]);
			exp += frac;
		}
		
		pos++;
	}
	
	while (exp > 0)
	{
		res /= (float)base;
		exp--;
	}
	
	return res;
}

int token_is_name(char *token)
{
	if (!token)
		return 0;
	
	int len = strlen(token);
	
	if (len == 0)
		return 0;
	
	for (int i = 0; i < len; i++)
	{
		if (char_is_letter(token[i]) || token[i] == '_')
			continue;
		
		if (i > 0 && char_is_number(token[i]))
			continue;
		
		return 0;
	}
	
	return 1;
}

int m_token_ll_advance(m_token_ll **list)
{
	if (!list)
		return ERR_NULL_PTR;
	
	if (*list)
		*list = (*list)->next;
	else
		return NO_ERROR;
	
	m_token_ll *current = *list;
	
	if (!current)
		return NO_ERROR;
	
	int cont;
	while (current && current->data)
	{
		cont = 0;
		
		if (current->data[0] == '\n' || current->data[0] == ' ' || current->data[0] == '\t')
			cont = 1;
			
		if (!cont)
			break;
		
		current = current->next;
	}
	
	*list = current;
	return NO_ERROR;
}

int m_token_ll_skip_ws(m_token_ll **list)
{
	if (!list)
		return ERR_NULL_PTR;
	
	m_token_ll *current = *list;
	
	if (!current)
		return NO_ERROR;
	
	int cont;
	while (current && current->data)
	{
		cont = 0;
		
		if (current->data[0] == '\n' || current->data[0] == ' ' || current->data[0] == '\t')
			cont = 1;
			
		if (!cont)
			break;
		
		current = current->next;
	}
	
	*list = current;
	return NO_ERROR;
}


int m_token_ll_safe_append(m_token_ll **list_ptr, char *x, int line, int index)
{
	if (!list_ptr)
		return ERR_NULL_PTR;
	
	m_token_ll *node = m_alloc(sizeof(m_token_ll));
	
	if (!node)
		return ERR_ALLOC_FAIL;
	
	node->data = x;
	node->line = line;
	node->index = index;
	node->next = NULL;
	
	if (*list_ptr)
	{
		m_token_ll *current = *list_ptr;
		
		while (current->next)
			current = current->next;
		
		current->next = node;
	}
	else
	{
		*list_ptr = node;
	}
	
	return NO_ERROR;
}

int m_token_ll_safe_aappend(m_token_ll **list_ptr, char *x, int line, int index)
{
	if (!list_ptr)
		return ERR_NULL_PTR;
	
	m_token_ll *node = m_parser_alloc(sizeof(m_token_ll));
	
	if (!node)
		return ERR_ALLOC_FAIL;
	
	node->data = x;
	node->line = line;
	node->index = index;
	node->next = NULL;
	
	if (*list_ptr)
	{
		m_token_ll *current = *list_ptr;
		
		while (current->next)
			current = current->next;
		
		current->next = node;
	}
	else
	{
		*list_ptr = node;
	}
	
	return NO_ERROR;
}

int tokenizer_policy(char c, int *state_ptr)
{
	if (!state_ptr)
		return -1;
	
	int state = *state_ptr;
	
	if (c == 0 || c == EOF)
	{
		*state_ptr = TOKENIZER_STATE_DONE;
		return TOKENIZER_POLICY_DISCARD;
	}
	
	if (state == TOKENIZER_STATE_STRING)
	{
		if (c == '"')
		{
			*state_ptr = TOKENIZER_STATE_IDLE;
			return TOKENIZER_POLICY_END_ACCEPT;
		}
		
		if (c == '\\')
		{
			*state_ptr = TOKENIZER_STATE_STRESC;
			return TOKENIZER_POLICY_ACCEPT;
		}
		
		if (c == '\n')
			return TOKENIZER_POLICY_COMPLAIN;
		
		return TOKENIZER_POLICY_ACCEPT;
	}
	
	if (state == TOKENIZER_STATE_STRESC)
	{
		*state_ptr = TOKENIZER_STATE_STRING;
		return TOKENIZER_POLICY_ACCEPT;
	}
	
	if (state != TOKENIZER_STATE_IDLE && (c == ' ' || c == '\t'))
	{
		*state_ptr = TOKENIZER_STATE_IDLE;
		return TOKENIZER_POLICY_END_DISCARD;
	}
	
	if (c == '\n' || char_is_bracket(c) || c == ':' || c == ',' ||
		(c == '.' && (state != TOKENIZER_STATE_LEADING_ZERO && state != TOKENIZER_STATE_NUMBER && state != TOKENIZER_STATE_NUMBER_BIN && state != TOKENIZER_STATE_NUMBER_HEX)))
	{
		*state_ptr = TOKENIZER_STATE_IDLE;
		return TOKENIZER_POLICY_SINGULAR;
	}
	
	switch (state)
	{
		case TOKENIZER_STATE_IDLE:
			if (c == ' ' || c == '\t') return TOKENIZER_POLICY_DISCARD;
			
			if (char_is_letter(c))
			{
				*state_ptr = TOKENIZER_STATE_NAME;
				return TOKENIZER_POLICY_BEGIN;
			}
			
			if (char_is_number(c))
			{
				if (c == '0')
					*state_ptr = TOKENIZER_STATE_LEADING_ZERO;
				else
					*state_ptr = TOKENIZER_STATE_NUMBER;
				
				return TOKENIZER_POLICY_BEGIN;
			}
			
			if (c == '"')
			{
				*state_ptr = TOKENIZER_STATE_STRING;
				return TOKENIZER_POLICY_BEGIN;
			}
			
			return TOKENIZER_POLICY_SINGULAR;
		
		case TOKENIZER_STATE_NAME:
			if (c == '_' || char_is_alphanumeric(c))
				return TOKENIZER_POLICY_ACCEPT;
			
			if (char_is_in_string(c, " \t"))
			{
				*state_ptr = TOKENIZER_STATE_IDLE;
				return TOKENIZER_POLICY_END_DISCARD;
			}
			
			return TOKENIZER_POLICY_SINGULAR;
			
		case TOKENIZER_STATE_LEADING_ZERO:
			if (c == 'b')
			{
				*state_ptr = TOKENIZER_STATE_NUMBER_BIN;
				return TOKENIZER_POLICY_ACCEPT;
			}
			
			if (c == 'x')
			{
				*state_ptr = TOKENIZER_STATE_NUMBER_HEX;
				return TOKENIZER_POLICY_ACCEPT;
			}
			
			if (c == '.')
			{
				*state_ptr = TOKENIZER_STATE_NUMBER;
				return TOKENIZER_POLICY_ACCEPT;
			}
			
			if (!char_is_in_string(c, "0123456789"))
			{
				*state_ptr = TOKENIZER_STATE_IDLE;
				return TOKENIZER_POLICY_SINGULAR;
			}
			
			return TOKENIZER_POLICY_ACCEPT;
		
		case TOKENIZER_STATE_NUMBER_HEX:
			if (char_is_in_string(c, "abcdefABCDEF"))
				return TOKENIZER_POLICY_ACCEPT;
			
		case TOKENIZER_STATE_NUMBER:
			if (char_is_in_string(c, ".123456789"))
				return TOKENIZER_POLICY_ACCEPT;
			
		case TOKENIZER_STATE_NUMBER_BIN:
			if (c == '0' || c == '1' || c == '.')
				return TOKENIZER_POLICY_ACCEPT;
			
			*state_ptr = TOKENIZER_STATE_IDLE;
			return TOKENIZER_POLICY_SINGULAR;
	}
	
	return TOKENIZER_POLICY_SINGULAR;
}

int m_tokenize_eff_file(m_eff_parsing_state *ps, FILE *file, m_token_ll **tokens)
{
	if (!file || !tokens || !ps)
		return ERR_NULL_PTR;
	
	char buf[256];
		
	int line = 1;
	int line_char = 4;
	int token_index = 0;
	int new_line = 0;
	int buf_pos = 0;
	char c;
	int C;
	int policy;
	
	int state = TOKENIZER_STATE_IDLE;
	
	buf[0] = fgetc(file);
	buf[1] = fgetc(file);
	buf[2] = fgetc(file);
	buf[3] = fgetc(file);
	buf[4] = 0;
	
	if (strcmp(ver_str, buf) != 0)
	{
		m_parser_error_at_line(ps, 1, "Version string \"%s\" required at start of file");
		return ERR_BAD_ARGS;
	}

	
	m_token_ll_safe_aappend(tokens, m_parser_strndup(ver_str, 4), line, 0);
	
	while (state != TOKENIZER_STATE_DONE)
	{
		C = fgetc(file);
		c = (char)C;
		
		if (C == EOF)
		{
			state = TOKENIZER_STATE_DONE;
			policy = TOKENIZER_POLICY_DISCARD;
		}
		else
		{
			policy = tokenizer_policy(c, &state);
		}
		
		switch (policy)
		{
			case TOKENIZER_POLICY_DISCARD:
				break;
			case TOKENIZER_POLICY_ACCEPT:
				buf[buf_pos++] = c;
				break;
				
			case TOKENIZER_POLICY_SINGULAR:
				if (buf_pos)
				{
					buf[buf_pos++] = 0;
					m_token_ll_safe_aappend(tokens, m_parser_strndup(buf, buf_pos), line, token_index);
					token_index += buf_pos;
					buf_pos = 0;
				}
				buf[0] = c;
				buf[1] = 0;
				m_token_ll_safe_aappend(tokens, m_parser_strndup(buf, 1), line, token_index);
				token_index += 1;
				break;
			case TOKENIZER_POLICY_BEGIN:
				buf_pos = 0;
				buf[buf_pos++] = c;
				break;
			case TOKENIZER_POLICY_END_ACCEPT:
				buf[buf_pos++] = c;
			case TOKENIZER_POLICY_END_DISCARD:
				buf[buf_pos++] = 0;
				m_token_ll_safe_aappend(tokens, m_parser_strndup(buf, buf_pos), line, token_index);
				token_index += buf_pos;
				buf_pos = 0;
				break;
			case TOKENIZER_POLICY_COMPLAIN:
				if (c == '\n')
				{
					buf[0] = '\\';
					buf[1] = 'n';
					buf[2] = 0;
				}
				else
				{
					buf[0] = c;
					buf[1] = 0;
				}
				m_parser_error_at_line(ps, line, "Unexpected \"%s\"", buf);
				return ERR_BAD_ARGS;
		}
		
		if (c == '\n')
		{
			line = line + 1;
			line_char = 0;
			token_index = 0;
		}
		line_char++;
	}
	
	if (buf_pos)
	{
		buf[buf_pos++] = 0;
		m_token_ll_safe_aappend(tokens, m_parser_strndup(buf, buf_pos), line, token_index);
	}
	
	return NO_ERROR;
}

m_token_ll *m_token_span_to_ll(m_token_ll *start, m_token_ll *end)
{
	if (!start)
		return NULL;
	
	m_token_ll *res = NULL;
	m_token_ll *current = start;
	
	while (current && current != end)
	{
		m_token_ll_safe_aappend(&res, current->data, current->line, current->index);
		current = current->next;
	}
	
	return res;
}

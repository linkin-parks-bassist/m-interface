#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "m_int.h"

const char *ver_str = "v1.0";

m_bump_arena m_eff_parser_mempool;
int m_parser_mempool_initialised = 0;

int m_eff_parser_init_mempool()
{
	if (m_parser_mempool_initialised)
		return NO_ERROR;
	
	int ret_val = m_bump_arena_init(&m_eff_parser_mempool, M_EFF_PARSER_MEM_POOL_SIZE_KB * 1024);
	m_parser_mempool_initialised = (ret_val == NO_ERROR);
	return ret_val;
}

int m_eff_parser_deinit_mempool()
{
	m_parser_mempool_initialised = 0;
	return m_bump_arena_destroy(&m_eff_parser_mempool);
}

void *m_parser_alloc(size_t size)
{
	return m_bump_arena_alloc(&m_eff_parser_mempool, size);
}

char *m_parser_strndup(const char *str, int n)
{
	size_t len = strnlen(str, n);
    
    char *new_str = m_parser_alloc(len + 1);
    
    if (!new_str) return NULL;
    memcpy(new_str, str, len);
    new_str[len] = '\0';
    
    return new_str;
}

int m_parse_tokens(m_eff_parsing_state *ps)
{
	if (!ps)
		return ERR_NULL_PTR;
	
	m_token_ll *tokens = ps->tokens;
	m_ast_node **root_ptr = &ps->ast;
	
	m_ast_node *root = m_parser_alloc(sizeof(m_ast_node));
	
	if (!root)
		return ERR_ALLOC_FAIL;
	
	*root_ptr = root;
	
	root->type  = M_AST_NODE_ROOT;
	root->data  = NULL;
	root->child = NULL;
	root->next  = NULL;
	
	// Let's do a first pass to find the sections
	
	m_ast_node *current_section = NULL;
	m_ast_node *next_section	= NULL;
	m_eff_desc_file_section *cs	= NULL;
	m_eff_desc_file_section *ns	= NULL;
	m_token_ll *current_token 	= tokens;
	m_token_ll *prev_tokens[4]  = {NULL, NULL, NULL, NULL};
	
	m_token_ll *current_sec_start = NULL;
	
	int section_start_score = 0;
	int next_section_start_score;
	
	int token_n = 0;
	int line = 1;
	
	while (current_token)
	{
		token_n++;
		
		if (current_token->data)
		{
			if (token_is_newline(current_token->data))
			{
				line++;
				token_n = 1;
			}
			
			next_section_start_score = get_section_start_score(current_token->data, section_start_score);
			
			if (section_start_score == 2)
			{
				if (next_section_start_score == 3)
				{
					next_section = m_parser_alloc(sizeof(m_ast_node));
					
					if (!next_section)
						return ERR_ALLOC_FAIL;
					
					next_section->type  = M_AST_NODE_SECTION;
					next_section->data  = NULL;
					next_section->next  = NULL;
					next_section->child = NULL;
					
					ns = m_parser_alloc(sizeof(m_eff_desc_file_section));
					
					if (!ns)
					{
						return ERR_ALLOC_FAIL;
					}
					
					ns->name = m_parser_strndup(current_token->data, 16);
					
					if (!ns->name)
					{
						return ERR_ALLOC_FAIL;
					}
					
					ns->dict = NULL;
					
					next_section->data = (void*)ns;
					
					if (current_section && cs)
					{
						cs->tokens = m_token_span_to_ll(current_sec_start->next, prev_tokens[3]);
						current_section->next = next_section;
					}
					else
					{
						root->child = next_section;
					}
					
					current_section = next_section;
					current_sec_start = current_token;
					cs = ns;
				}
				else
				{
					m_parser_error_at(ps, current_token, "Invalid section name \"%s\"", current_token->data);
					return ERR_BAD_ARGS;
				}
				
				section_start_score = 0;
			}
			else
			{
				section_start_score = next_section_start_score;
			}
		}
		
		prev_tokens[0] = current_token;
		prev_tokens[1] = prev_tokens[0];
		prev_tokens[2] = prev_tokens[1];
		prev_tokens[3] = prev_tokens[2];
		
		current_token = current_token->next;
	}
	
	if (current_section)
	{
		cs->tokens = m_token_span_to_ll(current_sec_start->next, NULL);
	}
	
	// Now let's actually parse those sections
	
	current_section = root->child;
	
	// We need to do the code section last
	m_ast_node *info_section = NULL;
	m_ast_node *code_section = NULL;
	m_ast_node *resources_section = NULL;
	m_ast_node *parameters_section = NULL;
	m_ast_node *settings_section = NULL;
	
	m_eff_desc_file_section *sect;
	
	int ret_val;
	while (current_section)
	{
		sect = (m_eff_desc_file_section*)current_section->data;
		
		if (!sect)
		{
			current_section = current_section->next;
			continue;
		}
		
		if (strcmp(sect->name, "INFO") == 0)
		{
			if ((ret_val = m_parse_dictionary_section(ps, current_section)) != NO_ERROR)
			{
				return ret_val;
			}
			info_section = current_section;
		}
		else if (strcmp(sect->name, "RESOURCES") == 0)
		{
			if ((ret_val = m_parse_dictionary_section(ps, current_section)) != NO_ERROR)
			{
				return ret_val;
			}
			resources_section = current_section;
		}
		else if (strcmp(sect->name, "PARAMETERS") == 0)
		{
			if ((ret_val = m_parse_dictionary_section(ps, current_section)) != NO_ERROR)
			{
				return ret_val;
			}
			parameters_section = current_section;
		}
		else if (strcmp(sect->name, "SETTINGS") == 0)
		{
			if ((ret_val = m_parse_dictionary_section(ps, current_section)) != NO_ERROR)
			{
				return ret_val;
			}
			settings_section = current_section;
		}
		else if (strcmp(sect->name, "CODE") == 0)
		{
			code_section = current_section;
			ret_val = NO_ERROR;
		}
		else
		{
			m_parser_error(ps, "Invalid section name \"%s\"", sect->name);
			return ERR_BAD_ARGS;
		}
	
		if (ret_val != NO_ERROR)
			return ret_val;
		current_section = current_section->next;
	}
	
	if (info_section)
	{
		if ((ret_val = m_dictionary_section_lookup_str(info_section, "name", &ps->name)) != NO_ERROR)
		{
			m_parser_error(ps, "Effect name missing");
			return ret_val;
		}
		else
		{
			printf("Found name: \"%s\"\n", ps->name ? ps->name : "(NULL)");
		}
	}
	else
	{
		m_parser_error(ps, "INFO section missing");
		return ERR_BAD_ARGS;
	}
	
	ps->scope = m_parser_alloc(sizeof(m_expr_scope));
	m_expr_scope_init(ps->scope);
	
	if (!ps->scope)
		return ERR_ALLOC_FAIL;
	
	if (resources_section)
	{
		if ((ret_val = m_resources_section_extract(ps, &ps->resources, resources_section)) != NO_ERROR)
		{
			return ret_val;
		}
		
		m_resources_assign_handles(ps->resources);
	}
	
	if (parameters_section)
	{
		if ((ret_val = m_parameters_section_extract(ps, &ps->parameters, parameters_section)) != NO_ERROR)
		{
			return ret_val;
		}
		
		m_parameters_assign_ids(ps->parameters);
		m_expr_scope_add_params(ps->scope, ps->parameters);
	}
	
	if (settings_section)
	{
		if ((ret_val = m_settings_section_extract(ps, &ps->settings, settings_section)) != NO_ERROR)
		{
			return ret_val;
		}
		
		m_settings_assign_ids(ps->settings);
		printf("Adding settings to scope...\n");
		m_expr_scope_add_settings(ps->scope, ps->settings);
	}
	
	if (code_section)
	{
		if ((ret_val = m_parse_code_section(ps, code_section)) != NO_ERROR)
		{
			return ret_val;
		}
		
		m_compute_register_formats(ps->blocks, ps->scope);
	}
	
	return NO_ERROR;
}

int init_parsing_state(m_eff_parsing_state *ps)
{
	if (!ps) return ERR_NULL_PTR;
	
	ps->tokens = NULL;
	ps->current_token = NULL;
	
	ps->parameters = NULL;
	ps->resources = NULL;
	ps->settings = NULL;
	ps->blocks = NULL;
	
	ps->errors = 0;
	ps->scope = NULL;
	
	return NO_ERROR;
}

m_effect_desc *m_read_eff_desc_from_file(char *fname)
{
	printf("m_read_eff_desc_from_file(fname = \"%s\")\n", fname);
	m_effect_desc *result = NULL;
	m_eff_parsing_state ps;
	
	FILE *src = fopen(fname, "r");
	
	if (!src)
	{
		printf("Failed to open file \"%s\"!\n", fname);
		return NULL;
	}
	
	int ret_val;
	
	if (!m_parser_mempool_initialised)
	{
		printf("Initialise mempool...\n");
		if ((ret_val = m_eff_parser_init_mempool()) != NO_ERROR)
		{
			printf("Error initialising parser mempool: %s\n", m_error_code_to_string(ret_val));
			return NULL;
		}
		printf("Parser mempool initialised.\n");
	}
	
	printf("Initialise parser...\n");
	init_parsing_state(&ps);
	printf("Parser initialised.\n");
	
	ps.fname = m_parser_strndup(fname, 128);
	
	
	m_token_ll *tokens = NULL;
	
	printf("Tokenize file...\n");
	m_tokenize_eff_file(&ps, src, &ps.tokens);
	printf("File tokenized.\n");
	int j = 0;
	
	fclose(src);
	src = NULL;
	
	if (ps.errors != 0)
	{
		printf("File \"%s\" ignored due to errors.\n", fname);
		return NULL;
	}
	
	m_block_pll *blocks = NULL;
	m_dsp_resource_pll *res = NULL;
	
	ret_val = m_parse_tokens(&ps);
	
	if (ps.errors != 0)
	{
		printf("File \"%s\" ignored due to errors.\n", fname);
		if (ps.parameters)
		{
			free_m_parameter_pll(ps.parameters);
		}
		if (ps.resources)
		{
			free_m_dsp_resource_pll(ps.resources);
		}
		if (ps.blocks)
		{
			free_m_block_pll(ps.blocks);
		}
		return NULL;
	}
	
	if (ret_val == NO_ERROR)
	{
		printf("File \"%s\" parsed sucessfully\n", fname);
	}
	else
	{
		printf("File \"%s\" parsing failed. Error code: %d\n", fname, ret_val);
	}
	
	result = m_alloc(sizeof(m_effect_desc));
	
	m_init_effect_desc(result);
	
	if (result)
	{
		result->parameters = ps.parameters;
		result->resources = ps.resources;
		result->settings = ps.settings;
		result->blocks = ps.blocks;
		
		result->name = m_strndup(ps.name, 128);
		
		result->scope = m_eff_desc_create_scope(result);
		
		m_effect_desc_generate_res_rpt(result);
	}
	
	return result;
}

int m_parse_dictionary_section(m_eff_parsing_state *ps, m_ast_node *section)
{
	if (!ps || !section)
		return ERR_NULL_PTR;
	
	if (section->type != M_AST_NODE_SECTION)
		return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	m_token_ll *tokens = sec->tokens;
	
	if (!tokens)
		return ERR_BAD_ARGS;
	
	ps->current_token = tokens->next;
	
	return m_parse_dictionary(ps, &sec->dict, sec->name);
}

int m_parse_code_section(m_eff_parsing_state *ps, m_ast_node *section)
{
	if (!ps || !section)
		return ERR_NULL_PTR;
	
	if (section->type != M_AST_NODE_SECTION)
		return ERR_BAD_ARGS;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	m_token_ll *tokens = sec->tokens;
	
	if (!tokens)
		return ERR_BAD_ARGS;
	
	ps->current_token = tokens->next;
	
	m_parse_asm(ps);
	
	return NO_ERROR;
}

int m_parse_dict_val(m_eff_parsing_state *ps, m_dictionary_entry *result)
{
	if (!ps || !result)
		return ERR_NULL_PTR;
	
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
		
		if (paren_cnt == 0 && token_is_dict_entry_seperator(end->data))
			break;
		
		end = end->next;
		n_tokens++;
	}
	
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
		
		result->value.val_string = m_parser_strndup(&current->data[1], len);
		
		goto parse_dict_val_fin;
	}
	else if (strcmp(current->data, "(") == 0)
	{
		result->type = DICT_ENTRY_TYPE_SUBDICT;
		ps->current_token = current->next;
		ret_val = m_parse_dictionary(ps, &result->value.val_dict, result->name);
		
		goto parse_dict_val_fin;
	}
	else
	{
		result->type = DICT_ENTRY_TYPE_EXPR;
		result->value.val_expr = m_parse_expression(ps, current, end);
		
		if (!result->value.val_expr)
		{
			result->type = DICT_ENTRY_TYPE_NOTHING;
			ret_val = ERR_BAD_ARGS;
		}
		
		goto parse_dict_val_fin;
	}
	
parse_dict_val_fin:
	
	if (end)
		end = end->next;
	
	if (next_token)
		*next_token = end;
	
	return ret_val;
}

int m_parse_dictionary(m_eff_parsing_state *ps, m_dictionary **result, const char *name)
{
	if (!ps || !result)
		return ERR_NULL_PTR;
	
	m_token_ll **next_token = &ps->current_token;
	m_token_ll *current = ps->current_token;
	m_token_ll *nt = NULL;
	
	if (!current)
		return ERR_BAD_ARGS;
	
	m_dictionary_entry centry;
	
	int ret_val = NO_ERROR;

	m_dictionary *dict = NULL;
	
	dict = m_new_dictionary();
	
	if (!dict)
	{
		ret_val = ERR_ALLOC_FAIL;
		goto parse_dict_fin;
	}
	
	char *cname;
	dict->name = m_parser_strndup(name, 128);
	
	m_token_ll_skip_ws(&current);
	
	while (current)
	{
		if (!token_is_name(current->data))
		{
			m_parser_error_at(ps, current, "Expected name, got \"%s\"", current->data);
			ret_val = ERR_BAD_ARGS;
			goto parse_dict_fin;
		}
		
		cname = m_parser_strndup(current->data, 64);
		
		if (!cname)
		{
			ret_val = ERR_ALLOC_FAIL;
			goto parse_dict_fin;
		}
		
		centry.name = cname;
		
		m_token_ll_advance(&current);
		
		if (!current || (strcmp(current->data, ":") != 0 && strcmp(current->data, "=") != 0))
		{
			m_parser_error_at(ps, current, "Expected \":\" or \"=\", got \"%s\"\n", current->line, current->data);
			m_free(cname);
			ret_val = ERR_BAD_ARGS;
			goto parse_dict_fin;
		}
		m_token_ll_advance(&current);
		
		ps->current_token = current;
		if ((ret_val = m_parse_dict_val(ps, &centry)) != NO_ERROR)
		{
			m_parser_error(ps, "Error parsing attribute %s.%s", dict->name, cname);
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
	
	return ret_val;
}

void m_parser_print_info_at(m_eff_parsing_state *ps, m_token_ll *token, const char *msg, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	char loc_string[128];
	if (ps->fname && token)
		snprintf(loc_string, 128, " (%s:%d)", ps->fname, token->line);
	else if (ps->fname)
		snprintf(loc_string, 128, " (%s)", ps->fname);
	else
		loc_string[0] = 0;
	
	printf("\e[01;36mINFO%s\e[0m: %s\n", loc_string, buf);
}

void m_parser_warn_at(m_eff_parsing_state *ps, m_token_ll *token, const char *msg, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	char loc_string[128];
	if (ps->fname && token)
		snprintf(loc_string, 128, " (%s:%d)", ps->fname, token->line);
	else if (ps->fname)
		snprintf(loc_string, 128, " (%s)", ps->fname);
	else
		loc_string[0] = 0;
	
	printf("\e[01;32mWARNING%s\e[0m: %s\n", loc_string, buf);
}

void m_parser_error_at(m_eff_parsing_state *ps, m_token_ll *token, const char *msg, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	char loc_string[128];
	if (ps->fname && token)
		snprintf(loc_string, 128, " (%s:%d)", ps->fname, token->line);
	else if (ps->fname)
		snprintf(loc_string, 128, " (%s)", ps->fname);
	else
		loc_string[0] = 0;
	
	ps->errors++;
	printf("\e[01;31mERROR%s\e[0m: %s\n", loc_string, buf);
}

void m_parser_print_info_at_line(m_eff_parsing_state *ps, int line, const char *msg, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	char loc_string[128];
	if (ps->fname)
		snprintf(loc_string, 128, " (%s:%d)", ps->fname, line);
	else if (ps->fname)
		snprintf(loc_string, 128, " (%s)", ps->fname);
	else
		loc_string[0] = 0;
	
	printf("\e[01;36mINFO%s\e[0m: %s\n", loc_string, buf);
}

void m_parser_warn_at_line(m_eff_parsing_state *ps, int line, const char *msg, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	char loc_string[128];
	if (ps->fname)
		snprintf(loc_string, 128, " (%s:%d)", ps->fname, line);
	else if (ps->fname)
		snprintf(loc_string, 128, " (%s)", ps->fname);
	else
		loc_string[0] = 0;
	
	printf("\e[01;32mWARNING%s\e[0m: %s\n", loc_string, buf);
}

void m_parser_error_at_line(m_eff_parsing_state *ps, int line, const char *msg, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	char loc_string[128];
	if (ps->fname)
		snprintf(loc_string, 128, " (%s:%d)", ps->fname, line);
	else if (ps->fname)
		snprintf(loc_string, 128, " (%s)", ps->fname);
	else
		loc_string[0] = 0;
	
	ps->errors++;
	printf("\e[01;31mERROR%s\e[0m: %s\n", loc_string, buf);
}

void m_parser_print_info_at_node(m_eff_parsing_state *ps, m_ast_node *node, const char *msg, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	char loc_string[128];
	if (ps->fname && node)
		snprintf(loc_string, 128, " (%s:%d)", ps->fname, node->line);
	else if (ps->fname)
		snprintf(loc_string, 128, " (%s)", ps->fname);
	else
		loc_string[0] = 0;
	
	printf("\e[01;36mINFO%s\e[0m: %s\n", loc_string, buf);
}

void m_parser_warn_at_node(m_eff_parsing_state *ps, m_ast_node *node, const char *msg, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	char loc_string[128];
	if (ps->fname && node)
		snprintf(loc_string, 128, " (%s:%d)", ps->fname, node->line);
	else if (ps->fname)
		snprintf(loc_string, 128, " (%s)", ps->fname);
	else
		loc_string[0] = 0;
	
	printf("\e[01;32mWARNING%s\e[0m: %s\n", loc_string, buf);
}

void m_parser_error_at_node(m_eff_parsing_state *ps, m_ast_node *node, const char *msg, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	char loc_string[128];
	if (ps->fname && node)
		snprintf(loc_string, 128, " (%s:%d)", ps->fname, node->line);
	else if (ps->fname)
		snprintf(loc_string, 128, " (%s)", ps->fname);
	else
		loc_string[0] = 0;
	
	ps->errors++;
	printf("\e[01;31mERROR%s\e[0m: %s\n", loc_string, buf);
}

void m_parser_print_info(m_eff_parsing_state *ps, const char *msg, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	char loc_string[128];
	if (ps->fname && ps->current_token)
		snprintf(loc_string, 128, " (%s:%d)", ps->fname, ps->current_token->line);
	else if (ps->fname)
		snprintf(loc_string, 128, " (%s)", ps->fname);
	else
		loc_string[0] = 0;
	
	printf("\e[01;36INFO%s\e[0m: %s\n", loc_string, buf);
}

void m_parser_warn(m_eff_parsing_state *ps, const char *msg, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	char loc_string[128];
	if (ps->fname && ps->current_token)
		snprintf(loc_string, 128, " (%s:%d)", ps->fname, ps->current_token->line);
	else if (ps->fname)
		snprintf(loc_string, 128, " (%s)", ps->fname);
	else
		loc_string[0] = 0;
	
	printf("\e[01;32mWARNING%s\e[0m: %s\n", loc_string, buf);
}

void m_parser_error(m_eff_parsing_state *ps, const char *msg, ...)
{
	char buf[1024];
	va_list args;
	va_start(args, msg);
	vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	
	char loc_string[128];
	if (ps->fname && ps->current_token)
		snprintf(loc_string, 128, " (%s:%d)", ps->fname, ps->current_token->line);
	else if (ps->fname)
		snprintf(loc_string, 128, " (%s)", ps->fname);
	else
		loc_string[0] = 0;
	
	ps->errors++;
	
	printf("\e[01;31mERROR%s\e[0m: %s\n", loc_string, buf);
}

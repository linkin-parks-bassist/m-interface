#include <stdlib.h>
#include <string.h>
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


int get_section_start_score(char *str, int current_score)
{
	if (!str)
		return 0;
	
	if (token_is_char(str, '\n')) return 1;
	
	switch (current_score)
	{
		case 0: return 0;
		case 1: return (token_is_char(str, '.')) 		  ? 2 : 0;
		case 2: return (token_is_valid_section_name(str)) ? 3 : 0;
	}
	
	return 0;
}

int m_parse_info_section(m_eff_parsing_state *ps, m_ast_node *section)
{
	if (!ps || !section)
		return ERR_NULL_PTR;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	m_token_ll *tokens = sec->tokens;
	
	if (!tokens)
		return ERR_BAD_ARGS;
	
	ps->current_token = tokens->next;
	
	return NO_ERROR;
}

int m_parse_resources_section(m_eff_parsing_state *ps, m_ast_node *section)
{
	if (!ps || !section)
		return ERR_NULL_PTR;
	
	m_eff_desc_file_section *sec = (m_eff_desc_file_section*)section->data;
	
	m_token_ll *tokens = sec->tokens;
	
	if (!tokens)
		return ERR_BAD_ARGS;
	
	ps->current_token = tokens->next;
	
	return NO_ERROR;
}


int m_parse_options_section(m_eff_parsing_state *ps, m_ast_node *section)
{
	if (!ps || !section)
		return ERR_NULL_PTR;
	
	if (section->type != M_AST_NODE_SECTION)
		return ERR_BAD_ARGS;
	
	m_token_ll *tokens = (m_token_ll*)section->data;
	
	if (!tokens)
		return ERR_BAD_ARGS;
	
	return NO_ERROR;
}

int m_parse_tokens(m_eff_parsing_state *ps)
{
	if (!ps)
		return ERR_NULL_PTR;
	
	m_token_ll *tokens = ps->tokens;
	m_ast_node **root_ptr = &ps->ast;
	
	m_ast_node *root = m_alloc(sizeof(m_ast_node));
	
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
					// Found the start of a section
					printf("Section found: %s\n", current_token->data);
					
					next_section = m_alloc(sizeof(m_ast_node));
					
					if (!next_section)
						return ERR_ALLOC_FAIL;
					
					next_section->type  = M_AST_NODE_SECTION;
					next_section->data  = NULL;
					next_section->next  = NULL;
					next_section->child = NULL;
					
					ns = m_alloc(sizeof(m_eff_desc_file_section));
					
					if (!ns)
					{
						m_free(next_section);
						return ERR_ALLOC_FAIL;
					}
					
					ns->name = m_strndup(current_token->data, 16);
					
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
					printf("ERROR: Invalid section name \"%s\" on line %d\n", current_token->data, line);
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
		
		//printf("Parse section \"%s\"\n", sect->name);
		
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
		else if (strcmp(sect->name, "OPTIONS") == 0)
		{
			if ((ret_val = m_parse_dictionary_section(ps, current_section)) != NO_ERROR)
			{
				return ret_val;
			}
		}
		else if (strcmp(sect->name, "CODE") == 0)
		{
			code_section = current_section;
			ret_val = NO_ERROR;
		}
		else
		{
			printf("Invalid section name \"%s\"\n", sect->name);
			return ERR_BAD_ARGS;
		}
	
		if (ret_val == NO_ERROR)
		{
			printf("Sucessfully parsed section\n");
		}
		else
		{
			printf("Failed to parse section. Error code: %d\n", ret_val);
			return ret_val;
		}
		current_section = current_section->next;
	}
	
	if (info_section)
	{
		if ((ret_val = m_dict_section_lookup_str(info_section, "name", &ps->name)) != NO_ERROR)
		{
			printf("Error: name missing! (Error code %d)\n", ret_val);
			return ret_val;
		}
		else
		{
			printf("Found name: \"%s\"\n", ps->name ? ps->name : "(NULL)");
		}
	}
	else
	{
		printf("Error: INFO section missing\n");
	}
	
	if (resources_section)
	{
		if ((ret_val = m_extract_resources(&ps->resources, resources_section)) != NO_ERROR)
		{
			printf("Error extracting resources! Error code %d\n", ret_val);
			return ret_val;
		}
		
		m_resources_assign_handles(ps->resources);
	}
	
	if (parameters_section)
	{
		if ((ret_val = m_extract_parameters(&ps->parameters, parameters_section)) != NO_ERROR)
		{
			printf("Error extracting parameters! Error code %d\n", ret_val);
			return ret_val;
		}
		
		m_parameters_assign_ids(ps->parameters);
	}
	
	if (code_section)
	{
		if ((ret_val = m_parse_code_section(ps, code_section)) != NO_ERROR)
		{
			printf("Error parsing code!\n");
			return ret_val;
		}
		
		m_compute_register_formats(ps);
	}
	
	return NO_ERROR;
}

int init_parsing_state(m_eff_parsing_state *ps)
{
	if (!ps)
		return ERR_NULL_PTR;
	
	ps->tokens = NULL;
	ps->current_token = NULL;
	
	ps->resources = NULL;
	ps->blocks = NULL;
	
	return NO_ERROR;
}

int m_parsing_state_cleanup(m_eff_parsing_state *ps)
{
	if (!ps)
		return ERR_NULL_PTR;
	
	// delete everything not handed to effect descriptor. prevent memory leaks. please
	
	return NO_ERROR;
}

m_effect_desc *m_read_eff_desc_from_file(char *fname)
{
	m_effect_desc *result = NULL;
	m_eff_parsing_state parsing_state;
	
	parsing_state.parameters = NULL;
	parsing_state.resources = NULL;
	parsing_state.tokens = NULL;
	parsing_state.blocks = NULL;
	
	FILE *src = fopen(fname, "r");
	
	m_token_ll *tokens = NULL;
	
	m_tokenize_eff_file(src, &parsing_state.tokens);
	
	int j = 0;
	
	m_block_pll *blocks = NULL;
	m_dsp_resource_pll *res = NULL;
	
	int ret_val = m_parse_tokens(&parsing_state);
	
	if (ret_val == NO_ERROR)
	{
		printf("File \"%s\" parsed sucessfully\n", fname);
	}
	else
	{
		printf("File \"%s\" parsing failed. Error code: %d\n", fname, ret_val);
	}
	
	result = m_alloc(sizeof(m_effect_desc));
	
	if (result)
	{
		result->parameters = parsing_state.parameters;
		result->resources = parsing_state.resources;
		result->blocks = parsing_state.blocks;
		
		m_effect_desc_generate_res_rpt(result);
	}
	
	return result;
}

int m_effect_desc_generate_res_rpt(m_effect_desc *eff)
{
	if (!eff)
		return ERR_NULL_PTR;
	
	unsigned int blocks = 0;
	unsigned int memory = 0;
	unsigned int delays = 0;
	
	m_block_pll *cb = eff->blocks;
	
	while (cb)
	{
		blocks++;
		cb = cb->next;
	}
	
	m_dsp_resource_pll *cr = eff->resources;
	
	while (cr)
	{
		if (cr->data)
		{
			switch (cr->data->type)
			{
				case M_DSP_RESOURCE_MEM:
					memory += cr->data->size;
					break;
				case M_DSP_RESOURCE_DELAY:
					delays += 1;
					break;
			}
		}
		cr = cr->next;
	}
	
	eff->res_rpt.blocks = blocks;
	eff->res_rpt.memory = memory;
	eff->res_rpt.delays = delays;
	
	return NO_ERROR;
}

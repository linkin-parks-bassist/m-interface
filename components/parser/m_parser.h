#ifndef M_PARSER_H_
#define M_PARSER_H_

#define N_BLOCKS 256

#define m_alloc 	malloc
#define m_free 		free
#define m_strndup	strndup
#define m_realloc	realloc

#define NO_ERROR		0
#define ERR_NULL_PTR 	1
#define ERR_BAD_ARGS 	2
#define ERR_ALLOC_FAIL 	3

typedef struct
{
	unsigned int blocks;
	unsigned int memory;
	unsigned int delays;
} m_eff_resource_report;

typedef struct {
	m_block_pll *blocks;
	m_parameter_pll *parameters;
	m_dsp_resource_pll *resources;
	
	m_eff_resource_report res_rpt;
} m_effect_desc;

int m_effect_desc_generate_res_rpt(m_effect_desc *eff);

#define M_AST_NODE_ROOT		0
#define M_AST_NODE_SECTION	1
#define M_AST_NODE_DICT		2

typedef struct m_ast_node {
	int type;
	void *data;
	
	struct m_ast_node *child;
	struct m_ast_node *next;
} m_ast_node;

struct m_dictionary;

typedef struct {
	char *name;
	m_token_ll *tokens;
	struct m_dictionary *dict;
} m_eff_desc_file_section;

typedef struct {
	const char *name;
	char *contents;
	
	int len;
	int current_line;
	
	char *version_string;
	
	m_effect_desc *result;
	m_token_ll *tokens;
	
	m_token_ll *current_token;
	
	m_block_pll *blocks;
	m_parameter_pll *parameters;
	m_dsp_resource_pll *resources;
	
	m_ast_node *ast;
} m_eff_parsing_state;

typedef struct {
	m_token_ll *start;
	m_token_ll *end;
	int start_line;
} m_token_span;

static const char *ver_str = "v1.0";


#endif

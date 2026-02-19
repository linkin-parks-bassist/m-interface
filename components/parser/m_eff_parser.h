#ifndef M_PARSER_H_
#define M_PARSER_H_

#define M_AST_NODE_ROOT		0
#define M_AST_NODE_SECTION	1
#define M_AST_NODE_DICT		2

typedef struct m_ast_node {
	int type;
	void *data;
	
	struct m_ast_node *child;
	struct m_ast_node *next;
} m_ast_node;

typedef struct {
	const char *fname;
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

extern const char *ver_str;

struct m_dictionary;

int m_parse_dict_val(m_eff_parsing_state *ps, m_dictionary_entry *result);
int m_parse_dictionary_section(m_eff_parsing_state *ps, m_ast_node *section);
int m_parse_code_section(m_eff_parsing_state *ps, m_ast_node *section);
int m_parse_dictionary(m_eff_parsing_state *ps, m_dictionary **result, const char *name);

m_effect_desc *m_read_eff_desc_from_file(char *fname);

void m_parser_print_info   (m_eff_parsing_state *ps, const char *error_msg, ...);
void m_parser_print_warning(m_eff_parsing_state *ps, const char *error_msg, ...);
void m_parser_print_error  (m_eff_parsing_state *ps, const char *error_msg, ...);

#endif

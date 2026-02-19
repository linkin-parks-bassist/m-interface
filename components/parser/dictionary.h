#ifndef DICTIONARY_H_
#define DICTIONARY_H_

#define DICT_ENTRY_TYPE_NOT_FOUND 	22222
#define DICT_ENTRY_TYPE_INT 		0
#define DICT_ENTRY_TYPE_FLOAT 		1
#define DICT_ENTRY_TYPE_STR 		2
#define DICT_ENTRY_TYPE_DQ 			3
#define DICT_ENTRY_TYPE_SUBDICT		4

struct m_dictionary;

typedef struct {
	const char *name;
	int type;
	union {
		int val_int;
		float val_float;
		const char *val_string;
		m_derived_quantity *val_dq;
		struct m_dictionary *val_dict;
	} value;
} m_dict_entry;

typedef struct m_dictionary {
	const char *name;
	
	int n_entries;
	int entry_array_length;
	m_dict_entry *entries;
} m_dictionary;

m_dictionary *m_new_dictionary();

int m_dictionary_lookup_str(m_dictionary *dict, const char *name, const char **result);
int m_dictionary_lookup_float(m_dictionary *dict, const char *name, float *result);
int m_dictionary_lookup_int(m_dictionary *dict, const char *name, int *result);
int m_dictionary_lookup_dq(m_dictionary *dict, const char *name, m_derived_quantity **result);
int m_dictionary_lookup_dict(m_dictionary *dict, const char *name, m_dictionary **result);

int m_dict_section_lookup_str(m_ast_node *section, const char *name, const char **result);
int m_dict_section_lookup_float(m_ast_node *section, const char *name, float *result);
int m_dict_section_lookup_int(m_ast_node *section, const char *name, int *result);
int m_dict_section_lookup_dq(m_ast_node *section, const char *name, m_derived_quantity **result);
int m_dict_section_lookup_dict(m_ast_node *section, const char *name, m_dictionary **result);

int m_parse_dictionary_section(m_eff_parsing_state *ps, m_ast_node *section);
int m_parse_dictionary(m_eff_parsing_state *ps, m_dictionary **result, const char *name);

#define ERR_NOT_FOUND  10
#define ERR_WRONG_TYPE 11

void print_dict(m_dictionary *dict);

#endif

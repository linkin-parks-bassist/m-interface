#ifndef M_DICTIONARY_H_
#define M_DICTIONARY_H_

#define DICT_ENTRY_TYPE_NOT_FOUND 	22222
#define DICT_ENTRY_TYPE_NOTHING 	22221
#define DICT_ENTRY_TYPE_INT 		0
#define DICT_ENTRY_TYPE_FLOAT 		1
#define DICT_ENTRY_TYPE_STR 		2
#define DICT_ENTRY_TYPE_EXPR 		3
#define DICT_ENTRY_TYPE_SUBDICT		4

struct m_dictionary;

typedef struct {
	const char *name;
	int type;
	union {
		int val_int;
		float val_float;
		const char *val_string;
		m_expression *val_expr;
		struct m_dictionary *val_dict;
	} value;
} m_dictionary_entry;

typedef struct m_dictionary {
	const char *name;
	
	int n_entries;
	int entry_array_length;
	m_dictionary_entry *entries;
} m_dictionary;

m_dictionary *m_new_dictionary();

int m_dictionary_add_entry(m_dictionary *dict, m_dictionary_entry entry);

int m_dictionary_add_entry_str  (m_dictionary *dict, const char *name, const char *value);
int m_dictionary_add_entry_int  (m_dictionary *dict, const char *name, int value);
int m_dictionary_add_entry_float(m_dictionary *dict, const char *name, float value);
int m_dictionary_add_entry_expr (m_dictionary *dict, const char *name, m_expression *value);
int m_dictionary_add_entry_dict (m_dictionary *dict, const char *name, m_dictionary *value);

int m_dictionary_lookup_str  (m_dictionary *dict, const char *name, const char **result);
int m_dictionary_lookup_float(m_dictionary *dict, const char *name, float *result);
int m_dictionary_lookup_int  (m_dictionary *dict, const char *name, int *result);
int m_dictionary_lookup_expr (m_dictionary *dict, const char *name, m_expression **result);
int m_dictionary_lookup_dict (m_dictionary *dict, const char *name, m_dictionary **result);

#define ERR_NOT_FOUND  10
#define ERR_WRONG_TYPE 11

void print_dict(m_dictionary *dict);

#endif

#ifndef TOK_H_
#define TOK_H_

#define TOKENIZER_STATE_IDLE			0
#define TOKENIZER_STATE_NAME			1
#define TOKENIZER_STATE_NUMBER			2
#define TOKENIZER_STATE_NUMBER_HEX		3
#define TOKENIZER_STATE_NUMBER_BIN		4
#define TOKENIZER_STATE_STRING			5
#define TOKENIZER_STATE_STRESC			6
#define TOKENIZER_STATE_LEADING_ZERO	7
#define TOKENIZER_STATE_DONE			8

#define TOKENIZER_POLICY_DISCARD   		0
#define TOKENIZER_POLICY_ACCEPT	   		1
#define TOKENIZER_POLICY_SINGULAR  		2
#define TOKENIZER_POLICY_BEGIN	   		3
#define TOKENIZER_POLICY_END_ACCEPT   	4
#define TOKENIZER_POLICY_END_DISCARD 	5
#define TOKENIZER_POLICY_COMPLAIN		6

typedef struct string_ll {
	char *data;
	struct string_ll *next;
} string_ll;

typedef struct m_token_ll {
	char *data;
	int line;
	int index;
	struct m_token_ll *next;
} m_token_ll;

typedef char* m_token;

int m_tokenize_eff_file(FILE *file, m_token_ll **tokens);

int m_token_ll_safe_append(m_token_ll **list_ptr, char *x, int line, int index);

int m_token_ll_advance(m_token_ll **list);
int m_token_ll_skip_ws(m_token_ll **list);

int token_is_newline(char *str);
int token_is_char(char *str, char c);
int token_is_valid_section_name(char *str);

int token_is_int(char *token);
int token_is_number(char *token);
float token_to_float(char *token);
int token_is_name(char *token);


m_token_ll *m_token_span_to_ll(m_token_ll *start, m_token_ll *end);

#endif

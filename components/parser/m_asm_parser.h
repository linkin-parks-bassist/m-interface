#ifndef ASM_H_
#define ASM_H_

#define M_ASM_ARG_CHANNEL 	0
#define M_ASM_ARG_EXPR 		1
#define M_ASM_ARG_RES  		2
#define M_ASM_ARG_INT  		3

#define INSTR_MAX_ARGS 4

typedef struct {
	int type;
	int addr;
	int val;
	struct m_expression *expr;
	m_dsp_resource *res;
	const char *res_name;
} m_asm_arg;

typedef struct {
	int instr;
	
	int n_args;
	m_asm_arg args[INSTR_MAX_ARGS];
} m_asm_instr;

int m_parse_asm(m_eff_parsing_state *ps);

#endif

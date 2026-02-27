#ifndef EXPR_H_
#define EXPR_H_

#define M_EXPR_FORM_ATOMIC   0
#define M_EXPR_FORM_UNARY_OP 1
#define M_EXPR_FORM_UNARY_FN 2
#define M_EXPR_FORM_INFIX_OP 3
#define M_EXPR_FORM_NORM	 4

#define M_EXPR_CONST 	0
#define M_EXPR_REF 		1
#define M_EXPR_NEG		2
#define M_EXPR_ADD 		3
#define M_EXPR_SUB 		4
#define M_EXPR_MUL 		5
#define M_EXPR_DIV 		6
#define M_EXPR_ABS 		7
#define M_EXPR_SQR 		8
#define M_EXPR_SQRT 	9
#define M_EXPR_EXP 		10
#define M_EXPR_LN 		11
#define M_EXPR_POW 		12
#define M_EXPR_SIN 		13
#define M_EXPR_SINH 	14
#define M_EXPR_COS 		15
#define M_EXPR_COSH 	16
#define M_EXPR_TAN 		17
#define M_EXPR_TANH 	18
#define M_EXPR_ASIN 	19
#define M_EXPR_ACOS 	20
#define M_EXPR_ATAN 	21

#define M_EXPR_TYPE_MAX_VAL M_EXPR_ATAN

#define M_EXPR_REC_MAX_DEPTH 128

//#define M_BOUNDS_CHECK_VERBOSE

typedef struct m_expression
{
	int type;
	int constant;
	int cached;
	float cached_val;
	union {
		float val_float;
		char *ref_name;
		struct m_expression **sub_exprs;
	} val;
} m_expression;

m_expression *new_m_expression_const(float v);
m_expression *new_m_expression_reference(char *ref_name);
m_expression *new_m_expression_unary(int unary_type, m_expression *rhs);
m_expression *new_m_expression_binary(int binary_type, m_expression *arg_1, m_expression *arg_2);

int m_expression_references_param(m_expression *expr, m_parameter *param);

float m_expression_evaluate(m_expression *expr, m_expr_scope *scope);

int m_expression_is_constant(m_expression *expr);

float m_expression_min(m_expression *expr, m_parameter_pll *params);
float m_expression_max(m_expression *expr, m_parameter_pll *params);

int m_expression_detect_constants(m_expression *expr);

typedef struct m_interval
{
	float a;
	float b;
} m_interval;

m_interval m_interval_real_line();
m_interval m_interval_ab(float a, float b);
m_interval m_interval_a_(float a);
m_interval m_interval__b(float b);
m_interval m_interval_singleton(float v);

m_interval m_expression_compute_range(m_expression *expr, m_expr_scope *scope);

char *m_expression_type_to_str(int type);
int m_expression_print(m_expression *expr);
const char *m_expression_to_string(m_expression *expr);

#endif

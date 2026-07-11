#ifndef EXPR_H_
#define EXPR_H_

#define KEST_EXPR_FORM_ATOMIC   	0
#define KEST_EXPR_FORM_UNARY_OP 	1
#define KEST_EXPR_FORM_UNARY_FN 	2
#define KEST_EXPR_FORM_INFIX_OP 	3
#define KEST_EXPR_FORM_NORM	 		4
#define KEST_EXPR_FORM_BINARY_FN 	5

#define KEST_EXPR_CONST 	0
#define KEST_EXPR_REF 		1
#define KEST_EXPR_NEG		2
#define KEST_EXPR_ADD 		3
#define KEST_EXPR_SUB 		4
#define KEST_EXPR_MUL 		5
#define KEST_EXPR_DIV 		6
#define KEST_EXPR_ABS 		7
#define KEST_EXPR_SQR 		8
#define KEST_EXPR_SQRT 		9
#define KEST_EXPR_EXP 		10
#define KEST_EXPR_LN 		11
#define KEST_EXPR_POW 		12
#define KEST_EXPR_SIN 		13
#define KEST_EXPR_SINH 		14
#define KEST_EXPR_COS 		15
#define KEST_EXPR_COSH 		16
#define KEST_EXPR_TAN 		17
#define KEST_EXPR_TANH 		18
#define KEST_EXPR_ASIN 		19
#define KEST_EXPR_ACOS 		20
#define KEST_EXPR_ATAN 		21
#define KEST_EXPR_LOG10		22
#define KEST_EXPR_ERF		23
#define KEST_EXPR_MIN		24
#define KEST_EXPR_MAX		25

#define KEST_EXPR_TYPE_MAX_VAL KEST_EXPR_MAX
#define KEST_EXPR_MAX_ARITY 2

#define KEST_EXPR_REC_MAX_DEPTH 128

//#define KEST_BOUNDS_CHECK_VERBOSE

typedef struct kest_expression
{
	int type;
	int constant;
	int cached;
	float cached_val;
	union {
		float val_float;
		char *ref_name;
	} val;
	
	struct kest_expression *sub_exprs[KEST_EXPR_MAX_ARITY];
} kest_expression;

DECLARE_PTR_LIST(kest_expression);

int kest_expr_init_const(kest_expression *expr, float v);

kest_expression kest_expression_const(float v);
kest_expression *kest_expr_new_const(float v);
kest_expression *kest_expr_new_reference(char *ref_name);
kest_expression *kest_expr_new_unary(int unary_type, kest_expression *rhs);
kest_expression *kest_expr_new_binary(int binary_type, kest_expression *arg_1, kest_expression *arg_2);

struct kest_parameter;
struct kest_parameter_pll;

int kest_expression_references_param(kest_expression *expr, struct kest_parameter *param);

float kest_expression_evaluate(kest_expression *expr, kest_scope *scope);
float kest_expression_evaluate_rec(kest_expression *expr, kest_scope *scope, int depth);

int kest_expression_is_constant(kest_expression *expr);

float kest_expression_min(kest_expression *expr, struct kest_parameter_pll *params);
float kest_expression_max(kest_expression *expr, struct kest_parameter_pll *params);

int kest_expression_detect_constants(kest_expression *expr);

typedef struct kest_interval
{
	float a;
	float b;
} kest_interval;

kest_interval kest_interval_real_line();
kest_interval kest_interval_ab(float a, float b);
kest_interval kest_interval_a_(float a);
kest_interval kest_interval__b(float b);
kest_interval kest_interval_singleton(float v);

float kest_expression_compute_min(kest_expression *expr, kest_scope *scope);
float kest_expression_compute_max(kest_expression *expr, kest_scope *scope);
kest_interval kest_expression_compute_range(kest_expression *expr, kest_scope *scope);
kest_interval kest_expression_compute_instantaneous_range(kest_expression *expr, kest_scope *scope);

char *kest_expression_type_to_str(int type);
int kest_expression_print(kest_expression *expr);
const char *kest_expression_to_string(kest_expression *expr);

int kest_expression_get_references(kest_expression *expr, string_list *names);
int kest_expression_updated_in_scope(kest_expression *expr, kest_scope *scope);

extern kest_expression kest_expression_standard_gain_min;
extern kest_expression kest_expression_standard_gain_max;
extern kest_expression kest_expression_zero;
extern kest_expression kest_expression_one;
extern kest_expression kest_expression_two;
extern kest_expression kest_expression_minus_one;
extern kest_expression kest_expression_minus_two;
extern kest_expression kest_expression_half;
extern kest_expression kest_expression_minus_half;
extern kest_expression kest_expression_pi;
extern kest_expression kest_expression_2pi;
extern kest_expression kest_expression_e;
extern kest_expression kest_expression_sample_rate;
extern kest_expression kest_expression_data_width;
extern kest_expression kest_expression_int_max;
extern kest_expression kest_expression_int_min;
extern kest_expression kest_expression_freq_max;
extern kest_expression kest_expression_nyquist;
extern kest_expression kest_expression_2pi_over_fs;
extern kest_expression kest_expression_root_2_over_2;

int kest_expr_create_lpf_coefficients(kest_expression **array, kest_expression *cutoff, kest_expression *Q);
int kest_expr_create_hpf_coefficients(kest_expression **array, kest_expression *cutoff, kest_expression *Q);
int kest_expr_create_bpf_coefficients(kest_expression **array, kest_expression *center, kest_expression *Q);

typedef struct {
	const char *name;
	kest_expression *expr;
} kest_named_expression;

DECLARE_LINKED_PTR_LIST(kest_named_expression);

DECLARE_POOL(kest_expression);
extern kest_allocator kest_expression_allocator;
extern kest_expression_pool kest_expression_mem_pool;

void max_depth_bp(const char *string);

#endif

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "kest_int.h"

#define PRINTLINES_ALLOWED 0

//#define KEST_EXPR_EVAL_VERBOSE
//#define KEST_BOUNDS_CHECK_VERBOSE

static const char *FNAME = "kest_expression.c";

IMPLEMENT_PTR_LIST(kest_expression);
IMPLEMENT_LINKED_PTR_LIST(kest_named_expression);

IMPLEMENT_POOL(kest_expression);
kest_allocator kest_expression_allocator;
kest_expression_pool kest_expression_mem_pool;

#define KEST_EXPRESSION_CONST(x) { 		\
	.type = KEST_EXPR_CONST,			\
	.val = {.val_float = (float)(x)},	\
	.constant = 1,						\
	.cached = 1,						\
	.cached_val = (float)(x)			\
};

void max_depth_bp(const char *string)
{
	return;
}

kest_expression kest_expression_standard_gain_min 	= KEST_EXPRESSION_CONST(KEST_STANDARD_GAIN_MIN);
kest_expression kest_expression_standard_gain_max 	= KEST_EXPRESSION_CONST(KEST_STANDARD_GAIN_MAX);
kest_expression kest_expression_zero 				= KEST_EXPRESSION_CONST(0);
kest_expression kest_expression_one 				= KEST_EXPRESSION_CONST(1);
kest_expression kest_expression_two 				= KEST_EXPRESSION_CONST(2);
kest_expression kest_expression_minus_one 			= KEST_EXPRESSION_CONST(-1);
kest_expression kest_expression_minus_two 			= KEST_EXPRESSION_CONST(-2);
kest_expression kest_expression_half	 			= KEST_EXPRESSION_CONST(0.5);
kest_expression kest_expression_minus_half 			= KEST_EXPRESSION_CONST(-0.5);
kest_expression kest_expression_pi 					= KEST_EXPRESSION_CONST(M_PI);
kest_expression kest_expression_2pi 				= KEST_EXPRESSION_CONST(2 * M_PI);
kest_expression kest_expression_e 					= KEST_EXPRESSION_CONST(exp(1));
kest_expression kest_expression_sample_rate 		= KEST_EXPRESSION_CONST((float)KEST_FPGA_SAMPLE_RATE);
kest_expression kest_expression_data_width			= KEST_EXPRESSION_CONST((float)KEST_FPGA_DATA_WIDTH);
kest_expression kest_expression_int_max				= KEST_EXPRESSION_CONST( pow(2, (float)KEST_FPGA_DATA_WIDTH - 1) - 1);
kest_expression kest_expression_int_min				= KEST_EXPRESSION_CONST(-pow(2, (float)KEST_FPGA_DATA_WIDTH - 1));
kest_expression kest_expression_freq_max 			= KEST_EXPRESSION_CONST((float)KEST_FPGA_SAMPLE_RATE / 2 - 50);
kest_expression kest_expression_nyquist 			= KEST_EXPRESSION_CONST((float)KEST_FPGA_SAMPLE_RATE / 2);
kest_expression kest_expression_2pi_over_fs 		= KEST_EXPRESSION_CONST((2 * M_PI) / (float)KEST_FPGA_SAMPLE_RATE);
kest_expression kest_expression_root_2_over_2 		= KEST_EXPRESSION_CONST(sqrt(2.0) / 2.0);

kest_expression kest_expression_const(float v)
{
	kest_expression result;
	result.type = KEST_EXPR_CONST;
	result.val.val_float = v;
	result.constant = 1;
	result.cached = 1;
	result.cached_val = v;
	return result;
}

int kest_expr_init_const(kest_expression *expr, float v)
{
	if (!expr) return ERR_NULL_PTR;
	
	*expr = kest_expression_const(v);
	
	return NO_ERROR;
}

kest_expression *kest_expr_new_const(float v)
{
	kest_expression *result = (kest_expression*)kest_allocator_alloc(&kest_expression_allocator, 1);
	
	if (!result) return NULL;
	
	*result = kest_expression_const(v);
	
	return result;
}

int kest_expr_init_unary(kest_expression *expr, int unary_type, kest_expression *rhs)
{
	if (!expr) return ERR_NULL_PTR;
	
	expr->type = unary_type;
	
	expr->sub_exprs[0] = rhs;
	expr->cached = 0;
	if (rhs)
		expr->constant = rhs->constant;
	
	return NO_ERROR;
}

kest_expression *kest_expr_new_unary(int unary_type, kest_expression *rhs)
{
	if (!rhs) return NULL;
	
	kest_expression *lhs = (kest_expression*)kest_allocator_alloc(&kest_expression_allocator, 1);
	
	if (!lhs) return NULL;
	
	lhs->type = unary_type;
	
	lhs->sub_exprs[0] = rhs;
	lhs->cached = 0;
	lhs->constant = rhs->constant;
	
	return lhs;
}

int kest_expr_init_binary(kest_expression *expr, int binary_type, kest_expression *arg_1, kest_expression *arg_2)
{
	if (!expr) return ERR_NULL_PTR;
	
	expr->type = binary_type;
	
	expr->sub_exprs[0] = arg_1;
	expr->sub_exprs[1] = arg_2;
	
	expr->cached = 0;
	expr->constant = arg_2->constant && arg_1->constant;
	
	return NO_ERROR;
}

kest_expression *kest_expr_new_binary(int binary_type, kest_expression *arg_1, kest_expression *arg_2)
{
	if (!arg_1 || !arg_2) return NULL;
	
	kest_expression *bin = (kest_expression*)kest_allocator_alloc(&kest_expression_allocator, 1);
	
	if (!bin) return NULL;
	
	bin->type = binary_type;
	
	bin->sub_exprs[0] = arg_1;
	bin->sub_exprs[1] = arg_2;
	
	bin->cached = 0;
	bin->constant = arg_2->constant && arg_1->constant;
	
	return bin;
}

int kest_expr_init_reference(kest_expression *expr, char *ref_name)
{
	if (!expr) return ERR_NULL_PTR;
	
	expr->type = KEST_EXPR_REF;
	expr->val.ref_name = kest_strndup(ref_name, 64);
	
	if (!expr->val.ref_name)
	{
		return ERR_ALLOC_FAIL;
	}
	
	expr->constant = 0;
	expr->cached = 0;
	
	return NO_ERROR;
}

kest_expression *kest_expr_new_reference(char *ref_name)
{
	if (!ref_name) return NULL;
	
	kest_expression *result = (kest_expression*)kest_allocator_alloc(&kest_expression_allocator, 1);
	
	if (!result) return NULL;
	
	result->type = KEST_EXPR_REF;
	result->val.ref_name = kest_strndup(ref_name, 64);
	
	if (!result->val.ref_name)
	{
		kest_allocator_free(&kest_expression_allocator, result);
		return NULL;
	}
	
	result->constant = 0;
	result->cached = 0;
	
	return result;
}

int kest_expr_init_neg(kest_expression *expr, kest_expression *a)
{
	return kest_expr_init_unary(expr, KEST_EXPR_NEG, a);
}

kest_expression *kest_expr_new_neg(kest_expression *a)
{
	return kest_expr_new_unary(KEST_EXPR_NEG, a);
}

int kest_expr_init_div(kest_expression *expr, kest_expression *a, kest_expression *b)
{
	return kest_expr_init_binary(expr, KEST_EXPR_DIV, a, b);
}

kest_expression *kest_expr_new_div(kest_expression *a, kest_expression *b)
{
	return kest_expr_new_binary(KEST_EXPR_DIV, a, b);
}

int kest_expr_init_mul(kest_expression *expr, kest_expression *a, kest_expression *b)
{
	return kest_expr_init_binary(expr, KEST_EXPR_MUL, a, b);
}

kest_expression *kest_expr_new_mul(kest_expression *a, kest_expression *b)
{
	return kest_expr_new_binary(KEST_EXPR_MUL, a, b);
}

int kest_expr_init_2x(kest_expression *expr, kest_expression *x)
{
	return kest_expr_init_binary(expr, KEST_EXPR_MUL, &kest_expression_two, x);
}

kest_expression *kest_expr_new_2x(kest_expression *x)
{
	return kest_expr_new_binary(KEST_EXPR_MUL, &kest_expression_two, x);
}

int kest_expr_init_half_x(kest_expression *expr, kest_expression *x)
{
	return kest_expr_init_binary(expr, KEST_EXPR_MUL, &kest_expression_half, x);
}

kest_expression *kest_expr_new_half_x(kest_expression *x)
{
	return kest_expr_new_binary(KEST_EXPR_MUL, &kest_expression_half, x);
}

int kest_expr_init_sin(kest_expression *expr, kest_expression *x)
{
	return kest_expr_init_unary(expr, KEST_EXPR_SIN, x);
}

kest_expression *kest_expr_new_sin(kest_expression *x)
{
	return kest_expr_new_unary(KEST_EXPR_SIN, x);
}

int kest_expr_init_cos(kest_expression *expr, kest_expression *x)
{
	return kest_expr_init_unary(expr, KEST_EXPR_COS, x);
}

kest_expression *kest_expr_new_cos(kest_expression *x)
{
	return kest_expr_new_unary(KEST_EXPR_COS, x);
}

int kest_expr_init_sub(kest_expression *expr, kest_expression *a, kest_expression *b)
{
	return kest_expr_init_binary(expr, KEST_EXPR_SUB, a, b);
}

kest_expression *kest_expr_new_sub(kest_expression *a, kest_expression *b)
{
	return kest_expr_new_binary(KEST_EXPR_SUB, a, b);
}

int kest_expr_init_sum(kest_expression *expr, kest_expression *a, kest_expression *b)
{
	return kest_expr_init_binary(expr, KEST_EXPR_ADD, a, b);
}

kest_expression *kest_expr_new_sum(kest_expression *a, kest_expression *b)
{
	return kest_expr_new_binary(KEST_EXPR_ADD, a, b);
}

char *kest_expression_type_to_str(int type)
{
	switch (type)
	{
		case KEST_EXPR_CONST: 	return "CONST";
		case KEST_EXPR_NEG:		return "NEG";
		case KEST_EXPR_REF: 	return "REF";
		case KEST_EXPR_ADD: 	return "ADD";
		case KEST_EXPR_SUB: 	return "SUB";
		case KEST_EXPR_MUL: 	return "MUL";
		case KEST_EXPR_DIV: 	return "DIV";
		case KEST_EXPR_ABS: 	return "ABS";
		case KEST_EXPR_SQR: 	return "SQR";
		case KEST_EXPR_SQRT: 	return "SQRT";
		case KEST_EXPR_EXP: 	return "EXP";
		case KEST_EXPR_LN: 		return "LN";
		case KEST_EXPR_POW: 	return "POW";
		case KEST_EXPR_SIN: 	return "SIN";
		case KEST_EXPR_SINH: 	return "SINH";
		case KEST_EXPR_COS: 	return "COS";
		case KEST_EXPR_COSH: 	return "COSH";
		case KEST_EXPR_TAN: 	return "TAN";
		case KEST_EXPR_TANH: 	return "TANH";
		case KEST_EXPR_ASIN: 	return "ASIN";
		case KEST_EXPR_ACOS:	return "ACOS";
		case KEST_EXPR_ATAN: 	return "ATAN";
		case KEST_EXPR_LOG10: 	return "LOG10";
		case KEST_EXPR_ERF: 	return "ERF";
		case KEST_EXPR_MIN: 	return "MIN";
		case KEST_EXPR_MAX: 	return "MAX";
	}
	
	return "TYPE_UNKNOWN";
}

int kest_expression_form(kest_expression *expr)
{
	if (!expr)
		return KEST_EXPR_FORM_ATOMIC;
	
	switch (expr->type)
	{
		case KEST_EXPR_CONST: 	return KEST_EXPR_FORM_ATOMIC;
		case KEST_EXPR_REF: 	return KEST_EXPR_FORM_ATOMIC;
		case KEST_EXPR_NEG:		return KEST_EXPR_FORM_UNARY_OP;
		case KEST_EXPR_ADD: 	return KEST_EXPR_FORM_INFIX_OP;
		case KEST_EXPR_SUB: 	return KEST_EXPR_FORM_INFIX_OP;
		case KEST_EXPR_MUL: 	return KEST_EXPR_FORM_INFIX_OP;
		case KEST_EXPR_DIV: 	return KEST_EXPR_FORM_INFIX_OP;
		case KEST_EXPR_ABS: 	return KEST_EXPR_FORM_NORM;
		case KEST_EXPR_SQR: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_SQRT: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_EXP: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_LN: 		return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_POW: 	return KEST_EXPR_FORM_INFIX_OP;
		case KEST_EXPR_SIN: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_SINH: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_COS: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_COSH: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_TAN: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_TANH: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_ASIN: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_ACOS:	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_ATAN: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_LOG10: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_ERF: 	return KEST_EXPR_FORM_UNARY_FN;
		case KEST_EXPR_MIN: 	return KEST_EXPR_FORM_BINARY_FN;
		case KEST_EXPR_MAX: 	return KEST_EXPR_FORM_BINARY_FN;
	}
	
	return KEST_EXPR_FORM_ATOMIC;
}

// Compute arity in the sense of, how many sub-expr's it uses.
// this is used to guard accesses to the array expr->sub_exprs.
// therefore, if in doubt, return 0.
// it should not return x if expr->val.sub_expr[x-1]
// is not a valid pointer to another expr
int kest_expression_arity(kest_expression *expr)
{
	if (!expr) return NO_ERROR;
	
	// if the type is nonsense, return 0
	if (expr->type < 0 || expr->type > KEST_EXPR_TYPE_MAX_VAL)
		return 0;
	
	if (expr->type == KEST_EXPR_CONST || expr->type == KEST_EXPR_REF)
		return 0;
	
	// arity is at least 1 if we reach this point. there are more arity 1 types than arity 2, but also arity 2 will
	// be more common, bc arithmetic. and none of arity 3. therefore, check the arity 2 case, then return 1 otherwise
	if (expr->type == KEST_EXPR_ADD
	 || expr->type == KEST_EXPR_SUB
	 || expr->type == KEST_EXPR_MUL
	 || expr->type == KEST_EXPR_DIV
	 || expr->type == KEST_EXPR_POW)
		return 2;
	
	return 1;
}

int kest_expression_is_constant_reference(kest_expression *expr)
{
	if (!expr)
		return 1;
	
	int ret_val = 0;
	
	if (expr->type == KEST_EXPR_REF)
	{
		if (!ret_val) ret_val |= (strcmp(expr->val.ref_name,          "pi") == 0);
		if (!ret_val) ret_val |= (strcmp(expr->val.ref_name,           "e") == 0);
		if (!ret_val) ret_val |= (strcmp(expr->val.ref_name, "sample_rate") == 0);
		if (!ret_val) ret_val |= (strcmp(expr->val.ref_name,  "data_width") == 0);
		
		if (ret_val) expr->constant = 1;
	}
	
	return ret_val;
}

int kest_expression_is_constant(kest_expression *expr)
{
	if (!expr) return 0;
	return (expr->type == KEST_EXPR_CONST) || expr->constant || kest_expression_is_constant_reference(expr);
}

int kest_expression_detect_constants_rec(kest_expression *expr, int depth)
{
	if (!expr)
		return 1;
	
	if (depth > KEST_EXPR_REC_MAX_DEPTH)
	{
		max_depth_bp(kest_expression_to_string(expr));
		return 1;
	}
	
	//KEST_PRINTF("The expression \"%s\" ", kest_expression_to_string(expr));
	
	int ret_val = 1;
	
	if (expr->type == KEST_EXPR_CONST)
	{
		//KEST_PRINTF("is a constant.\n");
		ret_val = 1;
		goto detect_constants_finish;
	}
	
	if (expr->type == KEST_EXPR_REF)
	{
		//KEST_PRINTF("is a reference");
		ret_val = kest_expression_is_constant_reference(expr);
		//if (ret_val)
		//	KEST_PRINTF(" to a constant.\n");
		//else
		//	KEST_PRINTF(" to a variable.\n");
		goto detect_constants_finish;
	}
	
	int arity = kest_expression_arity(expr);
	int sub_expr_cst;
	
	if (arity > 0)
	{
		//KEST_PRINTF("has top-level arity %d. To see if it's constant, we check its %d top-level sub-expressions.\n", arity, arity);
		
		for (int i = 0; ret_val && i < arity; i++)
		{
			sub_expr_cst = expr->sub_exprs[i] ? kest_expression_detect_constants_rec(expr->sub_exprs[i], depth + 1) : 1;
			
			ret_val = ret_val && sub_expr_cst;
		}
	}
	
detect_constants_finish:
	expr->constant = ret_val;
	//KEST_PRINTF("Therefore, \"%s\" is %sconstant.\n", kest_expression_to_string(expr), ret_val ? "" : "NOT ");
	
	return ret_val;
}

int kest_expression_detect_constants(kest_expression *expr)
{
	return kest_expression_detect_constants_rec(expr, 0);
}

float kest_expression_evaluate_rec(kest_expression *expr, kest_scope *scope, int depth)
{
	kest_parameter_pll *current;
	kest_scope_entry *ref;
	kest_parameter *param;
	int cmplen;
	
	float x = 0.0;
	float y;
	float ret_val;
	
	int ret;
	
	#ifdef KEST_EXPR_EVAL_VERBOSE
	KEST_PRINTF("[Depth %d] kest_expression_evaluate_rec(%p = \"%s\") in scope %p\n", depth, expr, kest_expression_to_string(expr), scope);
	#endif
	
	if (depth > KEST_EXPR_REC_MAX_DEPTH)
	{
		max_depth_bp(kest_expression_to_string(expr));
		KEST_PRINTF("kest_expression_evaluate(): Error: maximum recursion depth %d exceeded (possible dependency loop)\n", KEST_EXPR_REC_MAX_DEPTH);
		ret_val = 0.0;
		goto expr_compute_return;
	}
	
	if (!expr)
	{
		#ifdef KEST_EXPR_EVAL_VERBOSE
		KEST_PRINTF("expr compute: NULL expr!\n");
		#endif
		return 0.0;
	}
	
	if (expr->constant && expr->cached)
	{
		ret_val = expr->cached_val;
		goto expr_compute_return;
	}
	
	if (expr->type == KEST_EXPR_CONST)
	{
		ret_val = expr->val.val_float;
		goto expr_compute_return;
	}
	
	if (expr->type == KEST_EXPR_REF)
	{
		if (!expr->val.ref_name)
		{
			KEST_PRINTF("Error evaluating expression \"%s\": expr->val.ref_name = NULL!\n",
				kest_expression_to_string(expr));
			ret_val = 0.0;
			goto expr_compute_return;
		}
		
		if (strcmp(expr->val.ref_name, "pi") == 0)
		{
			ret_val = M_PI;
			expr->constant = 1;
			goto expr_compute_return;
		}
		else if (strcmp(expr->val.ref_name, "e") == 0)
		{
			ret_val = exp(1);
			expr->constant = 1;
			goto expr_compute_return;
		}
		else if (strcmp(expr->val.ref_name, "sample_rate") == 0)
		{
			ret_val = KEST_FPGA_SAMPLE_RATE;
			expr->constant = 1;
			goto expr_compute_return;
		}
		
		if (!scope)
		{
			KEST_PRINTF("Error evaluating expression \"%s\": expression refers to non-constant \"%s\", but no scope given!\n",
				kest_expression_to_string(expr), expr->val.ref_name);
			ret_val = 0.0;
			goto expr_compute_return;
		}
		
		ref = kest_scope_fetch(scope, expr->val.ref_name);
		
		if (!ref)
		{
			if (strcmp(expr->val.ref_name,          "pi") == 0) return M_PI;
			if (strcmp(expr->val.ref_name,           "e") == 0) return exp(1);
			if (strcmp(expr->val.ref_name, "sample_rate") == 0) return (float)KEST_FPGA_SAMPLE_RATE;
			if (strcmp(expr->val.ref_name,  "data_width") == 0) return (float)KEST_FPGA_DATA_WIDTH;
			
			KEST_PRINTF("Error evaluating expression \"%s\": expression refers to non-constant \"%s\", but it isn't found in scope!\n",
				kest_expression_to_string(expr), expr->val.ref_name);
			ret_val = 0.0;
			goto expr_compute_return;
		}
		
		ret = kest_scope_entry_eval_rec(ref, scope, &ret_val, depth + 1);
		
		if (ret != NO_ERROR)
		{
			KEST_PRINTF("Error evaluating expression \"%s\": reference \"%s\" failed to evaluate, with error code %d\n",
					kest_expression_to_string(expr), expr->val.ref_name, kest_error_code_to_string(ret));
			ret_val = 0.0f;
		}
		
		goto expr_compute_return;
	}
	
	switch (expr->type)
	{
		case KEST_EXPR_NEG:
			ret_val = -(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;
			
		case KEST_EXPR_ADD:
			ret_val = (kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1) + kest_expression_evaluate_rec(expr->sub_exprs[1], scope, depth + 1));
			break;

		case KEST_EXPR_SUB:
			ret_val = kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1) - kest_expression_evaluate_rec(expr->sub_exprs[1], scope, depth + 1);
			break;

		case KEST_EXPR_MUL:
			ret_val = kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1) * kest_expression_evaluate_rec(expr->sub_exprs[1], scope, depth + 1);
			break;

		case KEST_EXPR_DIV:
			x = kest_expression_evaluate_rec(expr->sub_exprs[1], scope, depth + 1);
			
			if (fabsf(x) < 1e-20)
			{
				KEST_PRINTF("[Depth %d] expr compute: division by zero!\n", depth);
				ret_val = 0.0;
				goto expr_compute_return; // avoid division by zero by just returning 0 lol. idk. what else to do?
			}
			
			ret_val = kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1) / x;
			break;

		case KEST_EXPR_ABS:
			ret_val = fabs(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;

		case KEST_EXPR_SQR: x = kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1); ret_val = x * x;
			break;

		case KEST_EXPR_SQRT:
			ret_val = sqrt(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;

		case KEST_EXPR_EXP:
			ret_val = exp(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;

		case KEST_EXPR_LN:
			ret_val = log(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;

		case KEST_EXPR_POW:
			ret_val = pow(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1),
						  kest_expression_evaluate_rec(expr->sub_exprs[1], scope, depth + 1));
			break;
		case KEST_EXPR_SIN:
			ret_val = sin(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;
			
		case KEST_EXPR_SINH:
			ret_val = sinh(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;
			
		case KEST_EXPR_ASIN:
			ret_val = asin(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;

		case KEST_EXPR_COS:
			ret_val = cos(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;
			
		case KEST_EXPR_COSH:
			ret_val = cosh(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;
			
		case KEST_EXPR_ACOS:
			ret_val = acos(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;

		case KEST_EXPR_TAN:
			ret_val = tan(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;

		case KEST_EXPR_TANH:
			ret_val = tanh(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;
			
		case KEST_EXPR_ATAN:
			ret_val = atan(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;
			
		case KEST_EXPR_LOG10:
			ret_val = log10(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;
			
		case KEST_EXPR_ERF:
			ret_val = erf(kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1));
			break;
			
		case KEST_EXPR_MIN:
			x = kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1);
			y = kest_expression_evaluate_rec(expr->sub_exprs[1], scope, depth + 1);
			ret_val = (x < y) ? x : y;
			break;
			
		case KEST_EXPR_MAX:
			x = kest_expression_evaluate_rec(expr->sub_exprs[0], scope, depth + 1);
			y = kest_expression_evaluate_rec(expr->sub_exprs[1], scope, depth + 1);
			ret_val = (x > y) ? x : y;
			break;
	}
	
expr_compute_return:

	expr->cached = 1;
	expr->cached_val = ret_val;
	
	#ifdef KEST_EXPR_EVAL_VERBOSE
	KEST_PRINTF("[Depth %d] kest_expression_evaluate_rec(%p = \"%s\") result: %f\n", depth, expr, kest_expression_to_string(expr), ret_val);
	#endif
	
	return ret_val;
}

float kest_expression_evaluate(kest_expression *expr, kest_scope *scope)
{
	float ret_val = kest_expression_evaluate_rec(expr, scope, 0);
	//KEST_PRINTF("Evaluated expression %p = %s = %.04f (scope: %p)\n", expr, kest_expression_to_string(expr), ret_val, scope);
	return ret_val;
}

int kest_expression_references_param_rec(kest_expression *expr, kest_parameter *param, int depth)
{
	if (!expr || !param)
		return NO_ERROR;
	
	if (!param->name_internal)
		return NO_ERROR;
	
	int arity = kest_expression_arity(expr);
	
	if (arity == 0)
	{
		if (expr->type != KEST_EXPR_REF)
			return NO_ERROR;
	
		if (!expr->val.ref_name)
				return NO_ERROR;
			
		return (strncmp(expr->val.ref_name, param->name_internal, strlen(expr->val.ref_name) + 1) == 0);
	}
	
	if (depth > KEST_EXPR_REC_MAX_DEPTH)
	{
		max_depth_bp(kest_expression_to_string(expr));
		return NO_ERROR;
	}
	
	for (int i = 0; i < arity; i++)
	{
		if (kest_expression_references_param_rec(expr->sub_exprs[i], param, depth + 1))
			return ERR_NULL_PTR;
	}
	
	return NO_ERROR;
}

int kest_expression_references_param(kest_expression *expr, kest_parameter *param)
{
	return kest_expression_references_param_rec(expr, param, 0);
}

kest_interval kest_interval_real_line()
{
	kest_interval result;
	result.a = -FLT_MAX;
	result.b =  FLT_MAX;
	return result;
}

kest_interval kest_interval_ab(float a, float b)
{
	kest_interval result;
	result.a = a;
	result.b = b;
	return result;
}

kest_interval kest_interval_a_(float a)
{
	kest_interval result;
	result.a = a;
	result.b = FLT_MAX;
	return result;
}

kest_interval kest_interval__b(float b)
{
	kest_interval result;
	result.a = -FLT_MAX;
	result.b = b;
	return result;
}

kest_interval kest_interval_singleton(float v)
{
	kest_interval result;
	result.a = v;
	result.b = v;
	return result;
}

float kest_expression_compute_max_rec(kest_expression *expr, kest_scope *scope, int depth);

float kest_expression_compute_min_rec(kest_expression *expr, kest_scope *scope, int depth)
{
	kest_parameter_pll *current;
	kest_scope_entry *ref;
	int found;
	
	//KEST_PRINTF("[Depth: %d] Computing min for expression \"%s\"...\n", depth, kest_expression_to_string(expr));
	
	float p1, p2, p3, p4;
	float z;
	int k;
	
	float ret = -FLT_MAX;
	
	float x_min = 0.0f;
	float x_max = 0.0f;
	float y_min = 0.0f;
	float y_max = 0.0f;
	
	int x_min_needed = 0;
	int x_max_needed = 0;
	
	int y_min_needed = 0;
	int y_max_needed = 0;
	
	kest_interval y_int_d;
	
	kest_lfo *lfo;
	
	int p_c = 0;
	
	if (!expr)
	{
		goto expr_int_ret;
	}
	if (depth > KEST_EXPR_REC_MAX_DEPTH)
	{
		max_depth_bp(kest_expression_to_string(expr));
		goto expr_int_ret;
	}
	
	if (expr->constant && expr->cached)
	{
		#ifdef KEST_BOUNDS_CHECK_VERBOSE
		KEST_PRINTF("[Depth: %d] Expression is constant (and cached!), with known value %.3f.\n", depth, expr->cached_val);
		#endif
		ret = expr->cached_val;
		goto expr_int_ret;
	}
	
	if (expr->type == KEST_EXPR_CONST)
	{
		#ifdef KEST_BOUNDS_CHECK_VERBOSE
		KEST_PRINTF("[Depth: %d] Expression is constant and cached, with value %.3f.\n", depth, expr->val.val_float);
		#endif
		ret = expr->val.val_float;
		goto expr_int_ret;
	}
	
	if (expr->type == KEST_EXPR_REF)
	{
		#ifdef KEST_BOUNDS_CHECK_VERBOSE
		KEST_PRINTF("[Depth: %d] Expression is a reference, to \"%s\". Therefore we must compute its min.\n", depth, expr->val.ref_name ? expr->val.ref_name : "(NULL)");
		#endif
		if (kest_expression_is_constant_reference(expr))
		{
			#ifdef KEST_BOUNDS_CHECK_VERBOSE
			KEST_PRINTF("[Depth: %d] The referenced value is a constant,", depth);
			#endif
			if (expr->cached)
				ret = expr->cached_val;
			else
				ret = kest_expression_evaluate(expr, NULL);
			#ifdef KEST_BOUNDS_CHECK_VERBOSE
			KEST_PRINTF(" with value %.4f\n", ret);
			#endif
			goto expr_int_ret;
		}
		
		if (!scope)
		{
			KEST_PRINTF("Error estimating expression (%p): expression refers to non-constant \"%s\", but no scope given!\n",
				expr->val.ref_name);
			goto expr_int_ret;
		}
		
		ref = kest_scope_fetch(scope, expr->val.ref_name);
		
		if (!ref)
		{
			KEST_PRINTF("Error estimating expression (%p): expression refers to non-constant \"%s\", but it isn't found in scope!\n",
				expr, expr->val.ref_name);
			goto expr_int_ret;
		}
		
		switch (ref->type)
		{
			case KEST_SCOPE_ENTRY_TYPE_EXPR:
				#ifdef KEST_BOUNDS_CHECK_VERBOSE
				KEST_PRINTF("[Depth: %d] The referred quantity is an expression, so we recurse and compute its min!\n", depth);
				#endif
				ret = kest_expression_compute_min_rec(ref->val.expr, scope, depth + 1);
				break;
				
			case KEST_SCOPE_ENTRY_TYPE_PARAM:
				if (!ref->val.param)
				{
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("The reference is to a parameter, but, ultimately, it turned up NULL!\n");
					#endif
				}
				else
				{
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("[Depth: %d] The reference is to a parameter. We compute its min.\n", depth);
					#endif
					if (ref->val.param->min_expr)
					{
						ret = kest_expression_evaluate_rec(ref->val.param->min_expr, scope, depth + 1);
					}
					else
					{
						ret = ref->val.param->min;
					}
					
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("[Depth: %d] Obtained parameter min %.4f.\n", depth, ret);
					#endif
				}
				break;
			
			case KEST_SCOPE_ENTRY_TYPE_SETTING:
				if (!ref->val.setting)
				{
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("The reference is to a setting, but, ultimately, it turned up NULL!\n");
					#endif
				}
				else
				{
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("[Depth: %d] The reference is to a setting. We compute its min.\n", depth);
					#endif
					ret = ref->val.setting->min;
					
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("[Depth: %d] Obtained setting min %.4f.\n", depth, ret);
					#endif
				}
				break;

			case KEST_SCOPE_ENTRY_TYPE_MEM:
				#ifdef KEST_BOUNDS_CHECK_VERBOSE
				KEST_PRINTF("[Depth: %d] The reference is to a mem slot. Those have min [-1, 1).\n", depth);
				#endif
				ret = -1.0f;
				break;

			case KEST_SCOPE_ENTRY_TYPE_LFO:
				#ifdef KEST_BOUNDS_CHECK_VERBOSE
				KEST_PRINTF("[Depth: %d] The reference is to an lfo.\n", depth);
				#endif
				
				lfo = (kest_lfo*)ref->val.lfo;
				
				if (!lfo)
				{
					
				}
				else
				{
					ret = kest_expression_compute_min_rec(lfo->min, scope, depth + 1);
				}
				break;
				
			default:
				KEST_PRINTF("Error estimating expression \"%s\": expression refers to non-constant \"%s\", but it has unrecognised type %d!\n",
					kest_expression_to_string(expr), expr->val.ref_name, ref->type);
				break;
		}
		
		goto expr_int_ret;
	}
	
	int arity = kest_expression_arity(expr);
	
	#ifdef KEST_BOUNDS_CHECK_VERBOSE
	KEST_PRINTF("[Depth: %d] The expression has top-level arity %d; therefore we recurse and compute mins of its top-level sub-expressions.\n", depth, arity);
	#endif
	
	if (arity >= 1)
	{
		x_min_needed = (
				expr->type == KEST_EXPR_ADD 	||
				expr->type == KEST_EXPR_SQRT 	||
				expr->type == KEST_EXPR_LN 		||
				expr->type == KEST_EXPR_ASIN 	||
				expr->type == KEST_EXPR_ACOS 	||
				expr->type == KEST_EXPR_ATAN 	||
				expr->type == KEST_EXPR_LOG10 	||
				expr->type == KEST_EXPR_ERF 	||
				expr->type == KEST_EXPR_TANH 	||
				expr->type == KEST_EXPR_SINH 	||
				expr->type == KEST_EXPR_EXP 	||
				expr->type == KEST_EXPR_SUB 	||
				expr->type == KEST_EXPR_SQR 	||
				expr->type == KEST_EXPR_COSH 	||
				expr->type == KEST_EXPR_ABS 	||
				expr->type == KEST_EXPR_MUL 	||
				expr->type == KEST_EXPR_DIV 	||
				expr->type == KEST_EXPR_POW 	||
				expr->type == KEST_EXPR_COS 	||
				expr->type == KEST_EXPR_SIN 	||
				expr->type == KEST_EXPR_TAN		||
				expr->type == KEST_EXPR_MIN		||
				expr->type == KEST_EXPR_MAX
			);
		x_max_needed = (
				expr->type == KEST_EXPR_NEG 	||
				expr->type == KEST_EXPR_ACOS 	||
				expr->type == KEST_EXPR_MUL 	||
				expr->type == KEST_EXPR_SQR 	||
				expr->type == KEST_EXPR_COSH 	||
				expr->type == KEST_EXPR_ABS 	||
				expr->type == KEST_EXPR_DIV 	||
				expr->type == KEST_EXPR_POW 	||
				expr->type == KEST_EXPR_COS 	||
				expr->type == KEST_EXPR_SIN 	||
				expr->type == KEST_EXPR_TAN
			);
		
		if (x_min_needed)
			x_min = kest_expression_compute_min_rec(expr->sub_exprs[0], scope, depth + 1);
		if (x_max_needed)
			x_max = kest_expression_compute_max_rec(expr->sub_exprs[0], scope, depth + 1);
	}
	if (arity >  1)
	{
		y_min_needed = (
				expr->type == KEST_EXPR_ADD ||
				expr->type == KEST_EXPR_MUL ||
				expr->type == KEST_EXPR_DIV ||
				expr->type == KEST_EXPR_POW	||
				expr->type == KEST_EXPR_MIN	||
				expr->type == KEST_EXPR_MAX
			);
		y_max_needed = (
				expr->type == KEST_EXPR_MUL ||
				expr->type == KEST_EXPR_DIV ||
				expr->type == KEST_EXPR_POW
			);
		
		if (y_min_needed)
			y_min = kest_expression_compute_min_rec(expr->sub_exprs[1], scope, depth + 1);
		if (y_max_needed)
			y_max = kest_expression_compute_max_rec(expr->sub_exprs[1], scope, depth + 1);
	}
	
	switch (expr->type)
	{
		case KEST_EXPR_NEG:
			ret = -x_max;
			goto expr_int_ret;
		
		case KEST_EXPR_ADD:
			ret = x_min + y_min;
			goto expr_int_ret;
			
		case KEST_EXPR_SQRT:
			if (x_min < 0) ret = 0;
			else ret = sqrt(x_min);
			
			goto expr_int_ret;
			
		case KEST_EXPR_LN:
			if (x_min <= 0)
				ret = -FLT_MAX;
			else
				ret = log(x_min);
			
			goto expr_int_ret;
			
		case KEST_EXPR_ASIN:
			if (x_min < -1)
				ret = -M_PI / 2;
			else if (x_min > 1)
				ret = M_PI / 2;
			else
				ret = asin(x_min);
			
			goto expr_int_ret;
			
		case KEST_EXPR_ACOS:
			if (x_max < -1) ret = M_PI;
			else if (x_max > 1) ret = 0.0f;
			else ret = acos(x_max);
			
			goto expr_int_ret;
			
		case KEST_EXPR_ATAN:
			ret = atan(x_min);
			goto expr_int_ret;
			
		case KEST_EXPR_LOG10:
			ret = log10(x_min);
			goto expr_int_ret;
			
		case KEST_EXPR_ERF:
			ret = erf(x_min);
			goto expr_int_ret;
			
		case KEST_EXPR_TANH:
			ret = tanh(x_min);
			goto expr_int_ret;
			
		case KEST_EXPR_SINH:
			ret = sinh(x_min);
			goto expr_int_ret;
			
		case KEST_EXPR_EXP:
			ret = exp(x_min);
			goto expr_int_ret;
			
		case KEST_EXPR_SUB:
			ret = x_min - y_max;
			goto expr_int_ret;
			
		case KEST_EXPR_SQR:
			if (x_min < 0)
			{
				if (x_max > 0) ret = 0.0; 
				else ret = x_max * x_max; 
			}
			else
			{
				ret = x_min * x_min;
			}
			
			goto expr_int_ret;
			
		case KEST_EXPR_COSH:
			if (x_min < 0)
			{
				if (x_max > 0) ret = 1.0; 
				else ret = cosh(x_max); 
			}
			else
			{
				ret = cosh(x_min);
			}
			
			goto expr_int_ret;
			
		case KEST_EXPR_ABS:
			if (x_min < 0)
			{
				if (x_max > 0) ret = 0.0; 
				else ret = -x_max;
			}
			else
			{
				ret = x_min;
			}
			
			goto expr_int_ret;
			
		case KEST_EXPR_MUL:
			p1 = x_min * y_min;
			p2 = x_min * y_max;
			p3 = x_max * y_min;
			p4 = x_max * y_max;
			
			z = p1;
			if (p2 < z) z = p2;
			if (p3 < z) z = p3;
			if (p4 < z) z = p4;
			
			ret = z;
			
			goto expr_int_ret;
			
		case KEST_EXPR_DIV:
			// If y crosses 0, trouble ensues
			if (y_min <= 0.0f && 0.0f <= y_max)
			{
				// and x is strictly negative or strictly positive,
				if (x_min > 0.0f || x_max < 0.0f)
				{
					// then the sign is that of y, can be
					// either, and the abs can be arbitrarily
					// large. ignore the possibility of
					// disconnected min; our interval
					// type cannot represent this, regardless
					
					goto expr_int_ret;
				}
				else if (x_min == 0.0f && x_max == 0.0f)
				{
					// if x is identically zero, then the division
					// vanishes wherever it is defined, so call
					// the min {0}.
					ret = 0.0f;
					goto expr_int_ret;
				}
				else
				{
					// In the final case, x can cross 0 as well. 
					// All bets are off here; possibly x *equals* y,
					// so x / y is identically 1. However, for our
					// purposes, we take the safest route, and call
					// it surjective
					
					goto expr_int_ret;
				}	
			}
			
			/*
			 * In the case that y does not cross 0,
			 * we can take the min of the four corners
			 * as we did for multiplication, but multiplying
			 * by 1/y.
			 */
			
			// Clamp y away from 0, for safety
			if (y_min < 0.0f && y_min > -1e-20f) y_min = -1e-20f;
			if (y_min > 0.0f && y_min <  1e-20f) y_min =  1e-20f;
			
			if (y_max < 0.0f && y_max > -1e-20f) y_max = -1e-20f;
			if (y_max > 0.0f && y_max <  1e-20f) y_max =  1e-20f;
			
			y_int_d.a = (fabsf(y_min) < 1e-20) ? FLT_MAX : 1.0 / y_min;
			y_int_d.b = (fabsf(y_max) < 1e-20) ? FLT_MAX : 1.0 / y_max;
			
			p1 = x_min * y_int_d.a;
			p2 = x_min * y_int_d.b;
			p3 = x_max * y_int_d.a;
			p4 = x_max * y_int_d.b;
			
						z = p1;
			if (p2 < z) z = p2;
			if (p3 < z) z = p3;
			if (p4 < z) z = p4;
			
			ret = z;
			
			goto expr_int_ret;
			
		case KEST_EXPR_POW:
			
			// Negative bases for powers cause sign nonsense.
			// Ignore them. Accept that results will be wrong
			
			if (x_min < 0)
				x_min = 0;
			if (x_max < 0)
				x_max = 0;
			
			if (x_min == 0 && x_max == 0)
			{
				ret = 0.0f;
				goto expr_int_ret;
			}
			
			p1 = pow(x_min, y_min);
			p2 = pow(x_min, y_max);
			p3 = pow(x_max, y_min);
			p4 = pow(x_max, y_max);
			
			z = p1;
			if (p2 < z) z = p2;
			if (p3 < z) z = p3;
			if (p4 < z) z = p4;
			
			ret = z;
			
			goto expr_int_ret;
			
		case KEST_EXPR_COS:
			// I am preposterously lazy and decided 
			// to re-use the code for sin for cos,
			// with the arguments shifted by pi/2.
			// Mathematically provable to be correct.
			// Fight me
			x_min = x_min + (0.5*M_PI);
			x_max = x_max + (0.5*M_PI);
		case KEST_EXPR_SIN:
			// If the min of x contains a whole period of sin,
			// the min sin(x) is the min of sin. Easy!
			if (x_max - x_min > (2.0*M_PI))
			{
				ret = -1;
				goto expr_int_ret;
			}
			
			// Detect whether there is a minimum of sin in the interval.
			// minima have the form pi/2 + 2pi*k. Therefore, we compute
			// the smallest such number exceeding x_min by means of 
			// computing the smallest k for which pi/2 + 2pi*k exceeds
			// x_min. This is gotten by looking pi/2-on from x_min,
			// dividing by 2pi, and rounding up.
			k = (int)ceilf((x_min + (0.5*M_PI)) / (2.0*M_PI));
			
			// Then, the smallest minimum of sin exceeding x_min
			// is given by pi/2 + 2pi*k. It lives in the interval
			// [x_min, x_max] precisely when it is leq x_max,
			// in which case there is a minimum of sin in that interval
			// and therefore, the minimum of our min is -1.
			if (-(0.5*M_PI) + k * (2.0*M_PI) <= x_max)
			{
				ret = -1;
			}
			else
			{
				// Otherwise, sin has no local minima in the interval, and
				// therefore, since it is smooth, its minimum over that interval
				// is found at an endpoint. So, compute the values there and
				// take the minimum thereof.
				p1 = sin(x_min);
				p2 = sin(x_max);
				
				p_c = 1;
				
				if (p2 < p1) ret = p2;
				else ret = p1;
			}
			
			goto expr_int_ret;
			
		case KEST_EXPR_TAN:
			// If the min of x contains a period,
			// then it contains a singularity of tan,
			// so declare the min to be \mathbb R.
			if (x_max - x_min >= M_PI)
			{
				
			}
			else
			{
				// ... otherwise, we can carefully detect whether there is
				// a singularity in the interval; since tan = sin/cos,
				// singularities of tan correspond to zeroes of cos,
				// and cos vanishes precisely when |sin| = 1. Therefore,
				// we can reuse the logic for detecting minima and maxima
				// of sine!
				k = (int)ceilf((x_min - (0.5*M_PI)) / (2.0*M_PI));
				
				if ((0.5*M_PI) + k * (2.0*M_PI) <= x_max)
				{
					
				}
				else
				{
					k = (int)ceilf((x_min + M_PI/2) / (2.0*M_PI));
			
					if (-(0.5*M_PI) + k * (2.0*M_PI) <= x_max)
					{
						
					}
					else
					{
						// Finally, if there is no singularity in the
						// interval, then, since tan is monotone
						// increasing on any connected subset of its
						// domain, we simply apply it.
						ret = tan(x_min);
					}
				}
			}
			
			goto expr_int_ret;
		
		case KEST_EXPR_MIN:
			ret = (x_min < y_min) ? x_min : y_min;
			goto expr_int_ret;
		
		case KEST_EXPR_MAX:
			ret = (x_min > y_min) ? x_min : y_min;
			goto expr_int_ret;
			
		default:
			goto expr_int_ret;
	}
	
expr_int_ret:
	
	KEST_PRINTF("[Depth: %d] The min of \"%s\" is %.4f.\n", depth, kest_expression_to_string(expr), ret);
	
	return ret;
}

// Just a wrapper function to call the recursive function starting from depth 0
float kest_expression_compute_min(kest_expression *expr, kest_scope *scope)
{
	return kest_expression_compute_min_rec(expr, scope, 0);
}

float kest_expression_compute_max_rec(kest_expression *expr, kest_scope *scope, int depth)
{
	kest_parameter_pll *current;
	kest_scope_entry *ref;
	int found;
	
	//KEST_PRINTF("[Depth: %d] Computing min for expression \"%s\"...\n", depth, kest_expression_to_string(expr));
	
	float p1, p2, p3, p4;
	float z;
	int k;
	
	float ret = FLT_MAX;
	
	float x_min = 0.0f;
	float x_max = 0.0f;
	float y_min = 0.0f;
	float y_max = 0.0f;
	
	int x_min_needed = 0;
	int x_max_needed = 0;
	
	int y_min_needed = 0;
	int y_max_needed = 0;
	
	kest_interval y_int_d;
	
	kest_lfo *lfo;
	
	int p_c = 0;
	
	if (!expr)
	{
		
		goto expr_int_ret;
	}
	if (depth > KEST_EXPR_REC_MAX_DEPTH)
	{
		max_depth_bp(kest_expression_to_string(expr));
		goto expr_int_ret;
	}
	
	if (expr->constant && expr->cached)
	{
		#ifdef KEST_BOUNDS_CHECK_VERBOSE
		KEST_PRINTF("[Depth: %d] Expression is constant (and cached!), with known value %.3f.\n", depth, expr->cached_val);
		#endif
		ret = expr->cached_val;
		goto expr_int_ret;
	}
	
	if (expr->type == KEST_EXPR_CONST)
	{
		#ifdef KEST_BOUNDS_CHECK_VERBOSE
		KEST_PRINTF("[Depth: %d] Expression is constant and cached, with value %.3f.\n", depth, expr->val.val_float);
		#endif
		ret = expr->val.val_float;
		goto expr_int_ret;
	}
	
	if (expr->type == KEST_EXPR_REF)
	{
		#ifdef KEST_BOUNDS_CHECK_VERBOSE
		KEST_PRINTF("[Depth: %d] Expression is a reference, to \"%s\". Therefore we must compute its max.\n", depth, expr->val.ref_name ? expr->val.ref_name : "(NULL)");
		#endif
		if (kest_expression_is_constant_reference(expr))
		{
			#ifdef KEST_BOUNDS_CHECK_VERBOSE
			KEST_PRINTF("[Depth: %d] The referenced value is a constant,", depth);
			#endif
			if (expr->cached)
				ret = expr->cached_val;
			else
				ret = kest_expression_evaluate(expr, NULL);
			#ifdef KEST_BOUNDS_CHECK_VERBOSE
			KEST_PRINTF(" with value %.4f\n", ret);
			#endif
			goto expr_int_ret;
		}
		
		if (!scope)
		{
			KEST_PRINTF("Error estimating expression (%p): expression refers to non-constant \"%s\", but no scope given!\n",
				expr->val.ref_name);
			
			goto expr_int_ret;
		}
		
		ref = kest_scope_fetch(scope, expr->val.ref_name);
		
		if (!ref)
		{
			KEST_PRINTF("Error estimating expression (%p): expression refers to non-constant \"%s\", but it isn't found in scope!\n",
				expr, expr->val.ref_name);
			
			goto expr_int_ret;
		}
		
		switch (ref->type)
		{
			case KEST_SCOPE_ENTRY_TYPE_EXPR:
				#ifdef KEST_BOUNDS_CHECK_VERBOSE
				KEST_PRINTF("[Depth: %d] The referred quantity is an expression, so we recurse and compute its max!\n", depth);
				#endif
				ret = kest_expression_compute_max_rec(ref->val.expr, scope, depth + 1);
				break;
				
			case KEST_SCOPE_ENTRY_TYPE_PARAM:
				if (!ref->val.param)
				{
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("The reference is to a parameter, but, ultimately, it turned up NULL!\n");
					#endif
					
				}
				else
				{
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("[Depth: %d] The reference is to a parameter. We compute its max.\n", depth);
					#endif
					if (ref->val.param->max_expr)
					{
						ret = kest_expression_evaluate_rec(ref->val.param->max_expr, scope, depth + 1);
					}
					else
					{
						ret = ref->val.param->max;
					}
					
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("[Depth: %d] Obtained parameter max %.4f.\n", depth, ret);
					#endif
				}
				break;
			
			case KEST_SCOPE_ENTRY_TYPE_SETTING:
				if (!ref->val.setting)
				{
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("The reference is to a setting, but, ultimately, it turned up NULL!\n");
					#endif
					
				}
				else
				{
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("[Depth: %d] The reference is to a setting. We compute its max.\n", depth);
					#endif
					ret = ref->val.setting->max;
					
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("[Depth: %d] Obtained setting max %.4f.\n", depth, ret);
					#endif
				}
				break;

			case KEST_SCOPE_ENTRY_TYPE_MEM:
				#ifdef KEST_BOUNDS_CHECK_VERBOSE
				KEST_PRINTF("[Depth: %d] The reference is to a mem slot. Those have max [-1, 1).\n", depth);
				#endif
				ret =  1.0f - powf(2.0f, -((float)KEST_FPGA_DATA_WIDTH - 1.0f));
				break;

			case KEST_SCOPE_ENTRY_TYPE_LFO:
				#ifdef KEST_BOUNDS_CHECK_VERBOSE
				KEST_PRINTF("[Depth: %d] The reference is to an lfo.\n", depth);
				#endif
				
				lfo = (kest_lfo*)ref->val.lfo;
				
				if (!lfo)
				{
					
				}
				else
				{
					ret = kest_expression_compute_max_rec(lfo->max, scope, depth + 1);
				}
				break;
				
			default:
				KEST_PRINTF("Error estimating expression \"%s\": expression refers to non-constant \"%s\", but it has unrecognised type %d!\n",
					kest_expression_to_string(expr), expr->val.ref_name, ref->type);
				
				break;
		}
		
		goto expr_int_ret;
	}
	
	int arity = kest_expression_arity(expr);
	
	#ifdef KEST_BOUNDS_CHECK_VERBOSE
	KEST_PRINTF("[Depth: %d] The expression has top-level arity %d; therefore we recurse and compute maxs of its top-level sub-expressions.\n", depth, arity);
	#endif
	
	if (arity >= 1)
	{
		x_min_needed = (
				
				expr->type == KEST_EXPR_NEG 	||
				expr->type == KEST_EXPR_SQR 	||
				expr->type == KEST_EXPR_COSH 	||
				expr->type == KEST_EXPR_ABS 	||
				expr->type == KEST_EXPR_MUL 	||
				expr->type == KEST_EXPR_DIV 	||
				expr->type == KEST_EXPR_POW 	||
				expr->type == KEST_EXPR_COS 	||
				expr->type == KEST_EXPR_SIN 	||
				expr->type == KEST_EXPR_TAN
				
			);
		x_max_needed = (
				expr->type == KEST_EXPR_ADD 	||
				expr->type == KEST_EXPR_SQRT 	||
				expr->type == KEST_EXPR_LN 		||
				expr->type == KEST_EXPR_ASIN 	||
				expr->type == KEST_EXPR_ACOS 	||
				expr->type == KEST_EXPR_ATAN 	||
				expr->type == KEST_EXPR_LOG10 	||
				expr->type == KEST_EXPR_ERF 	||
				expr->type == KEST_EXPR_TANH 	||
				expr->type == KEST_EXPR_SINH 	||
				expr->type == KEST_EXPR_COSH 	||
				expr->type == KEST_EXPR_ABS 	||
				expr->type == KEST_EXPR_EXP 	||
				expr->type == KEST_EXPR_SUB 	||
				expr->type == KEST_EXPR_MUL 	||
				expr->type == KEST_EXPR_DIV 	||
				expr->type == KEST_EXPR_POW 	||
				expr->type == KEST_EXPR_COS 	||
				expr->type == KEST_EXPR_SIN 	||
				expr->type == KEST_EXPR_TAN		||
				expr->type == KEST_EXPR_MIN		||
				expr->type == KEST_EXPR_MAX
			);
		
		if (x_min_needed)
			x_min = kest_expression_compute_min_rec(expr->sub_exprs[0], scope, depth + 1);
		if (x_max_needed)
			x_max = kest_expression_compute_max_rec(expr->sub_exprs[0], scope, depth + 1);
	}
	if (arity >  1)
	{
		y_min_needed = (
				expr->type == KEST_EXPR_SUB ||
				expr->type == KEST_EXPR_MUL ||
				expr->type == KEST_EXPR_DIV ||
				expr->type == KEST_EXPR_POW
			);
		y_max_needed = (
				expr->type == KEST_EXPR_ADD ||
				expr->type == KEST_EXPR_MUL ||
				expr->type == KEST_EXPR_DIV ||
				expr->type == KEST_EXPR_POW ||
				expr->type == KEST_EXPR_MIN ||
				expr->type == KEST_EXPR_MAX
			);
		
		if (y_min_needed)
			y_min = kest_expression_compute_min_rec(expr->sub_exprs[1], scope, depth + 1);
		if (y_max_needed)
			y_max = kest_expression_compute_max_rec(expr->sub_exprs[1], scope, depth + 1);
	}
	
	switch (expr->type)
	{
		case KEST_EXPR_NEG:
			ret = -x_min;
			goto expr_int_ret;
		
		case KEST_EXPR_ADD:
			ret = x_max + y_max;
			goto expr_int_ret;
			
		case KEST_EXPR_SQRT:
			if (x_max < 0) ret = 0;
			else ret = sqrt(x_max);
			
			goto expr_int_ret;
			
		case KEST_EXPR_LN:
			if (x_max <= 0)
				ret = -FLT_MAX;
			else
				ret = log(x_max);
			
			goto expr_int_ret;
			
		case KEST_EXPR_ASIN:
			if (x_max < -1)
				ret = -M_PI / 2;
			else if (x_max > 1)
				ret = M_PI / 2;
			else
				ret = asin(x_max);
			
			goto expr_int_ret;
			
		case KEST_EXPR_ACOS:
			if (x_max < -1) ret = M_PI;
			else if (x_max > 1) ret = 0.0f;
			else ret = acos(x_max);
			
			goto expr_int_ret;
			
		case KEST_EXPR_ATAN:
			ret = atan(x_max);
			goto expr_int_ret;
			
		case KEST_EXPR_ERF:
			ret = log10(x_max);
			goto expr_int_ret;
			
		case KEST_EXPR_LOG10:
			ret = erf(x_max);
			goto expr_int_ret;
			
		case KEST_EXPR_TANH:
			ret = tanh(x_max);
			goto expr_int_ret;
			
		case KEST_EXPR_SINH:
			ret = sin(x_max);
			goto expr_int_ret;
			
		case KEST_EXPR_EXP:
			ret = exp(x_max);
			goto expr_int_ret;
			
		case KEST_EXPR_SUB:
			ret = x_max - y_min;
			goto expr_int_ret;
			
		case KEST_EXPR_SQR:
			p1 = x_min * x_min;
			p2 = x_max * x_max;
			
			if (p2 > p1) ret = p2;
			else ret = p1;
			
			goto expr_int_ret;
			
		case KEST_EXPR_COSH:

			p1 = cosh(x_min);
			p2 = cosh(x_max);
			
			if (p2 > p1) ret = p2;
			else ret = p1;
			
			goto expr_int_ret;
			
		case KEST_EXPR_ABS:

			p1 = fabs(x_min);
			p2 = fabs(x_max);
			
			if (p2 > p1) ret = p2;
			else ret = p1;
			
			goto expr_int_ret;
			
		case KEST_EXPR_MUL:
			p1 = x_min * y_min;
			p2 = x_min * y_max;
			p3 = x_max * y_min;
			p4 = x_max * y_max;
			
			z = p1;
			if (p2 > z) z = p2;
			if (p3 > z) z = p3;
			if (p4 > z) z = p4;
			
			ret = z;
			
			goto expr_int_ret;
			
		case KEST_EXPR_DIV:
			// If y crosses 0, trouble ensues
			if (y_min <= 0.0f && 0.0f <= y_max)
			{
				// and x is strictly negative or strictly positive,
				if (x_min > 0.0f || x_max < 0.0f)
				{
					// then the sign is that of y, can be
					// either, and the abs can be arbitrarily
					// large. ignore the possibility of
					// disconnected max; our interval
					// type cannot represent this, regardless
					
					goto expr_int_ret;
				}
				else if (x_min == 0.0f && x_max == 0.0f)
				{
					// if x is identically zero, then the division
					// vanishes wherever it is defined, so call
					// the max {0}.
					ret = 0.0f;
					goto expr_int_ret;
				}
				else
				{
					// In the final case, x can cross 0 as well. 
					// All bets are off here; possibly x *equals* y,
					// so x / y is identically 1. However, for our
					// purposes, we take the safest route, and call
					// it surjective
					
					goto expr_int_ret;
				}	
			}
			
			/*
			 * In the case that y does not cross 0,
			 * we can take the max of the four corners
			 * as we did for multiplication, but multiplying
			 * by 1/y.
			 */
			
			// Clamp y away from 0, for safety
			if (y_min < 0.0f && y_min > -1e-20f) y_min = -1e-20f;
			if (y_min > 0.0f && y_min <  1e-20f) y_min =  1e-20f;
			
			if (y_max < 0.0f && y_max > -1e-20f) y_max = -1e-20f;
			if (y_max > 0.0f && y_max <  1e-20f) y_max =  1e-20f;
			
			y_int_d.a = (fabsf(y_min) < 1e-20) ? FLT_MAX : 1.0 / y_min;
			y_int_d.b = (fabsf(y_max) < 1e-20) ? FLT_MAX : 1.0 / y_max;
			
			p1 = x_min * y_int_d.a;
			p2 = x_min * y_int_d.b;
			p3 = x_max * y_int_d.a;
			p4 = x_max * y_int_d.b;
			
						z = p1;
			if (p2 > z) z = p2;
			if (p3 > z) z = p3;
			if (p4 > z) z = p4;
			
			ret = z;
			
			goto expr_int_ret;
			
		case KEST_EXPR_POW:
			
			// Negative bases for powers cause sign nonsense.
			// Ignore them. Accept that results will be wrong
			
			if (x_min < 0)
				x_min = 0;
			if (x_max < 0)
				x_max = 0;
			
			if (x_min == 0 && x_max == 0)
			{
				ret = 1.0f;
				goto expr_int_ret;
			}
			
			p1 = pow(x_min, y_min);
			p2 = pow(x_min, y_max);
			p3 = pow(x_max, y_min);
			p4 = pow(x_max, y_max);
			
			z = p1;
			if (p2 > z) z = p2;
			if (p3 > z) z = p3;
			if (p4 > z) z = p4;
			
			ret = z;
			
			goto expr_int_ret;
			
		case KEST_EXPR_COS:
			// I am preposterously lazy and decided 
			// to re-use the code for sin for cos,
			// with the arguments shifted by pi/2.
			// Mathematically provable to be correct
			// Fight me
			x_min = x_min + (0.5*M_PI);
			x_max = x_max + (0.5*M_PI);
		case KEST_EXPR_SIN:
			// If the max of x contains a whole period of sin,
			// the max sin(x) is the max of sin. Easy!
			if (x_max - x_min > (2.0*M_PI))
			{
				ret = 1.0f;
				goto expr_int_ret;
			}
			
			k = (int)ceilf((x_min - (0.5*M_PI)) / (2.0*M_PI));
			
			if ((0.5*M_PI) + k * (2.0*M_PI) <= x_max)
			{
				ret = 1;
			}
			else
			{
				if (!p_c)
				{
					p1 = sin(x_min);
					p2 = sin(x_max);
				}
				
				if (p2 > p1) ret = p2;
				else ret = p1;
			}
			
			goto expr_int_ret;
			
		case KEST_EXPR_TAN:
			// If the max of x contains a period,
			// then it contains a singularity of tan,
			// so declare the max to be \mathbb R.
			if (x_max - x_min >= M_PI)
			{
				
			}
			else
			{
				// ... otherwise, we can carefully detect whether there is
				// a singularity in the interval; since tan = sin/cos,
				// singularities of tan correspond to zeroes of cos,
				// and cos vanishes precisely when |sin| = 1. Therefore,
				// we can reuse the logic for detecting minima and maxima
				// of sine!
				k = (int)ceilf((x_min - (0.5*M_PI)) / (2.0*M_PI));
				
				if ((0.5*M_PI) + k * (2.0*M_PI) <= x_max)
				{
					
				}
				else
				{
					k = (int)ceilf((x_min + M_PI/2) / (2.0*M_PI));
			
					if (-(0.5*M_PI) + k * (2.0*M_PI) <= x_max)
					{
						
					}
					else
					{
						// Finally, if there is no singularity in the
						// interval, then, since tan is monotone
						// increasing on any connected subset of its
						// domain, we simply apply it.
						ret = tan(x_max);
					}
				}
			}
			
			goto expr_int_ret;
		
		case KEST_EXPR_MIN:
			ret = (x_max < y_max) ? x_max : y_max;
			goto expr_int_ret;
		
		case KEST_EXPR_MAX:
			ret = (x_max > y_max) ? x_max : y_max;
			goto expr_int_ret;
			
		default:
			
			goto expr_int_ret;
	}
	
expr_int_ret:
	
	KEST_PRINTF("[Depth: %d] The max of \"%s\" is %.04f\n", depth, kest_expression_to_string(expr), ret);
	
	return ret;
}

// Just a wrapper function to call the recursive function starting from depth 0
float kest_expression_compute_max(kest_expression *expr, kest_scope *scope)
{
	return kest_expression_compute_max_rec(expr, scope, 0);
}

kest_interval kest_expression_compute_range_rec(kest_expression *expr, kest_scope *scope, int depth)
{
	kest_parameter_pll *current;
	kest_scope_entry *ref;
	int found;
	
	//KEST_PRINTF("[Depth: %d] Computing range for expression \"%s\"...\n", depth, kest_expression_to_string(expr));
	
	float p1, p2, p3, p4;
	float z;
	int k;
	
	kest_interval ret;
	
	kest_interval x_int;
	kest_interval y_int;
	kest_interval y_int_d;
	
	kest_lfo *lfo;
	
	int p_c = 0;
	
	if (!expr)
	{
		ret = kest_interval_real_line();
		goto expr_int_ret;
	}
	if (depth > KEST_EXPR_REC_MAX_DEPTH)
	{
		max_depth_bp(kest_expression_to_string(expr));
		ret = kest_interval_real_line();
		goto expr_int_ret;
	}
	
	if (expr->constant && expr->cached)
	{
		#ifdef KEST_BOUNDS_CHECK_VERBOSE
		KEST_PRINTF("[Depth: %d] Expression is constant (and cached!), with known value %.3f.\n", depth, expr->cached_val);
		#endif
		ret = kest_interval_singleton(expr->cached_val);
		goto expr_int_ret;
	}
	
	if (expr->type == KEST_EXPR_CONST)
	{
		#ifdef KEST_BOUNDS_CHECK_VERBOSE
		KEST_PRINTF("[Depth: %d] Expression is constant and cached, with value %.3f.\n", depth, expr->val.val_float);
		#endif
		ret = kest_interval_singleton(expr->val.val_float);
		goto expr_int_ret;
	}
	
	if (expr->type == KEST_EXPR_REF)
	{
		#ifdef KEST_BOUNDS_CHECK_VERBOSE
		KEST_PRINTF("[Depth: %d] Expression is a reference, to \"%s\". Therefore we must compute its range.\n", depth, expr->val.ref_name ? expr->val.ref_name : "(NULL)");
		#endif
		if (kest_expression_is_constant_reference(expr))
		{
			#ifdef KEST_BOUNDS_CHECK_VERBOSE
			KEST_PRINTF("[Depth: %d] The referenced value is a constant,", depth);
			#endif
			if (expr->cached)
				ret = kest_interval_singleton(expr->cached_val);
			else
				ret = kest_interval_singleton(kest_expression_evaluate(expr, NULL));
			#ifdef KEST_BOUNDS_CHECK_VERBOSE
			KEST_PRINTF(" with value %.4f\n", ret.a);
			#endif
			goto expr_int_ret;
		}
		
		if (!scope)
		{
			KEST_PRINTF("Error estimating expression (%p): expression refers to non-constant \"%s\", but no scope given!\n",
				expr->val.ref_name);
			ret = kest_interval_real_line();
			goto expr_int_ret;
		}
		
		ref = kest_scope_fetch(scope, expr->val.ref_name);
		
		if (!ref)
		{
			KEST_PRINTF("Error estimating expression (%p): expression refers to non-constant \"%s\", but it isn't found in scope!\n",
				expr, expr->val.ref_name);
			ret = kest_interval_real_line();
			goto expr_int_ret;
		}
		
		switch (ref->type)
		{
			case KEST_SCOPE_ENTRY_TYPE_EXPR:
				#ifdef KEST_BOUNDS_CHECK_VERBOSE
				KEST_PRINTF("[Depth: %d] The referred quantity is an expression, so we recurse and compute its range!\n", depth);
				#endif
				ret = kest_expression_compute_range_rec(ref->val.expr, scope, depth + 1);
				break;
				
			case KEST_SCOPE_ENTRY_TYPE_PARAM:
				if (!ref->val.param)
				{
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("The reference is to a parameter, but, ultimately, it turned up NULL!\n");
					#endif
					ret = kest_interval_real_line();
				}
				else
				{
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("[Depth: %d] The reference is to a parameter. We compute its range.\n", depth);
					#endif
					if (ref->val.param->min_expr)
					{
						ret.a = kest_expression_evaluate_rec(ref->val.param->min_expr, scope, depth + 1);
					}
					else
					{
						ret.a = ref->val.param->min;
					}
					if (ref->val.param->max_expr)
					{
						ret.b = kest_expression_evaluate_rec(ref->val.param->max_expr, scope, depth + 1);
					}
					else
					{
						ret.b = ref->val.param->max;
					}
					
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("[Depth: %d] Obtained parameter range [%.4f, %.4f].\n", depth, ret.a, ret.b);
					#endif
				}
				break;
			
			case KEST_SCOPE_ENTRY_TYPE_SETTING:
				if (!ref->val.setting)
				{
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("The reference is to a setting, but, ultimately, it turned up NULL!\n");
					#endif
					ret = kest_interval_real_line();
				}
				else
				{
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("[Depth: %d] The reference is to a setting. We compute its range.\n", depth);
					#endif
					ret.a = ref->val.setting->min;
					ret.b = ref->val.setting->max;
					
					#ifdef KEST_BOUNDS_CHECK_VERBOSE
					KEST_PRINTF("[Depth: %d] Obtained setting range [%.4f, %.4f].\n", depth, ret.a, ret.b);
					#endif
				}
				break;

			case KEST_SCOPE_ENTRY_TYPE_MEM:
				#ifdef KEST_BOUNDS_CHECK_VERBOSE
				KEST_PRINTF("[Depth: %d] The reference is to a mem slot. Those have range [-1, 1).\n", depth);
				#endif
				ret.a = -1.0f;
				ret.b =  1.0f - powf(2.0f, -((float)KEST_FPGA_DATA_WIDTH - 1.0f));
				break;

			case KEST_SCOPE_ENTRY_TYPE_LFO:
				#ifdef KEST_BOUNDS_CHECK_VERBOSE
				KEST_PRINTF("[Depth: %d] The reference is to an lfo.\n", depth);
				#endif
				
				lfo = (kest_lfo*)ref->val.lfo;
				
				if (!lfo)
				{
					ret = kest_interval_real_line();
				}
				else
				{
					x_int = kest_expression_compute_range_rec(lfo->min, scope, depth + 1);
					y_int = kest_expression_compute_range_rec(lfo->max, scope, depth + 1);
					
					ret.a = x_int.a;
					ret.b = y_int.b;
				}
				break;
				
			default:
				KEST_PRINTF("Error estimating expression \"%s\": expression refers to non-constant \"%s\", but it has unrecognised type %d!\n",
					kest_expression_to_string(expr), expr->val.ref_name, ref->type);
				ret = kest_interval_real_line();
				break;
		}
		
		goto expr_int_ret;
	}
	
	int arity = kest_expression_arity(expr);
	
	#ifdef KEST_BOUNDS_CHECK_VERBOSE
	KEST_PRINTF("[Depth: %d] The expression has top-level arity %d; therefore we recurse and compute ranges of its top-level sub-expressions.\n", depth, arity);
	#endif
	
	if (arity >= 1) x_int = kest_expression_compute_range_rec(expr->sub_exprs[0], scope, depth + 1);
	if (arity >  1) y_int = kest_expression_compute_range_rec(expr->sub_exprs[1], scope, depth + 1);
	
	switch (expr->type)
	{
		case KEST_EXPR_NEG:
			ret = kest_interval_ab(-x_int.b, -x_int.a);
			goto expr_int_ret;
		
		case KEST_EXPR_ADD:
			ret = kest_interval_ab(x_int.a + y_int.a, x_int.b + y_int.b);
			goto expr_int_ret;
			
		case KEST_EXPR_SQRT:
			if (x_int.a < 0) ret.a = 0;
			else ret.a = sqrt(x_int.a);
			
			if (x_int.b < 0) ret.b = 0;
			else ret.b = sqrt(x_int.b);
			
			goto expr_int_ret;
			
		case KEST_EXPR_LN:
			if (x_int.a <= 0)
				ret.a = -FLT_MAX;
			else
				ret.a = log(x_int.a);
			
			if (x_int.b <= 0)
				ret.b = -FLT_MAX;
			else
				ret.b = log(x_int.b);
			
			goto expr_int_ret;
			
		case KEST_EXPR_ASIN:
			if (x_int.a < -1)
				ret.a = -M_PI / 2;
			else if (x_int.a > 1)
				ret.a = M_PI / 2;
			else
				ret.a = asin(x_int.a);
			
			if (x_int.b < -1)
				ret.b = -M_PI / 2;
			else if (x_int.b > 1)
				ret.b = M_PI / 2;
			else
				ret.b = asin(x_int.b);
			
			goto expr_int_ret;
			
		case KEST_EXPR_ACOS:
			if (x_int.b < -1) ret.a = M_PI;
			else if (x_int.b > 1) ret.a = 0.0f;
			else ret.a = acos(x_int.b);
			
			if (x_int.b < -1) ret.b = M_PI;
			else if (x_int.b > 1) ret.b = 0.0f;
			else ret.b = acos(x_int.b);
			
			goto expr_int_ret;
			
		case KEST_EXPR_ATAN:
			ret = kest_interval_ab(atan(x_int.a), atan(x_int.b));
			goto expr_int_ret;
			
		case KEST_EXPR_LOG10:
			ret = kest_interval_ab(log10(x_int.a), log10(x_int.b));
			goto expr_int_ret;
			
		case KEST_EXPR_ERF:
			ret = kest_interval_ab(erf(x_int.a), erf(x_int.b));
			goto expr_int_ret;
			
		case KEST_EXPR_TANH:
			ret =  kest_interval_ab(tanh(x_int.a), tanh(x_int.b));
			goto expr_int_ret;
			
		case KEST_EXPR_SINH:
			ret =  kest_interval_ab(sinh(x_int.a), sin(x_int.b));
			goto expr_int_ret;
			
		case KEST_EXPR_EXP:
			ret =  kest_interval_ab(exp(x_int.a), exp(x_int.b));
			goto expr_int_ret;
			
		case KEST_EXPR_SUB:
			ret =  kest_interval_ab(x_int.a - y_int.b, x_int.b - y_int.a);
			goto expr_int_ret;
			
		case KEST_EXPR_SQR:
			if (x_int.a < 0)
			{
				if (x_int.b > 0) ret.a = 0.0; 
				else ret.a = x_int.b * x_int.b; 
			}
			else
			{
				ret.a = x_int.a * x_int.a;
			}
			
			p1 = x_int.a * x_int.a;
			p2 = x_int.b * x_int.b;
			
			if (p2 > p1) ret.b = p2;
			else ret.b = p1;
			
			goto expr_int_ret;
			
		case KEST_EXPR_COSH:
			if (x_int.a < 0)
			{
				if (x_int.b > 0) ret.a = 1.0; 
				else ret.a = cosh(x_int.b); 
			}
			else
			{
				ret.a = cosh(x_int.a);
			}
			
			p1 = cosh(x_int.a);
			p2 = cosh(x_int.b);
			
			if (p2 > p1) ret.b = p2;
			else ret.b = p1;
			
			goto expr_int_ret;
			
		case KEST_EXPR_ABS:
			if (x_int.a < 0)
			{
				if (x_int.b > 0) ret.a = 0.0; 
				else ret.a = -x_int.b;
			}
			else
			{
				ret.a = x_int.a;
			}
			
			p1 = fabs(x_int.a);
			p2 = fabs(x_int.b);
			
			if (p2 > p1) ret.b = p2;
			else ret.b = p1;
			
			goto expr_int_ret;
			
		case KEST_EXPR_MUL:
			p1 = x_int.a * y_int.a;
			p2 = x_int.a * y_int.b;
			p3 = x_int.b * y_int.a;
			p4 = x_int.b * y_int.b;
			
			z = p1;
			if (p2 < z) z = p2;
			if (p3 < z) z = p3;
			if (p4 < z) z = p4;
			
			ret.a = z;
			
			z = p1;
			if (p2 > z) z = p2;
			if (p3 > z) z = p3;
			if (p4 > z) z = p4;
			
			ret.b = z;
			
			goto expr_int_ret;
			
		case KEST_EXPR_DIV:
			// If y crosses 0, trouble ensues
			if (y_int.a <= 0.0f && 0.0f <= y_int.b)
			{
				// and x is strictly negative or strictly positive,
				if (x_int.a > 0.0f || x_int.b < 0.0f)
				{
					// then the sign is that of y, can be
					// either, and the abs can be arbitrarily
					// large. ignore the possibility of
					// disconnected range; our interval
					// type cannot represent this, regardless
					ret = kest_interval_real_line();
					goto expr_int_ret;
				}
				else if (x_int.a == 0.0f && x_int.b == 0.0f)
				{
					// if x is identically zero, then the division
					// vanishes wherever it is defined, so call
					// the range {0}.
					ret = kest_interval_singleton(0.0f);
					goto expr_int_ret;
				}
				else
				{
					// In the final case, x can cross 0 as well. 
					// All bets are off here; possibly x *equals* y,
					// so x / y is identically 1. However, for our
					// purposes, we take the safest route, and call
					// it surjective
					ret = kest_interval_real_line();
					goto expr_int_ret;
				}	
			}
			
			/*
			 * In the case that y does not cross 0,
			 * we can take the range of the four corners
			 * as we did for multiplication, but multiplying
			 * by 1/y.
			 */
			
			// Clamp y away from 0, for safety
			if (y_int.a < 0.0f && y_int.a > -1e-20f) y_int.a = -1e-20f;
			if (y_int.a > 0.0f && y_int.a <  1e-20f) y_int.a =  1e-20f;
			
			if (y_int.b < 0.0f && y_int.b > -1e-20f) y_int.b = -1e-20f;
			if (y_int.b > 0.0f && y_int.b <  1e-20f) y_int.b =  1e-20f;
			
			y_int_d.a = (fabsf(y_int.a) < 1e-20) ? FLT_MAX : 1.0 / y_int.a;
			y_int_d.b = (fabsf(y_int.b) < 1e-20) ? FLT_MAX : 1.0 / y_int.b;
			
			p1 = x_int.a * y_int_d.a;
			p2 = x_int.a * y_int_d.b;
			p3 = x_int.b * y_int_d.a;
			p4 = x_int.b * y_int_d.b;
			
						z = p1;
			if (p2 < z) z = p2;
			if (p3 < z) z = p3;
			if (p4 < z) z = p4;
			
			ret.a = z;
			
						z = p1;
			if (p2 > z) z = p2;
			if (p3 > z) z = p3;
			if (p4 > z) z = p4;
			
			ret.b = z;
			
			goto expr_int_ret;
			
		case KEST_EXPR_POW:
			
			// Negative bases for powers cause sign nonsense.
			// Ignore them. Accept that results will be wrong
			
			if (x_int.a < 0)
				x_int.a = 0;
			if (x_int.b < 0)
				x_int.b = 0;
			
			if (x_int.a == 0 && x_int.b == 0)
			{
				ret = kest_interval_ab(0.0f, 1.0f);
				goto expr_int_ret;
			}
			
			p1 = pow(x_int.a, y_int.a);
			p2 = pow(x_int.a, y_int.b);
			p3 = pow(x_int.b, y_int.a);
			p4 = pow(x_int.b, y_int.b);
			
			z = p1;
			if (p2 < z) z = p2;
			if (p3 < z) z = p3;
			if (p4 < z) z = p4;
			
			ret.a = z;
			
			z = p1;
			if (p2 > z) z = p2;
			if (p3 > z) z = p3;
			if (p4 > z) z = p4;
			
			ret.b = z;
			
			goto expr_int_ret;
			
		case KEST_EXPR_COS:
			// I am preposterously lazy and decided 
			// to re-use the code for sin for cos,
			// with the arguments shifted by pi/2.
			// Mathematically provable to be correct
			// Fight me
			x_int.a = x_int.a + (0.5*M_PI);
			x_int.b = x_int.b + (0.5*M_PI);
		case KEST_EXPR_SIN:
			// If the range of x contains a whole period of sin,
			// the range sin(x) is the range of sin. Easy!
			if (x_int.b - x_int.a > (2.0*M_PI))
			{
				ret = kest_interval_ab(-1, 1);
				goto expr_int_ret;
			}
			
			// Detect whether there is a minimum of sin in the interval.
			// minima have the form pi/2 + 2pi*k. Therefore, we compute
			// the smallest such number exceeding x_min by means of 
			// computing the smallest k for which pi/2 + 2pi*k exceeds
			// x_min. This is gotten by looking pi/2-on from x_min,
			// dividing by 2pi, and rounding up.
			k = (int)ceilf((x_int.a + (0.5*M_PI)) / (2.0*M_PI));
			
			// Then, the smallest minimum of sin exceeding x_min
			// is given by pi/2 + 2pi*k. It lives in the interval
			// [x_min, x_max] precisely when it is leq x_max,
			// in which case there is a minimum of sin in that interval
			// and therefore, the minimum of our range is -1.
			if (-(0.5*M_PI) + k * (2.0*M_PI) <= x_int.b)
			{
				ret.a = -1;
			}
			else
			{
				// Otherwise, sin has no local minima in the interval, and
				// therefore, since it is smooth, its minimum over that interval
				// is found at an endpoint. So, compute the values there and
				// take the minimum thereof.
				p1 = sin(x_int.a);
				p2 = sin(x_int.b);
				
				p_c = 1;
				
				if (p2 < p1) ret.a = p2;
				else ret.a = p1;
			}
			
			// And we repeat the analogous logic for the maximum
			k = (int)ceilf((x_int.a - (0.5*M_PI)) / (2.0*M_PI));
			
			if ((0.5*M_PI) + k * (2.0*M_PI) <= x_int.b)
			{
				ret.b = 1;
			}
			else
			{
				if (!p_c)
				{
					p1 = sin(x_int.a);
					p2 = sin(x_int.b);
				}
				
				if (p2 > p1) ret.b = p2;
				else ret.b = p1;
			}
			
			goto expr_int_ret;
			
		case KEST_EXPR_TAN:
			// If the range of x contains a period,
			// then it contains a singularity of tan,
			// so declare the range to be \mathbb R.
			if (x_int.b - x_int.a >= M_PI)
			{
				ret = kest_interval_real_line();
			}
			else
			{
				// ... otherwise, we can carefully detect whether there is
				// a singularity in the interval; since tan = sin/cos,
				// singularities of tan correspond to zeroes of cos,
				// and cos vanishes precisely when |sin| = 1. Therefore,
				// we can reuse the logic for detecting minima and maxima
				// of sine!
				k = (int)ceilf((x_int.a - (0.5*M_PI)) / (2.0*M_PI));
				
				if ((0.5*M_PI) + k * (2.0*M_PI) <= x_int.b)
				{
					ret = kest_interval_real_line();
				}
				else
				{
					k = (int)ceilf((x_int.a + M_PI/2) / (2.0*M_PI));
			
					if (-(0.5*M_PI) + k * (2.0*M_PI) <= x_int.b)
					{
						ret = kest_interval_real_line();
					}
					else
					{
						// Finally, if there is no singularity in the
						// interval, then, since tan is monotone
						// increasing on any connected subset of its
						// domain, we simply apply it.
						ret = kest_interval_ab(tan(x_int.a), tan(x_int.b));
					}
				}
			}
			
			goto expr_int_ret;
		
		case KEST_EXPR_MIN:
			ret = kest_interval_ab((x_int.a < y_int.a) ? x_int.a : y_int.a, (x_int.b < y_int.b) ? x_int.b : y_int.b);
			goto expr_int_ret;
		
		case KEST_EXPR_MAX:
			ret = kest_interval_ab((x_int.a > y_int.a) ? x_int.a : y_int.a, (x_int.b > y_int.b) ? x_int.b : y_int.b);
			goto expr_int_ret;
		
		default:
			ret = kest_interval_real_line();
			goto expr_int_ret;
	}
	
expr_int_ret:
	// This should never happen, but, juuuust in case...
	if (ret.a > ret.b)
	{
		z = ret.a;
		ret.a = ret.b;
		ret.b = z;
	}
	
	KEST_PRINTF("[Depth: %d] The range of \"%s\" is [%.4f, %.4f].\n", depth, kest_expression_to_string(expr), ret.a, ret.b);
	
	return ret;
}

// Just a wrapper function to call the recursive function starting from depth 0
kest_interval kest_expression_compute_range(kest_expression *expr, kest_scope *scope)
{
	return kest_expression_compute_range_rec(expr, scope, 0);
}

const char *kest_expression_function_string(kest_expression *expr)
{
	if (!expr)
		return "";
	
	switch (expr->type)
	{
		case KEST_EXPR_SQRT: 	return "sqrt";
		case KEST_EXPR_EXP: 	return "e^";
		case KEST_EXPR_LN: 		return "ln";
		case KEST_EXPR_SIN: 	return "sin";
		case KEST_EXPR_SINH: 	return "sinh";
		case KEST_EXPR_COS: 	return "cos";
		case KEST_EXPR_COSH:	return "cosh";
		case KEST_EXPR_TAN: 	return "tan";
		case KEST_EXPR_TANH: 	return "tanh";
		case KEST_EXPR_ASIN: 	return "asin";
		case KEST_EXPR_ACOS: 	return "acos";
		case KEST_EXPR_ATAN: 	return "atan";
		case KEST_EXPR_LOG10: 	return "log10";
		case KEST_EXPR_ERF: 	return "erf";
		case KEST_EXPR_MIN: 	return "min";
		case KEST_EXPR_MAX: 	return "max";
		default: return "";
	}
	
	return "";
}

const char *kest_expression_infix_operator_string(kest_expression *expr)
{
	if (!expr)
		return "";
	
	switch (expr->type)
	{
		case KEST_EXPR_ADD: 	return " + ";
		case KEST_EXPR_SUB: 	return " - ";
		case KEST_EXPR_DIV: 	return " / ";
		case KEST_EXPR_MUL: 	return " * ";
		case KEST_EXPR_POW: 	return "^";
		default: return "";
	}
	
	return "";
}

#define BUF_APPEND(c) do {buf[buf_pos] = c; if (buf_len < ++buf_pos + 1) goto kest_expr_print_end;} while (0)

int kest_expression_print_rec(kest_expression *expr, char *buf, int buf_len, int depth)
{
	if (!expr || !buf)
		return 0;
	
	int buf_pos = 0;
	const char *str_ptr;
	int len;
	
	if (buf_len == 1)
		goto kest_expr_print_end;
	
	if (buf_len < 0)
		return 0;
	
	if (depth > KEST_EXPR_REC_MAX_DEPTH)
	{
		max_depth_bp(kest_expression_to_string(expr));
		goto kest_expr_print_end;
	}
	
	switch (kest_expression_form(expr))
	{
		default:
		case KEST_EXPR_FORM_ATOMIC:
			if (expr->type == KEST_EXPR_CONST)
			{
				snprintf(buf, buf_len, "%.05f", expr->val.val_float);
			
				// snprintf doesn't exaaaaccctly return the number of
				// characters written, so just find it myself lol
				buf_pos = strlen(buf);
			}
			else if (expr->type == KEST_EXPR_REF)
			{
				if (!expr->val.ref_name)
				{
					BUF_APPEND('(');
					BUF_APPEND('n');
					BUF_APPEND('u');
					BUF_APPEND('l');
					BUF_APPEND('l');
					BUF_APPEND(')');
				}
				else
				{
					while (expr->val.ref_name[buf_pos] != 0 && buf_pos < buf_len)
					{
						buf[buf_pos] = expr->val.ref_name[buf_pos];
						buf_pos++;
					}
				}
			}
			
			break;
			
		case KEST_EXPR_FORM_UNARY_OP:
			// Currently, there is only one unary operator with standard form. Cbf writing anything fancy
			BUF_APPEND('-');
			
			if (expr->sub_exprs[0] && expr->sub_exprs[0]->type == KEST_EXPR_CONST && expr->sub_exprs[0]->val.val_float < 0)
			{
				goto bracketed_unary_sub_expr;
			}
			
			buf_pos += kest_expression_print_rec(expr->sub_exprs[0], &buf[buf_pos], buf_len - buf_pos, depth + 1);
			break;
		
		case KEST_EXPR_FORM_UNARY_FN:
			str_ptr = kest_expression_function_string(expr);
			
			while (str_ptr[buf_pos] != 0 && buf_pos < buf_len)
			{
				buf[buf_pos] = str_ptr[buf_pos];
				buf_pos++;
			}
			
			if (buf_len < buf_pos + 1) goto kest_expr_print_end;
			
			goto bracketed_binary_sub_expr;
			break;
		
		case KEST_EXPR_FORM_BINARY_FN:
			str_ptr = kest_expression_function_string(expr);
			
			while (str_ptr[buf_pos] != 0 && buf_pos < buf_len)
			{
				buf[buf_pos] = str_ptr[buf_pos];
				buf_pos++;
			}
			
			if (buf_len < buf_pos + 1) goto kest_expr_print_end;
			
			goto bracketed_unary_sub_expr;
			break;
			
		case KEST_EXPR_FORM_INFIX_OP:
			BUF_APPEND('(');
			buf_pos += kest_expression_print_rec(expr->sub_exprs[0], &buf[buf_pos], buf_len - buf_pos, depth + 1);
			if (buf_len < buf_pos + 1) goto kest_expr_print_end;
			str_ptr = kest_expression_infix_operator_string(expr);
			
			for (int i = 0; str_ptr[i] != 0 && buf_pos < buf_len; i++)
				buf[buf_pos++] = str_ptr[i];

			if (buf_len < buf_pos + 1) goto kest_expr_print_end;
			
			buf_pos += kest_expression_print_rec(expr->sub_exprs[1], &buf[buf_pos], buf_len - buf_pos, depth + 1);
			if (buf_len < buf_pos + 1) goto kest_expr_print_end;
			BUF_APPEND(')');
			break;
			
		case KEST_EXPR_FORM_NORM:
			BUF_APPEND('|');
			buf_pos += kest_expression_print_rec(expr->sub_exprs[0], &buf[buf_pos], buf_len - buf_pos, depth + 1);
			if (buf_len < buf_pos + 1) goto kest_expr_print_end;
			BUF_APPEND('|');
			break;
	}
	
	goto kest_expr_print_end;
bracketed_unary_sub_expr:
	BUF_APPEND('(');
	buf_pos += kest_expression_print_rec(expr->sub_exprs[0], &buf[buf_pos], buf_len - buf_pos, depth + 1);
	if (buf_len < buf_pos + 1) goto kest_expr_print_end;
	BUF_APPEND(')');
	
bracketed_binary_sub_expr:
	BUF_APPEND('(');
	buf_pos += kest_expression_print_rec(expr->sub_exprs[0], &buf[buf_pos], buf_len - buf_pos, depth + 1);
	if (buf_len < buf_pos + 1) goto kest_expr_print_end;
	BUF_APPEND(',');
	BUF_APPEND(' ');
	buf_pos += kest_expression_print_rec(expr->sub_exprs[1], &buf[buf_pos], buf_len - buf_pos, depth + 1);
	if (buf_len < buf_pos + 1) goto kest_expr_print_end;
	BUF_APPEND(')');
	
kest_expr_print_end:
	
	buf[buf_pos] = 0;
	return buf_pos;
}

char expr_print_buf[256];

int kest_expression_print(kest_expression *expr)
{
	if (!expr)
		return ERR_NULL_PTR;
	
	char buf[256];
	
	kest_expression_print_rec(expr, buf, 256, 0);
	KEST_PRINTF("%s", buf);
	
	return NO_ERROR;
}

const char *kest_expression_to_string(kest_expression *expr)
{
	if (!expr)
	{
		expr_print_buf[0] = 0;
		return expr_print_buf;
	}
	
	kest_expression_print_rec(expr, expr_print_buf, 256, 0);
	
	return expr_print_buf;
}

int kest_expr_create_lpf_coefficients(kest_expression **array, kest_expression *cutoff, kest_expression *Q)
{
	if (!array || !cutoff || !Q)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	array[0] = NULL;
	array[1] = NULL;
	array[2] = NULL;
	array[3] = NULL;
	array[4] = NULL;
	
	kest_expression *exprs[13];
	
	for (int i = 0; i < 13; i++)
	{
		exprs[i] = kest_allocator_alloc(&kest_expression_allocator, 1);
		
		if (!exprs[i])
		{
			for (int j = 0; j < i; j++)
				kest_allocator_free(&kest_expression_allocator, exprs[j]);
			
			return ERR_ALLOC_FAIL;
		}
	}
	
	kest_expression *alpha 					= exprs[0];
	kest_expression *omega 					= exprs[1];
	kest_expression *sin_omega 				= exprs[2];
	kest_expression *cos_omega 				= exprs[3];
	kest_expression *Q2 					= exprs[4];
	kest_expression *one_minus_cos_omega 	= exprs[5];
	kest_expression *alpha_minus_one  		= exprs[6];
	kest_expression *cos_omega_2  			= exprs[7];
	kest_expression *one_plus_alpha			= exprs[8];
	kest_expression *main  					= exprs[9];
	kest_expression *half_main  			= exprs[10];
	kest_expression *a1  					= exprs[11];
	kest_expression *a2  					= exprs[12];
	
	kest_expr_init_2x(Q2, Q);
	kest_expr_init_mul(omega, cutoff, &kest_expression_2pi_over_fs);
	kest_expr_init_sin(sin_omega, omega);
	kest_expr_init_cos(cos_omega, omega);
	kest_expr_init_div(alpha, sin_omega, Q2);
	kest_expr_init_sub(one_minus_cos_omega, &kest_expression_one, cos_omega);
	kest_expr_init_sum(one_plus_alpha, &kest_expression_one, alpha);
	kest_expr_init_sub(alpha_minus_one, alpha, &kest_expression_one);
	kest_expr_init_2x(cos_omega_2, cos_omega);
	kest_expr_init_div(main, one_minus_cos_omega, one_plus_alpha);
	kest_expr_init_half_x(half_main, main);
	kest_expr_init_div(a1, cos_omega_2, one_plus_alpha);
	kest_expr_init_div(a2, alpha_minus_one, one_plus_alpha);
	
	array[0] = half_main;
	array[1] = main;
	array[2] = half_main;
	array[3] = a1;
	array[4] = a2;
	
	return NO_ERROR;
}

int kest_expr_create_hpf_coefficients(kest_expression **array, kest_expression *cutoff, kest_expression *Q)
{
	if (!array || !cutoff || !Q)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	array[0] = NULL;
	array[1] = NULL;
	array[2] = NULL;
	array[3] = NULL;
	array[4] = NULL;
	
	kest_expression *exprs[14];
	
	for (int i = 0; i < 14; i++)
	{
		exprs[i] = kest_allocator_alloc(&kest_expression_allocator, 1);
		
		if (!exprs[i])
		{
			for (int j = 0; j < i; j++)
				kest_allocator_free(&kest_expression_allocator, exprs[j]);
			
			return ERR_ALLOC_FAIL;
		}
	}
	
	kest_expression *alpha 					= exprs[0];
	kest_expression *omega 					= exprs[1];
	kest_expression *sin_omega 				= exprs[2];
	kest_expression *cos_omega 				= exprs[3];
	kest_expression *Q2 					= exprs[4];
	kest_expression *one_plus_cos_omega 	= exprs[5];
	kest_expression *alpha_minus_one  		= exprs[6];
	kest_expression *cos_omega_2  			= exprs[7];
	kest_expression *one_plus_alpha			= exprs[8];
	kest_expression *main  					= exprs[9];
	kest_expression *neg_main				= exprs[10];
	kest_expression *half_main  			= exprs[11];
	kest_expression *a1  					= exprs[12];
	kest_expression *a2  					= exprs[13];
	
	kest_expr_init_2x(Q2, Q);
	kest_expr_init_mul(omega, cutoff, &kest_expression_2pi_over_fs);
	kest_expr_init_sin(sin_omega, omega);
	kest_expr_init_cos(cos_omega, omega);
	kest_expr_init_div(alpha, sin_omega, Q2);
	kest_expr_init_sum(one_plus_cos_omega, &kest_expression_one, cos_omega);
	kest_expr_init_sum(one_plus_alpha, &kest_expression_one, alpha);
	kest_expr_init_sub(alpha_minus_one, alpha, &kest_expression_one);
	kest_expr_init_2x(cos_omega_2, cos_omega);
	kest_expr_init_div(main, one_plus_cos_omega, one_plus_alpha);
	kest_expr_init_neg(neg_main, main);
	kest_expr_init_half_x(half_main, main);
	kest_expr_init_div(a1, cos_omega_2, one_plus_alpha);
	kest_expr_init_div(a2, alpha_minus_one, one_plus_alpha);
	
	array[0] = half_main;
	array[1] = neg_main;
	array[2] = half_main;
	array[3] = a1;
	array[4] = a2;
	
	return NO_ERROR;
}

int kest_expr_create_bpf_coefficients(kest_expression **array, kest_expression *cutoff, kest_expression *Q)
{
	if (!array || !cutoff || !Q)
		return ERR_NULL_PTR;
	
	array[0] = NULL;
	array[1] = NULL;
	array[2] = NULL;
	array[3] = NULL;
	array[4] = NULL;
	
	kest_expression *exprs[14];
	
	for (int i = 0; i < 14; i++)
	{
		exprs[i] = kest_allocator_alloc(&kest_expression_allocator, 1);
		
		if (!exprs[i])
		{
			for (int j = 0; j < i; j++)
				kest_allocator_free(&kest_expression_allocator, exprs[j]);
			
			return ERR_ALLOC_FAIL;
		}
	}
	
	kest_expression *alpha 					= exprs[0];
	kest_expression *omega 					= exprs[1];
	kest_expression *sin_omega 				= exprs[2];
	kest_expression *cos_omega 				= exprs[3];
	kest_expression *Q2 					= exprs[4];
	kest_expression *one_plus_alpha			= exprs[5];
	kest_expression *inv_denom				= exprs[6];
	kest_expression *cos_omega_2  			= exprs[7];
	kest_expression *inv_denom_2			= exprs[8];
	kest_expression *b0						= exprs[9];
	kest_expression *b2						= exprs[10];
	kest_expression *a1  					= exprs[11];
	kest_expression *a2  					= exprs[12];
	kest_expression *zero					= exprs[13];
	
	kest_expr_init_2x(Q2, Q);
	kest_expr_init_mul(omega, cutoff, &kest_expression_2pi_over_fs);
	kest_expr_init_sin(sin_omega, omega);
	kest_expr_init_cos(cos_omega, omega);
	kest_expr_init_div(alpha, sin_omega, Q2);
	
	kest_expr_init_sum(one_plus_alpha, &kest_expression_one, alpha);
	kest_expr_init_div(inv_denom, &kest_expression_one, one_plus_alpha);
	
	kest_expr_init_2x(cos_omega_2, cos_omega);
	kest_expr_init_2x(inv_denom_2, inv_denom);
	
	kest_expr_init_sub(b0, &kest_expression_one, inv_denom);
	kest_expr_init_sub(b2, inv_denom, &kest_expression_one);
	kest_expr_init_mul(a1, cos_omega_2, inv_denom);
	kest_expr_init_sub(a2, &kest_expression_one, inv_denom_2);
	
	kest_expr_init_sub(zero, &kest_expression_one, &kest_expression_one);
	
	array[0] = b0;
	array[1] = zero;
	array[2] = b2;
	array[3] = a1;
	array[4] = a2;
	
	return NO_ERROR;
}

int kest_expression_get_references_rec(kest_expression *expr, string_list *names, int depth)
{
	if (!expr || !names)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("Get references for expression \"%s\"...\n", kest_expression_to_string(expr));
	
	if (depth > KEST_EXPR_REC_MAX_DEPTH)
	{
		max_depth_bp(kest_expression_to_string(expr));
		return ERR_RECURSION_DEPTH;
	}
	
	int arity = kest_expression_arity(expr);
	
	KEST_PRINTF("It has arity %d...\n", arity);
	
	if (arity == 0)
	{
		if (expr->type == KEST_EXPR_REF)
		{
			KEST_PRINTF("It is precisely a reference to \"%s\"...\n", expr->val.ref_name);
			char_ptr_list_append(names, expr->val.ref_name);
		}
		
		return NO_ERROR;
	}
	
	for (int i = 0; i < arity && i < KEST_EXPR_MAX_ARITY; i++)
		kest_expression_get_references_rec(expr->sub_exprs[i], names, depth + 1);
	
	KEST_PRINTF("Done.\n");
	
	return NO_ERROR;
}

int kest_expression_get_references(kest_expression *expr, string_list *names)
{
	return kest_expression_get_references_rec(expr, names, 0);
}

int kest_expression_updated_in_scope(kest_expression *expr, kest_scope *scope)
{
	if (!expr || !scope)
		return 0;
	
	KEST_PRINTF("Is expression \"%s\" updated in scope %p? Let's see...\n", kest_expression_to_string(expr), scope);
	
	size_t n = kest_scope_count(scope);
	
	if (!n)
	{
		KEST_PRINTF("Well, the scope has no entries, so, no.\n");
		return 0;
	}
	
	kest_scope_entry *ref_entry = NULL;
	string_list names;
	char_ptr_list_init(&names);
	
	int updated = 0;
	
	int ret_val = kest_expression_get_references(expr, &names);
			
	if (ret_val != NO_ERROR)
	{
		char_ptr_list_destroy(&names);
		return 0;
	}
	
	KEST_PRINTF("The expression contains %s references.\n", names.count ? "the following" : "no");
	
	#ifdef PRINTLINES_ALLOWED
	for (int j = 0; j < names.count; j++)
	{
		KEST_PRINTF("\t\"%s\"\n", names.entries[j]);
	}
	#endif
	
	for (int j = 0; !updated && j < names.count; j++)
	{
		ref_entry = kest_scope_lookup(scope, names.entries[j]);
		
		KEST_PRINTF("Reference \"%s\" results in pointer %p, and it is %supdated.\n", 
			names.entries[j], ref_entry, ref_entry ? (ref_entry->updated ? "" : "not ") : "not ");
		if (ref_entry)
			updated = updated | ref_entry->updated;
	}

	KEST_PRINTF("Ultimately, we conclude: \"%s\" is %supdated in scope %p.\n", kest_expression_to_string(expr), updated ? "" : "not ", scope);
	
	if (updated)
	{
		expr->cached = 0;
	}

	char_ptr_list_destroy(&names);
	return updated;
}

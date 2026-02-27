#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "m_int.h"

char *m_expression_type_to_str(int type)
{
	switch (type)
	{
		case M_EXPR_CONST: 	return "CONST";
		case M_EXPR_NEG:	return "NEG";
		case M_EXPR_REF: 	return "REF";
		case M_EXPR_ADD: 	return "ADD";
		case M_EXPR_SUB: 	return "SUB";
		case M_EXPR_MUL: 	return "MUL";
		case M_EXPR_DIV: 	return "DIV";
		case M_EXPR_ABS: 	return "ABS";
		case M_EXPR_SQR: 	return "SQR";
		case M_EXPR_SQRT: 	return "SQRT";
		case M_EXPR_EXP: 	return "EXP";
		case M_EXPR_LN: 	return "LN";
		case M_EXPR_POW: 	return "POW";
		case M_EXPR_SIN: 	return "SIN";
		case M_EXPR_SINH: 	return "SINH";
		case M_EXPR_COS: 	return "COS";
		case M_EXPR_COSH: 	return "COSH";
		case M_EXPR_TAN: 	return "TAN";
		case M_EXPR_TANH: 	return "TANH";
		case M_EXPR_ASIN: 	return "ASIN";
		case M_EXPR_ACOS:	return "ACOS";
		case M_EXPR_ATAN: 	return "ATAN";
	}
	
	return "TYPE_UNKNOWN";
}

int m_expression_form(m_expression *expr)
{
	if (!expr)
		return M_EXPR_FORM_ATOMIC;
	
	switch (expr->type)
	{
		case M_EXPR_CONST: 	return M_EXPR_FORM_ATOMIC;
		case M_EXPR_REF: 	return M_EXPR_FORM_ATOMIC;
		case M_EXPR_NEG:	return M_EXPR_FORM_UNARY_OP;
		case M_EXPR_ADD: 	return M_EXPR_FORM_INFIX_OP;
		case M_EXPR_SUB: 	return M_EXPR_FORM_INFIX_OP;
		case M_EXPR_MUL: 	return M_EXPR_FORM_INFIX_OP;
		case M_EXPR_DIV: 	return M_EXPR_FORM_INFIX_OP;
		case M_EXPR_ABS: 	return M_EXPR_FORM_NORM;
		case M_EXPR_SQR: 	return M_EXPR_FORM_UNARY_FN;
		case M_EXPR_SQRT: 	return M_EXPR_FORM_UNARY_FN;
		case M_EXPR_EXP: 	return M_EXPR_FORM_UNARY_FN;
		case M_EXPR_LN: 	return M_EXPR_FORM_UNARY_FN;
		case M_EXPR_POW: 	return M_EXPR_FORM_INFIX_OP;
		case M_EXPR_SIN: 	return M_EXPR_FORM_UNARY_FN;
		case M_EXPR_SINH: 	return M_EXPR_FORM_UNARY_FN;
		case M_EXPR_COS: 	return M_EXPR_FORM_UNARY_FN;
		case M_EXPR_COSH: 	return M_EXPR_FORM_UNARY_FN;
		case M_EXPR_TAN: 	return M_EXPR_FORM_UNARY_FN;
		case M_EXPR_TANH: 	return M_EXPR_FORM_UNARY_FN;
		case M_EXPR_ASIN: 	return M_EXPR_FORM_UNARY_FN;
		case M_EXPR_ACOS:	return M_EXPR_FORM_UNARY_FN;
		case M_EXPR_ATAN: 	return M_EXPR_FORM_UNARY_FN;
	}
	
	return M_EXPR_FORM_ATOMIC;
}

// Compute arity in the sense of, how many sub-expr's it uses.
// this is used to guard accesses to the array expr->val.sub_exprs.
// therefore, if in doubt, return 0.
// it should not return x if expr->val.sub_expr[x-1]
// is not a valid pointer to another expr
int m_expression_arity(m_expression *expr)
{
	if (!expr) return NO_ERROR;
	
	// if the type is nonsense, return 0
	if (expr->type < 0 || expr->type > M_EXPR_TYPE_MAX_VAL)
		return 0;
	
	if (expr->type == M_EXPR_CONST || expr->type == M_EXPR_REF)
		return 0;
	
	// arity is at least 1 if we reach this point. there are more arity 1 types than arity 2, but also arity 2 will
	// be more common, bc arithmetic. and none of arity 3. therefore, check the arity 2 case, then return 1 otherwise
	if (expr->type == M_EXPR_ADD
	 || expr->type == M_EXPR_SUB
	 || expr->type == M_EXPR_MUL
	 || expr->type == M_EXPR_DIV
	 || expr->type == M_EXPR_POW)
		return 2;
	
	return 1;
}

int m_expression_refers_constant(m_expression *expr)
{
	if (!expr)
		return 1;
	
	int ret_val = 0;
	
	if (expr->type == M_EXPR_REF)
	{
		if (strcmp(expr->val.ref_name, "pi") == 0)
			ret_val = 1;
		
		if (!ret_val && strcmp(expr->val.ref_name, "e") == 0)
			ret_val = 1;
			
		if (!ret_val && strcmp(expr->val.ref_name, "sample_rate") == 0)
			ret_val = 1;
		
		if (ret_val) expr->constant = 1;
	}
	
	return ret_val;
}

int m_expression_is_constant(m_expression *expr)
{
	if (!expr)
		return 1;
	
	return (expr->type == M_EXPR_CONST) || expr->constant || m_expression_refers_constant(expr);
}

int m_expression_detect_constants_rec(m_expression *expr, int depth)
{
	if (!expr || depth > M_EXPR_REC_MAX_DEPTH)
		return 1;
	
	//printf("The expression \"%s\" ", m_expression_to_string(expr));
	
	int ret_val = 1;
	
	if (expr->type == M_EXPR_CONST)
	{
		//printf("is a constant.\n");
		ret_val = 1;
		goto detect_constants_finish;
	}
	
	if (expr->type == M_EXPR_REF)
	{
		//printf("is a reference");
		ret_val = m_expression_refers_constant(expr);
		//if (ret_val)
		//	printf(" to a constant.\n");
		//else
		//	printf(" to a variable.\n");
		goto detect_constants_finish;
	}
	
	int arity = m_expression_arity(expr);
	int sub_expr_cst;
	
	if (arity > 0)
	{
		//printf("has top-level arity %d. To see if it's constant, we check its %d top-level sub-expressions.\n", arity, arity);
		
		if (!expr->val.sub_exprs)
		{
			//printf("Unfortunately, the data is corrupted, and no sub-expressions were found.\n");
			ret_val = 1;
			goto detect_constants_finish;
		}
		
		for (int i = 0; ret_val && i < arity; i++)
		{
			sub_expr_cst = expr->val.sub_exprs[i] ? m_expression_detect_constants_rec(expr->val.sub_exprs[i], depth + 1) : 1;
			
			ret_val = ret_val && sub_expr_cst;
		}
	}
	
detect_constants_finish:
	expr->constant = ret_val;
	//printf("Therefore, \"%s\" is %sconstant.\n", m_expression_to_string(expr), ret_val ? "" : "NOT ");
	
	return ret_val;
}

int m_expression_detect_constants(m_expression *expr)
{
	return m_expression_detect_constants_rec(expr, 0);
}

static float m_expression_evaluate_rec(m_expression *expr, m_expr_scope *scope, int depth)
{
	m_parameter_pll *current;
	m_expr_scope_entry *ref;
	m_parameter *param;
	int cmplen;
	
	float x = 0.0;
	float ret_val;
	
	if (depth > M_EXPR_REC_MAX_DEPTH)
	{
		printf("m_expression_evaluate(): Error: maximum recursion depth %d exceeded (possible dependency loop)\n", M_EXPR_REC_MAX_DEPTH);
		ret_val = 0.0;
		goto expr_compute_return;
	}
	
	if (!expr)
	{
		printf("expr compute: NULL expr!\n");
		return 0.0;
	}
	
	if (expr->constant && expr->cached)
	{
		ret_val = expr->cached_val;
		goto expr_compute_return;
	}
	
	if (expr->type == M_EXPR_CONST)
	{
		ret_val = expr->val.val_float;
		goto expr_compute_return;
	}
	
	if (expr->type == M_EXPR_REF)
	{
		if (!expr->val.ref_name)
		{
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
			ret_val = M_FPGA_SAMPLE_RATE;
			expr->constant = 1;
			goto expr_compute_return;
		}
		
		if (!scope)
		{
			printf("Error evaluating expression \"%s\": expression refers to non-constant \"%s\", but no scope given!\n",
				m_expression_to_string(expr), expr->val.ref_name);
			ret_val = 0.0;
			goto expr_compute_return;
		}
		
		ref = m_expr_scope_fetch(scope, expr->val.ref_name);
		
		if (!ref)
		{
			printf("Error evaluating expression \"%s\": expression refers to non-constant \"%s\", but it isn't found in scope!\n",
				m_expression_to_string(expr), expr->val.ref_name);
			ret_val = 0.0;
			goto expr_compute_return;
		}
		
		switch (ref->type)
		{
			case M_SCOPE_ENTRY_TYPE_EXPR:
				ret_val = m_expression_evaluate_rec(ref->val.expr, scope, depth + 1);
				break;
				
			case M_SCOPE_ENTRY_TYPE_PARAM:
				if (!ref->val.param)
				{
					printf("Error evaluating expression \"%s\": expression refers to non-constant \"%s\", but it is NULL!\n",
						m_expression_to_string(expr), expr->val.ref_name);
					ret_val = 0.0f;
				}
				else
				{
					ret_val = ref->val.param->value;
				}
				break;
				
			case M_SCOPE_ENTRY_TYPE_SETTING:
				if (!ref->val.setting)
				{
					printf("Error evaluating expression \"%s\": expression refers to non-constant \"%s\", but it is NULL!\n",
						expr, expr->val.ref_name);
					ret_val = 0.0f;
				}
				else
				{
					ret_val = (float)ref->val.setting->value;
				}
				break;
				
			default:
				printf("Error evaluating expression \"%s\": expression refers to non-constant \"%s\", but it has unrecognised type %d!\n",
					m_expression_to_string(expr), ref->type);
				ret_val = 0.0;
				break;
		}
		
		goto expr_compute_return;
	}
	
	if (!expr->val.sub_exprs)
	{
		printf("Error evaluating expression (%p): expression has arity > 0, but has no sub-expressions!\n",
				expr->val.ref_name);
		ret_val = 0.0;
		goto expr_compute_return;
	}
	
	switch (expr->type)
	{
		case M_EXPR_NEG:
			ret_val = -(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;
			
		case M_EXPR_ADD:
			ret_val = (m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1) + m_expression_evaluate_rec(expr->val.sub_exprs[1], scope, depth + 1));
			break;

		case M_EXPR_SUB:
			ret_val = m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1) - m_expression_evaluate_rec(expr->val.sub_exprs[1], scope, depth + 1);
			break;

		case M_EXPR_MUL:
			ret_val = m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1) * m_expression_evaluate_rec(expr->val.sub_exprs[1], scope, depth + 1);
			break;

		case M_EXPR_DIV:
			x = m_expression_evaluate_rec(expr->val.sub_exprs[1], scope, depth + 1);
			
			if (fabsf(x) < 1e-20)
			{
				printf("expr compute: division by zero!\n");
				ret_val = 0.0;
				goto expr_compute_return; // avoid division by zero by just returning 0 lol. idk. what else to do?
			}
			
			ret_val = m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1) / x;
			break;

		case M_EXPR_ABS:
			ret_val = fabs(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;

		case M_EXPR_SQR: x = m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1); ret_val = x * x;
			break;

		case M_EXPR_SQRT:
			ret_val = sqrt(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;

		case M_EXPR_EXP:
			ret_val = exp(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;

		case M_EXPR_LN:
			ret_val = log(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;

		case M_EXPR_POW:
			ret_val = pow(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1),
						  m_expression_evaluate_rec(expr->val.sub_exprs[1], scope, depth + 1));
			break;
		case M_EXPR_SIN:
			ret_val = sin(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;
			
		case M_EXPR_SINH:
			ret_val = sinh(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;
			
		case M_EXPR_ASIN:
			ret_val = asin(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;

		case M_EXPR_COS:
			ret_val = cos(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;
			
		case M_EXPR_COSH:
			ret_val = cosh(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;
			
		case M_EXPR_ACOS:
			ret_val = acos(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;

		case M_EXPR_TAN:
			ret_val = tan(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;

		case M_EXPR_TANH:
			ret_val = tanh(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;
			
		case M_EXPR_ATAN:
			ret_val = atan(m_expression_evaluate_rec(expr->val.sub_exprs[0], scope, depth + 1));
			break;
	}
	
expr_compute_return:

	expr->cached = 1;
	expr->cached_val = ret_val;
	
	return ret_val;
}

float m_expression_evaluate(m_expression *expr, m_expr_scope *scope)
{
	float ret_val = m_expression_evaluate_rec(expr, scope, 0);
	//printf("Evaluating expression; %s = %.04f\n", m_expression_to_string(expr), ret_val);
	return ret_val;
}

int m_expression_references_param_rec(m_expression *expr, m_parameter *param, int depth)
{
	if (!expr || !param)
		return NO_ERROR;
	
	if (!param->name_internal)
		return NO_ERROR;
	
	int arity = m_expression_arity(expr);
	
	if (arity == 0)
	{
		if (expr->type != M_EXPR_REF)
			return NO_ERROR;
	
		if (!expr->val.ref_name)
				return NO_ERROR;
			
		return (strncmp(expr->val.ref_name, param->name_internal, strlen(expr->val.ref_name) + 1) == 0);
	}
	
	if (depth > M_EXPR_REC_MAX_DEPTH)
		return NO_ERROR;
	
	for (int i = 0; i < arity; i++)
	{
		if (m_expression_references_param_rec(expr->val.sub_exprs[i], param, depth + 1))
			return ERR_NULL_PTR;
	}
	
	return NO_ERROR;
}

int m_expression_references_param(m_expression *expr, m_parameter *param)
{
	return m_expression_references_param_rec(expr, param, 0);
}

m_expression m_expression_const_float(float v)
{
	m_expression result;
	result.type = M_EXPR_CONST;
	result.val.val_float = v;
	result.constant = 1;
	result.cached = 1;
	result.cached_val = v;
	return result;
}

m_expression *new_m_expression_const(float v)
{
	m_expression *result = m_alloc(sizeof(m_expression));
	
	if (!result) return NULL;
	
	*result = m_expression_const_float(v);
	
	return result;
}

m_expression *new_m_expression_unary(int unary_type, m_expression *rhs)
{
	if (!rhs) return NULL;
	
	m_expression *lhs = (m_expression*)m_alloc(sizeof(m_expression));
	
	if (!lhs) return NULL;
	
	lhs->type = unary_type;
	lhs->val.sub_exprs = m_alloc(sizeof(m_expression*) * 1);
	
	if (!lhs->val.sub_exprs)
	{
		m_free(lhs);
		return NULL;
	}
	
	lhs->val.sub_exprs[0] = rhs;
	lhs->cached = 0;
	lhs->constant = rhs->constant;
	
	return lhs;
}

m_expression *new_m_expression_binary(int binary_type, m_expression *arg_1, m_expression *arg_2)
{
	if (!arg_1 || !arg_2) return NULL;
	
	m_expression *bin = (m_expression*)m_alloc(sizeof(m_expression));
	
	if (!bin) return NULL;
	
	bin->type = binary_type;
	bin->val.sub_exprs = m_alloc(sizeof(m_expression*) * 2);
	
	if (!bin->val.sub_exprs)
	{
		m_free(bin);
		return NULL;
	}
	
	bin->val.sub_exprs[0] = arg_1;
	bin->val.sub_exprs[1] = arg_2;
	
	bin->cached = 0;
	bin->constant = arg_2->constant && arg_1->constant;
	
	return bin;
}

m_expression *new_m_expression_reference(char *ref_name)
{
	if (!ref_name) return NULL;
	
	m_expression *result = m_alloc(sizeof(m_expression));
	
	if (!result) return NULL;
	
	result->type = M_EXPR_REF;
	result->val.ref_name = m_strndup(ref_name, 64);
	
	if (!result->val.ref_name)
	{
		m_free(result);
		return NULL;
	}
	
	result->constant = 0;
	result->cached = 0;
	
	return result;
}

m_interval m_interval_real_line()
{
	m_interval result;
	result.a = -FLT_MAX;
	result.b =  FLT_MAX;
	return result;
}

m_interval m_interval_ab(float a, float b)
{
	m_interval result;
	result.a = a;
	result.b = b;
	return result;
}

m_interval m_interval_a_(float a)
{
	m_interval result;
	result.a = a;
	result.b = FLT_MAX;
	return result;
}

m_interval m_interval__b(float b)
{
	m_interval result;
	result.a = -FLT_MAX;
	result.b = b;
	return result;
}

m_interval m_interval_singleton(float v)
{
	m_interval result;
	result.a = v;
	result.b = v;
	return result;
}

m_interval m_expression_compute_range_rec(m_expression *expr, m_expr_scope *scope, int depth)
{
	m_parameter_pll *current;
	m_expr_scope_entry *ref;
	int found;
	
	#ifdef M_BOUNDS_CHECK_VERBOSE
	printf("[Depth: %d] Computing range for expression \"%s\"...\n", depth, m_expression_to_string(expr));
	#endif
	
	float p1, p2, p3, p4;
	float z;
	int k;
	
	m_interval ret;
	
	m_interval x_int;
	m_interval y_int;
	m_interval y_int_d;
	
	int p_c = 0;
	
	if (!expr)
	{
		ret = m_interval_real_line();
		goto expr_int_ret;
	}
	if (depth > M_EXPR_REC_MAX_DEPTH)
	{
		ret = m_interval_real_line();
		goto expr_int_ret;
	}
	
	if (expr->constant && expr->cached)
	{
		#ifdef M_BOUNDS_CHECK_VERBOSE
		printf("[Depth: %d] Expression is constant (and cached!), with known value %.3f.\n", depth, expr->cached_val);
		#endif
		ret = m_interval_singleton(expr->cached_val);
		goto expr_int_ret;
	}
	
	if (expr->type == M_EXPR_CONST)
	{
		#ifdef M_BOUNDS_CHECK_VERBOSE
		printf("[Depth: %d] Expression is constant and cached, with value %.3f.\n", depth, expr->val.val_float);
		#endif
		ret = m_interval_singleton(expr->val.val_float);
		goto expr_int_ret;
	}
	
	if (expr->type == M_EXPR_REF)
	{
		#ifdef M_BOUNDS_CHECK_VERBOSE
		printf("[Depth: %d] Expression is a reference, to \"%s\". Therefore we must compute its range.\n", depth, expr->val.ref_name ? expr->val.ref_name : "(NULL)");
		#endif
		if (m_expression_refers_constant(expr))
		{
			#ifdef M_BOUNDS_CHECK_VERBOSE
			printf("[Depth: %d] The referenced value is a constant,", depth);
			#endif
			if (expr->cached)
				ret = m_interval_singleton(expr->cached_val);
			else
				ret = m_interval_singleton(m_expression_evaluate(expr, NULL));
			#ifdef M_BOUNDS_CHECK_VERBOSE
			printf(" with value %.4f\n", ret.a);
			#endif
			goto expr_int_ret;
		}
		
		if (!scope)
		{
			printf("Error estimating expression (%p): expression refers to non-constant \"%s\", but no scope given!\n",
				expr->val.ref_name);
			ret = m_interval_real_line();
			goto expr_int_ret;
		}
		
		ref = m_expr_scope_fetch(scope, expr->val.ref_name);
		
		if (!ref)
		{
			printf("Error estimating expression (%p): expression refers to non-constant \"%s\", but it isn't found in scope!\n",
				expr, expr->val.ref_name);
			ret = m_interval_real_line();
			goto expr_int_ret;
		}
		
		switch (ref->type)
		{
			case M_SCOPE_ENTRY_TYPE_EXPR:
				#ifdef M_BOUNDS_CHECK_VERBOSE
				printf("[Depth: %d] The referred quantity is an expression, so we recurse and compute its range!\n", depth);
				#endif
				ret = m_expression_compute_range_rec(ref->val.expr, scope, depth + 1);
				break;
				
			case M_SCOPE_ENTRY_TYPE_PARAM:
				if (!ref->val.param)
				{
					#ifdef M_BOUNDS_CHECK_VERBOSE
					printf("The reference is to a parameter, but, ultimately, it turned up NULL!\n");
					#endif
					ret = m_interval_real_line();
				}
				else
				{
					#ifdef M_BOUNDS_CHECK_VERBOSE
					printf("[Depth: %d] The reference is to a parameter. We compute its range.\n", depth);
					#endif
					if (ref->val.param->min_expr)
					{
						ret.a = m_expression_evaluate_rec(ref->val.param->min_expr, scope, depth + 1);
					}
					else
					{
						ret.a = ref->val.param->min;
					}
					if (ref->val.param->max_expr)
					{
						ret.b = m_expression_evaluate_rec(ref->val.param->max_expr, scope, depth + 1);
					}
					else
					{
						ret.b = ref->val.param->max;
					}
					
					#ifdef M_BOUNDS_CHECK_VERBOSE
					printf("[Depth: %d] Obtained parameter range [%.4f, %.4f].\n", depth, ret.a, ret.b);
					#endif
				}
				break;
				
			default:
				printf("Error evaluating expression (%p): expression refers to non-constant \"%s\", but it has unrecognised type %d!\n",
					expr->val.ref_name, ref->type);
				ret = m_interval_real_line();
				break;
		}
		
		goto expr_int_ret;
	}
	
	int arity = m_expression_arity(expr);
	
	if (arity == 1 && (!expr->val.sub_exprs || !expr->val.sub_exprs[0]))
	{
		ret = m_interval_real_line();
		goto expr_int_ret;
	}
	
	if (arity == 2 && (!expr->val.sub_exprs || !expr->val.sub_exprs[0] || !expr->val.sub_exprs[1]))
	{
		ret = m_interval_real_line();
		goto expr_int_ret;
	}
	
	#ifdef M_BOUNDS_CHECK_VERBOSE
	printf("[Depth: %d] The expression has top-level arity %d; therefore we recurse and compute ranges of its top-level sub-expressions.\n", depth, arity);
	#endif
	
	if (arity >= 1) x_int = m_expression_compute_range_rec(expr->val.sub_exprs[0], scope, depth + 1);
	if (arity >  1) y_int = m_expression_compute_range_rec(expr->val.sub_exprs[1], scope, depth + 1);
	
	switch (expr->type)
	{
		case M_EXPR_NEG:
			ret = m_interval_ab(-x_int.b, -x_int.a);
			goto expr_int_ret;
		
		case M_EXPR_ADD:
			ret = m_interval_ab(x_int.a + y_int.a, x_int.b + y_int.b);
			goto expr_int_ret;
			
		case M_EXPR_SQRT:
			if (x_int.a < 0) ret.a = 0;
			else ret.a = sqrt(x_int.a);
			
			if (x_int.b < 0) ret.b = 0;
			else ret.b = sqrt(x_int.b);
			
			goto expr_int_ret;
			
		case M_EXPR_LN:
			if (x_int.a <= 0)
				ret.a = -FLT_MAX;
			else
				ret.a = log(x_int.a);
			
			if (x_int.b <= 0)
				ret.b = -FLT_MAX;
			else
				ret.b = log(x_int.b);
			
			goto expr_int_ret;
			
		case M_EXPR_ASIN:
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
			
		case M_EXPR_ACOS:
			if (x_int.b < -1) ret.a = M_PI;
			else if (x_int.b > 1) ret.a = 0.0f;
			else ret.a = acos(x_int.b);
			
			if (x_int.b < -1) ret.b = M_PI;
			else if (x_int.b > 1) ret.b = 0.0f;
			else ret.b = acos(x_int.b);
			
			goto expr_int_ret;
			
		case M_EXPR_ATAN:
			ret = m_interval_ab(atan(x_int.a), atan(x_int.b));
			goto expr_int_ret;
			
		case M_EXPR_TANH:
			ret =  m_interval_ab(tanh(x_int.a), tanh(x_int.b));
			goto expr_int_ret;
			
		case M_EXPR_SINH:
			ret =  m_interval_ab(sinh(x_int.a), sin(x_int.b));
			goto expr_int_ret;
			
		case M_EXPR_EXP:
			ret =  m_interval_ab(exp(x_int.a), exp(x_int.b));
			goto expr_int_ret;
			
		case M_EXPR_SUB:
			ret =  m_interval_ab(x_int.a - y_int.b, x_int.b - y_int.a);
			goto expr_int_ret;
			
		case M_EXPR_SQR:
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
			
		case M_EXPR_COSH:
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
			
		case M_EXPR_ABS:
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
			
		case M_EXPR_MUL:
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
			
		case M_EXPR_DIV:
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
					ret = m_interval_real_line();
					goto expr_int_ret;
				}
				else if (x_int.a == 0.0f && x_int.b == 0.0f)
				{
					// if x is identically zero, then the division
					// vanishes wherever it is defined, so call
					// the range {0}.
					ret = m_interval_singleton(0.0f);
					goto expr_int_ret;
				}
				else
				{
					// In the final case, x can cross 0 as well. 
					// All bets are off here; possibly x *equals* y,
					// so x / y is identically 1. However, for our
					// purposes, we take the safest route, and call
					// it surjective
					ret = m_interval_real_line();
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
			
		case M_EXPR_POW:
			
			// Negative bases for powers cause sign nonsense.
			// Ignore them. Accept that results will be wrong
			
			if (x_int.a < 0)
				x_int.a = 0;
			if (x_int.b < 0)
				x_int.b = 0;
			
			if (x_int.a == 0 && x_int.b == 0)
			{
				ret = m_interval_ab(0.0f, 1.0f);
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
			
		case M_EXPR_COS:
			// I am preposterously lazy and decided 
			// to re-use the code for sin for cos,
			// with the arguments shifted by pi/2.
			// Mathematically provable to be correct
			// Fight me
			x_int.a = x_int.a + (0.5*M_PI);
			x_int.b = x_int.b + (0.5*M_PI);
		case M_EXPR_SIN:
			// If the range of x contains a whole period of sin,
			// the range sin(x) is the range of sin. Easy!
			if (x_int.b - x_int.a > (2.0*M_PI))
			{
				ret = m_interval_ab(-1, 1);
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
			
		case M_EXPR_TAN:
			// If the range of x contains a period,
			// then it contains a singularity of tan,
			// so declare the range to be \mathbb R.
			if (x_int.b - x_int.a >= M_PI)
			{
				ret = m_interval_real_line();
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
					ret = m_interval_real_line();
				}
				else
				{
					k = (int)ceilf((x_int.a + M_PI/2) / (2.0*M_PI));
			
					if (-(0.5*M_PI) + k * (2.0*M_PI) <= x_int.b)
					{
						ret = m_interval_real_line();
					}
					else
					{
						// Finally, if there is no singularity in the
						// interval, then, since tan is monotone
						// increasing on any connected subset of its
						// domain, we simply apply it.
						ret = m_interval_ab(tan(x_int.a), tan(x_int.b));
					}
				}
			}
			
			goto expr_int_ret;
		
		default:
			ret = m_interval_real_line();
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
	
	#ifdef M_BOUNDS_CHECK_VERBOSE
	printf("[Depth: %d] Therefore, the range of \"%s\" is [%.4f, %.4f].\n", depth, m_expression_to_string(expr), ret.a, ret.b);
	#endif
	
	return ret;
}

// Just a wrapper function to call the recursive function starting from depth 0
m_interval m_expression_compute_range(m_expression *expr, m_expr_scope *scope)
{
	return m_expression_compute_range_rec(expr, scope, 0);
}

const char *m_expression_function_string(m_expression *expr)
{
	if (!expr)
		return "";
	
	switch (expr->type)
	{
		case M_EXPR_SQRT: 	return "sqrt";
		case M_EXPR_EXP: 	return "e^";
		case M_EXPR_LN: 	return "ln";
		case M_EXPR_SIN: 	return "sin";
		case M_EXPR_SINH: 	return "sinh";
		case M_EXPR_COS: 	return "cos";
		case M_EXPR_COSH:	return "cosh";
		case M_EXPR_TAN: 	return "tan";
		case M_EXPR_TANH: 	return "tanh";
		case M_EXPR_ASIN: 	return "asin";
		case M_EXPR_ACOS: 	return "acos";
		case M_EXPR_ATAN: 	return "atan";
		default: return "";
	}
	
	return "";
}

const char *m_expression_infix_operator_string(m_expression *expr)
{
	if (!expr)
		return "";
	
	switch (expr->type)
	{
		case M_EXPR_ADD: 	return " + ";
		case M_EXPR_SUB: 	return " - ";
		case M_EXPR_DIV: 	return " / ";
		case M_EXPR_MUL: 	return " * ";
		case M_EXPR_POW: 	return "^";
		default: return "";
	}
	
	return "";
}

int m_expression_print_rec(m_expression *expr, char *buf, int buf_len, int depth)
{
	if (!expr || !buf)
		return 0;
	
	int buf_pos = 0;
	const char *str_ptr;
	int len;
	
	if (buf_len == 1)
		goto m_expr_print_end;
	
	if (buf_len < 0)
		return 0;
	
	if (depth > M_EXPR_REC_MAX_DEPTH)
		goto m_expr_print_end;
	
	switch (m_expression_form(expr))
	{
		default:
		case M_EXPR_FORM_ATOMIC:
			if (expr->type == M_EXPR_CONST)
			{
				snprintf(buf, buf_len, "%.03f", expr->val.val_float);
			
				// snprintf doesn't exaaaaccctly return the number of
				// characters written, so just find it myself lol
				buf_pos = strlen(buf);
			}
			else if (expr->type == M_EXPR_REF)
			{
				if (!expr->val.ref_name)
				{
					buf[buf_pos++] = '('; if (buf_len < buf_pos + 1) goto m_expr_print_end;
					buf[buf_pos++] = 'n'; if (buf_len < buf_pos + 1) goto m_expr_print_end;
					buf[buf_pos++] = 'u'; if (buf_len < buf_pos + 1) goto m_expr_print_end;
					buf[buf_pos++] = 'l'; if (buf_len < buf_pos + 1) goto m_expr_print_end;
					buf[buf_pos++] = 'l'; if (buf_len < buf_pos + 1) goto m_expr_print_end;
					buf[buf_pos++] = ')';
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
			
		case M_EXPR_FORM_UNARY_OP:
			// Currently, there is only one unary operator with standard form. Cbf writing anything fancy
			buf[buf_pos++] = '-'; if (buf_len < buf_pos + 1) goto m_expr_print_end;
			
			if (expr->val.sub_exprs && expr->val.sub_exprs[0] && expr->val.sub_exprs[0]->type == M_EXPR_CONST && expr->val.sub_exprs[0]->val.val_float < 0)
			{
				goto bracketed_unary_sub_expr;
			}
			
			buf_pos += m_expression_print_rec(expr->val.sub_exprs[0], &buf[buf_pos], buf_len - buf_pos, depth + 1);
			break;
		
		case M_EXPR_FORM_UNARY_FN:
			str_ptr = m_expression_function_string(expr);
			
			while (str_ptr[buf_pos] != 0 && buf_pos < buf_len)
			{
				buf[buf_pos] = str_ptr[buf_pos];
				buf_pos++;
			}
			
			if (buf_len < buf_pos + 1) goto m_expr_print_end;
			
			goto bracketed_unary_sub_expr;
			break;
			
		case M_EXPR_FORM_INFIX_OP:
			buf[buf_pos++] = '('; if (buf_len < buf_pos + 1) goto m_expr_print_end;
			buf_pos += m_expression_print_rec(expr->val.sub_exprs[0], &buf[buf_pos], buf_len - buf_pos, depth + 1);
			if (buf_len < buf_pos + 1) goto m_expr_print_end;
			str_ptr = m_expression_infix_operator_string(expr);
			
			for (int i = 0; str_ptr[i] != 0 && buf_pos < buf_len; i++)
				buf[buf_pos++] = str_ptr[i];

			if (buf_len < buf_pos + 1) goto m_expr_print_end;
			
			buf_pos += m_expression_print_rec(expr->val.sub_exprs[1], &buf[buf_pos], buf_len - buf_pos, depth + 1);
			if (buf_len < buf_pos + 1) goto m_expr_print_end;
			buf[buf_pos++] = ')';
			break;
			
		case M_EXPR_FORM_NORM:
			buf[buf_pos++] = '|';  if (buf_len < buf_pos + 1) goto m_expr_print_end;
			buf_pos += m_expression_print_rec(expr->val.sub_exprs[0], &buf[buf_pos], buf_len - buf_pos, depth + 1);
			if (buf_len < buf_pos + 1) goto m_expr_print_end;
			buf[buf_pos++] = '|';
			break;
	}
	
	goto m_expr_print_end;
bracketed_unary_sub_expr:
	buf[buf_pos++] = '(';  if (buf_len < buf_pos + 1) goto m_expr_print_end;
	buf_pos += m_expression_print_rec(expr->val.sub_exprs[0], &buf[buf_pos], buf_len - buf_pos, depth + 1);
	if (buf_len < buf_pos + 1) goto m_expr_print_end;
	buf[buf_pos++] = ')';
	
m_expr_print_end:
	
	buf[buf_pos] = 0;
	return buf_pos;
}

char expr_print_buf[256];

int m_expression_print(m_expression *expr)
{
	if (!expr)
		return ERR_NULL_PTR;
	
	char buf[256];
	
	m_expression_print_rec(expr, buf, 256, 0);
	printf("%s", buf);
	
	return NO_ERROR;
}

const char *m_expression_to_string(m_expression *expr)
{
	if (!expr)
	{
		expr_print_buf[0] = 0;
		return expr_print_buf;
	}
	
	m_expression_print_rec(expr, expr_print_buf, 256, 0);
	
	return expr_print_buf;
}

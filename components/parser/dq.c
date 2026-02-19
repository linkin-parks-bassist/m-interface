#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>

#include "tokenizer.h"
#include "block.h"
#include "dq.h"
#include "m_parser.h"

char *m_dq_type_to_str(int type)
{
	switch (type)
	{
		case M_DERIVED_QUANTITY_CONST_FLT: 	return "CONST";
		case M_DERIVED_QUANTITY_NEG:		return "NEG";
		case M_DERIVED_QUANTITY_REFERENCE: 	return "REF";
		case M_DERIVED_QUANTITY_FCALL_ADD: 	return "ADD";
		case M_DERIVED_QUANTITY_FCALL_SUB: 	return "SUB";
		case M_DERIVED_QUANTITY_FCALL_MUL: 	return "MUL";
		case M_DERIVED_QUANTITY_FCALL_DIV: 	return "DIV";
		case M_DERIVED_QUANTITY_FCALL_ABS: 	return "ABS";
		case M_DERIVED_QUANTITY_FCALL_SQR: 	return "SQR";
		case M_DERIVED_QUANTITY_FCALL_SQRT: return "SQRT";
		case M_DERIVED_QUANTITY_FCALL_EXP: 	return "EXP";
		case M_DERIVED_QUANTITY_FCALL_LN: 	return "LN";
		case M_DERIVED_QUANTITY_FCALL_POW: 	return "POW";
		case M_DERIVED_QUANTITY_FCALL_SIN: 	return "SIN";
		case M_DERIVED_QUANTITY_FCALL_SINH: return "SINH";
		case M_DERIVED_QUANTITY_FCALL_COS: 	return "COS";
		case M_DERIVED_QUANTITY_FCALL_COSH: return "COSH";
		case M_DERIVED_QUANTITY_FCALL_TAN: 	return "TAN";
		case M_DERIVED_QUANTITY_FCALL_TANH: return "TANH";
		case M_DERIVED_QUANTITY_FCALL_ASIN: return "ASIN";
		case M_DERIVED_QUANTITY_FCALL_ACOS:	return "ACOS";
		case M_DERIVED_QUANTITY_FCALL_ATAN: return "ATAN";
	}
	
	return "TYPE_UNKNOWN";
}

// Compute arity in the sense of, how many sub-dq's it uses.
// this is used to guard accesses to the array dq->sub_dqs.
// therefore, if in doubt, return 0.
// it should not return x if dq->val.sub_dq[x-1]
// is not a valid pointer to another dq
int m_derived_quantity_arity(m_derived_quantity *dq)
{
	if (!dq) return NO_ERROR;
	
	// if the type is nonsense, return 0
	if (dq->type < 0 || dq->type > M_DERIVED_QUANTITY_TYPE_MAX_VAL)
		return 0;
	
	if (dq->type == M_DERIVED_QUANTITY_CONST_FLT || dq->type == M_DERIVED_QUANTITY_REFERENCE)
		return 0;
	
	// arity is at least 1 if we reach this point. there are more arity 1 types than arity 2, but also arity 2 will
	// be more common, bc arithmetic. and none of arity 3. therefore, check the arity 2 case, then return 1 otherwise
	if (dq->type == M_DERIVED_QUANTITY_FCALL_ADD
	 || dq->type == M_DERIVED_QUANTITY_FCALL_SUB
	 || dq->type == M_DERIVED_QUANTITY_FCALL_MUL
	 || dq->type == M_DERIVED_QUANTITY_FCALL_DIV
	 || dq->type == M_DERIVED_QUANTITY_FCALL_POW)
		return 2;
	
	return 1;
}

int m_derived_quantity_refers_constant(m_derived_quantity *dq)
{
	if (!dq)
		return 1;
	
	int ret_val = 0;
	
	if (dq->type == M_DERIVED_QUANTITY_REFERENCE)
	{
		if (strcmp(dq->val.ref_name, "pi") == 0)
			ret_val = 1;
		
		if (!ret_val && strcmp(dq->val.ref_name, "e") == 0)
			ret_val = 1;
			
		if (!ret_val && strcmp(dq->val.ref_name, "sample_rate") == 0)
			ret_val = 1;
		
		if (ret_val) dq->constant = 1;
	}
	
	return ret_val;
}

#define DQ_MAX_RECURSION_DEPTH 256

static float m_derived_quantity_compute_rec(m_derived_quantity *dq, m_parameter_pll *params, int depth)
{
	m_parameter_pll *current;
	m_parameter *param;
	int cmplen;
	
	float x = 0.0;
	float ret_val;
	
	//printf("dq compute (depth %d): %s (%d)\n", depth, m_dq_type_to_str(dq->type), dq->type);
	
	if (!dq)
	{
		printf("dq compute: NULL dq!\n");
		return 0.0;
	}
	
	if (dq->constant && dq->cached)
	{
		ret_val = dq->cached_val;
		goto dq_compute_return;
	}
	
	if (dq->type == M_DERIVED_QUANTITY_REFERENCE)
	{
		if (!dq->val.ref_name)
		{
			ret_val = 0.0;
			goto dq_compute_return;
		}
		
		if (strcmp(dq->val.ref_name, "pi") == 0)
		{
			ret_val = M_PI;
			goto dq_compute_return;
		}
		else if (strcmp(dq->val.ref_name, "e") == 0)
		{
			ret_val = exp(1);
			goto dq_compute_return;
		}
		else if (strcmp(dq->val.ref_name, "sample_rate") == 0)
		{
			ret_val = M_FPGA_SAMPLE_RATE;
			goto dq_compute_return;
		}
		
		if (!params)
		{
			ret_val = 0.0;
			goto dq_compute_return;
		}
		
		cmplen = strlen(dq->val.ref_name) + 1;
		
		current = params;
		
		while (current)
		{
			param = current->data;
			if (param && param->name_internal)
			{
				if (strncmp(dq->val.ref_name, param->name_internal, cmplen) == 0)
				{
					ret_val = param->value;
					goto dq_compute_return;
				}
			}
			
			current = current->next;
		}
		
		ret_val = 0.0;
		
		goto dq_compute_return;
	}
	
	if (dq->type == M_DERIVED_QUANTITY_CONST_FLT)
	{
		ret_val = dq->val.val_float;
		goto dq_compute_return;
	}
	
	if (depth > DQ_MAX_RECURSION_DEPTH)
	{
		printf("dq compute: hit max recursion depth!\n");
		ret_val = 0.0;
		goto dq_compute_return;
	}
	
	if (!dq->val.sub_dqs)
	{
		printf("dq compute: arity > 0 dq has no sub dqs!\n");
		ret_val = 0.0;
		goto dq_compute_return;
	}
	
	switch (dq->type)
	{
		case M_DERIVED_QUANTITY_NEG:
			ret_val = -(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;
			
		case M_DERIVED_QUANTITY_FCALL_ADD:
			ret_val = (m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1) + m_derived_quantity_compute_rec(dq->val.sub_dqs[1], params, depth + 1));
			break;

		case M_DERIVED_QUANTITY_FCALL_SUB:
			ret_val = m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1) - m_derived_quantity_compute_rec(dq->val.sub_dqs[1], params, depth + 1);
			break;

		case M_DERIVED_QUANTITY_FCALL_MUL:
			ret_val = m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1) * m_derived_quantity_compute_rec(dq->val.sub_dqs[1], params, depth + 1);
			break;

		case M_DERIVED_QUANTITY_FCALL_DIV:
			x = m_derived_quantity_compute_rec(dq->val.sub_dqs[1], params, depth + 1);
			
			if (fabsf(x) < 1e-20)
			{
				printf("dq compute: division by zero!\n");
				ret_val = 0.0;
				goto dq_compute_return; // avoid division by zero by just returning 0 lol. idk. what else to do?
			}
			
			ret_val = m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1) / x;
			break;

		case M_DERIVED_QUANTITY_FCALL_ABS:
			ret_val = fabs(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;

		case M_DERIVED_QUANTITY_FCALL_SQR: x = m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1); ret_val = x * x;
			break;

		case M_DERIVED_QUANTITY_FCALL_SQRT:
			ret_val = sqrt(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;

		case M_DERIVED_QUANTITY_FCALL_EXP:
			ret_val = exp(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;

		case M_DERIVED_QUANTITY_FCALL_LN:
			ret_val = log(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;

		case M_DERIVED_QUANTITY_FCALL_POW:
			ret_val = pow(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1),
						  m_derived_quantity_compute_rec(dq->val.sub_dqs[1], params, depth + 1));
			break;
		case M_DERIVED_QUANTITY_FCALL_SIN:
			ret_val = sin(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;
			
		case M_DERIVED_QUANTITY_FCALL_SINH:
			ret_val = sinh(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;
			
		case M_DERIVED_QUANTITY_FCALL_ASIN:
			ret_val = asin(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;

		case M_DERIVED_QUANTITY_FCALL_COS:
			ret_val = cos(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;
			
		case M_DERIVED_QUANTITY_FCALL_COSH:
			ret_val = cosh(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;
			
		case M_DERIVED_QUANTITY_FCALL_ACOS:
			ret_val = acos(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;

		case M_DERIVED_QUANTITY_FCALL_TAN:
			ret_val = tan(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;

		case M_DERIVED_QUANTITY_FCALL_TANH:
			ret_val = tanh(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;
			
		case M_DERIVED_QUANTITY_FCALL_ATAN:
			ret_val = atan(m_derived_quantity_compute_rec(dq->val.sub_dqs[0], params, depth + 1));
			break;
	}
	
dq_compute_return:
	//printf("dq compute (depth %d): return %f\n", depth, ret_val);
	
	dq->cached = 1;
	dq->cached_val = ret_val;
	
	return ret_val;
}

float m_derived_quantity_compute(m_derived_quantity *dq, m_parameter_pll *params)
{
	return m_derived_quantity_compute_rec(dq, params, 0);
}

int m_derived_quantity_references_param_rec(m_derived_quantity *dq, m_parameter *param, int depth)
{
	if (!dq || !param)
		return NO_ERROR;
	
	if (!param->name_internal)
		return NO_ERROR;
	
	int arity = m_derived_quantity_arity(dq);
	
	if (arity == 0)
	{
		if (dq->type != M_DERIVED_QUANTITY_REFERENCE)
			return NO_ERROR;
	
		if (!dq->val.ref_name)
				return NO_ERROR;
			
		return (strncmp(dq->val.ref_name, param->name_internal, strlen(dq->val.ref_name) + 1) == 0);
	}
	
	if (depth > DQ_MAX_RECURSION_DEPTH)
		return NO_ERROR;
	
	for (int i = 0; i < arity; i++)
	{
		if (m_derived_quantity_references_param_rec(dq->val.sub_dqs[i], param, depth + 1))
			return ERR_NULL_PTR;
	}
	
	return NO_ERROR;
}

int m_derived_quantity_references_param(m_derived_quantity *dq, m_parameter *param)
{
	return m_derived_quantity_references_param_rec(dq, param, 0);
}

m_derived_quantity m_derived_quantity_const_float(float v)
{
	m_derived_quantity result;
	result.type = M_DERIVED_QUANTITY_CONST_FLT;
	result.val.val_float = v;
	result.constant = 1;
	result.cached = 1;
	result.cached_val = v;
	return result;
}

m_derived_quantity *new_m_derived_quantity_const_float(float v)
{
	m_derived_quantity *result = m_alloc(sizeof(m_derived_quantity));
	
	if (!result) return NULL;
	
	*result = m_derived_quantity_const_float(v);
	
	return result;
}

m_derived_quantity *new_m_derived_quantity_reference(char *ref_name)
{
	if (!ref_name) return NULL;
	
	if (strcmp(ref_name, "pi") == 0)
		return new_m_derived_quantity_const_float(M_PI);
		
	if (strcmp(ref_name, "e") == 0)
		return new_m_derived_quantity_const_float(exp(1));
		
	if (strcmp(ref_name, "sample_rate") == 0)
		return new_m_derived_quantity_const_float(44100);
	
	m_derived_quantity *result = m_alloc(sizeof(m_derived_quantity));
	
	if (!result) return NULL;
	
	result->type = M_DERIVED_QUANTITY_REFERENCE;
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

m_derived_quantity *new_m_derived_quantity_from_string_rec(char *str, char **next)
{
	if (!str)
		return NULL;
	
	m_derived_quantity *dq = (m_derived_quantity*)m_alloc(sizeof(m_derived_quantity));
	
	if (!dq)
		return NULL;
	
	dq->type = M_DERIVED_QUANTITY_CONST_FLT;
	dq->val.val_float = 0.0;
	
	int pos = 0;
	
	int bufpos = 0;
	int len = strlen(str);
	
	char buf[len];
	
	int state = 0;
	char c;
	
	int arithmetic = 0;
	int unary_call = 0;
	int binary_call = 0;
	int type = 0;
	
	char *sub_next = NULL;
	
	while (pos < len + 1)
	{
		c = str[pos];
		
		switch (state)
		{
			case -1:
				m_free(dq);
				return NULL;
				
			case 0:
				if (c == ' ')
					break;
				
				if (c == '(')
				{
					pos++;
					continue;
				}
				
				if (c == 0)
				{
					state = -1;
				}
				else if (c == '+')
				{
					dq->type = M_DERIVED_QUANTITY_FCALL_ADD;
					binary_call = 1;
				}
				else if (c == '-')
				{
					dq->type = M_DERIVED_QUANTITY_FCALL_SUB;
					binary_call = 1;
				}
				else if (c == '*')
				{
					dq->type = M_DERIVED_QUANTITY_FCALL_MUL;
					binary_call = 1;
				}
				else if (c == '/')
				{
					dq->type = M_DERIVED_QUANTITY_FCALL_DIV;
					binary_call = 1;
				}
				else if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '_')
				{
					state = 1;
					buf[0] = c;
					bufpos = 1;
				}
				else if ('0' <= c && c <= '9')
				{
					state = 2;
					buf[0] = c;
					bufpos = 1;
				}
				else
				{
					state = -1;
				}
				
				break;
				
			case 1:
				if (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || ('0' <= c && c <= '9') || c == '_')
				{
					buf[bufpos++] = c;
				}
				else if (c == ' ' || c == ')' || c == 0)
				{
					buf[bufpos++] = 0;
					
					if (strcmp(buf, "abs") == 0)
					{
						dq->type = M_DERIVED_QUANTITY_FCALL_ABS;
						unary_call = 1;
					}
					else if (strcmp(buf, "sqr") == 0)
					{
						dq->type = M_DERIVED_QUANTITY_FCALL_SQR;
						unary_call = 1;
					}
					else if (strcmp(buf, "sqrt") == 0)
					{
						dq->type = M_DERIVED_QUANTITY_FCALL_SQRT;
						unary_call = 1;
					}
					else if (strcmp(buf, "exp") == 0)
					{
						dq->type = M_DERIVED_QUANTITY_FCALL_EXP;
						unary_call = 1;
					}
					else if (strcmp(buf, "ln") == 0)
					{
						dq->type = M_DERIVED_QUANTITY_FCALL_LN;
						unary_call = 1;
					}
					else if (strcmp(buf, "sin") == 0)
					{
						dq->type = M_DERIVED_QUANTITY_FCALL_SIN;
						unary_call = 1;
					}
					else if (strcmp(buf, "sinh") == 0)
					{
						dq->type = M_DERIVED_QUANTITY_FCALL_SINH;
						unary_call = 1;
					}
					else if (strcmp(buf, "cos") == 0)
					{
						dq->type = M_DERIVED_QUANTITY_FCALL_COS;
						unary_call = 1;
					}
					else if (strcmp(buf, "cosh") == 0)
					{
						dq->type = M_DERIVED_QUANTITY_FCALL_COSH;
						unary_call = 1;
					}
					else if (strcmp(buf, "tan") == 0)
					{
						dq->type = M_DERIVED_QUANTITY_FCALL_TAN;
						unary_call = 1;
					}
					else if (strcmp(buf, "tanh") == 0)
					{
						dq->type = M_DERIVED_QUANTITY_FCALL_TAN;
						unary_call = 1;
					}
					else  if (strcmp(buf, "pow") == 0)
					{
						dq->type = M_DERIVED_QUANTITY_FCALL_POW;
						binary_call = 1;
					}
					else
					{
						dq->type = M_DERIVED_QUANTITY_REFERENCE;
						dq->val.ref_name = m_strndup(buf, len);
						if (next) *next = &str[pos+1];
						return dq;
					}
				}
				else
				{
					state = -1;
				}
				
				break;
			
			case 2:
				if (('0' <= c && c <= '9') || c == '.')
				{
					buf[bufpos++] = c;
				}
				else if (c == ' ' || c == ')' || c == ',' || c == 0)
				{
					buf[bufpos] = 0;
					
					dq->type = M_DERIVED_QUANTITY_CONST_FLT;
					dq->val.val_float = strtof(buf, NULL);
					
					if (next)
						*next = &str[pos+1];
					
					return dq;
				}
				else
				{
					state = -1;
				}
				
				break;
			
			default:
				state = -1;
				break;
		}
		
		if (unary_call)
		{
			dq->val.sub_dqs = (m_derived_quantity**)m_alloc(sizeof(m_derived_quantity*) * 1);
			if (!dq->val.sub_dqs) // alloc fail; delete everything and bail
			{
				m_free(dq);
				return NULL;
			}
			dq->val.sub_dqs[0] = new_m_derived_quantity_from_string_rec(&str[pos+1], next);
			
			return dq;
		}
		
		if (binary_call)
		{
			dq->val.sub_dqs = (m_derived_quantity**)m_alloc(sizeof(m_derived_quantity*) * 2);
			if (!dq->val.sub_dqs) // alloc fail; delete everything and bail
			{
				m_free(dq);
				return NULL;
			}
			dq->val.sub_dqs[0] = new_m_derived_quantity_from_string_rec(&str[pos+1], &sub_next);
			dq->val.sub_dqs[1] = new_m_derived_quantity_from_string_rec(sub_next, next);
			
			return dq;
		}
		
		pos++;
	}
	
	return dq;
}

m_derived_quantity *new_m_derived_quantity_from_string(char *str)
{
	return new_m_derived_quantity_from_string_rec(str, NULL);
}

#define DQ_PARSE_MAX_DEPTH 128

int m_dq_token_unary_type(char *token)
{
	if (!token) return 0;
	
	if (strcmp(token, "-") == 0) return M_DERIVED_QUANTITY_NEG;
	
	if (strcmp(token, "abs")  == 0) return M_DERIVED_QUANTITY_FCALL_ABS;
	
	if (strcmp(token, "sqr")  == 0) return M_DERIVED_QUANTITY_FCALL_SQR;
	if (strcmp(token, "sqrt") == 0) return M_DERIVED_QUANTITY_FCALL_SQRT;
	
	if (strcmp(token, "exp")  == 0) return M_DERIVED_QUANTITY_FCALL_EXP;
	
	if (strcmp(token, "sin")  == 0) return M_DERIVED_QUANTITY_FCALL_SIN;
	if (strcmp(token, "cos")  == 0) return M_DERIVED_QUANTITY_FCALL_COS;
	if (strcmp(token, "tan")  == 0) return M_DERIVED_QUANTITY_FCALL_TAN;
	if (strcmp(token, "sinh") == 0) return M_DERIVED_QUANTITY_FCALL_SINH;
	if (strcmp(token, "cosh") == 0) return M_DERIVED_QUANTITY_FCALL_COSH;
	if (strcmp(token, "tanh") == 0) return M_DERIVED_QUANTITY_FCALL_TANH;
	if (strcmp(token, "asin") == 0) return M_DERIVED_QUANTITY_FCALL_ASIN;
	if (strcmp(token, "acos") == 0) return M_DERIVED_QUANTITY_FCALL_ACOS;
	if (strcmp(token, "atan") == 0) return M_DERIVED_QUANTITY_FCALL_ATAN;
	
	return 0;
}

int m_dq_token_infix_type(char *token)
{
	if (!token) return 0;
	
	if (strcmp(token, "+") == 0) return M_DERIVED_QUANTITY_FCALL_ADD;
	if (strcmp(token, "-") == 0) return M_DERIVED_QUANTITY_FCALL_SUB;
	
	if (strcmp(token, "*") == 0) return M_DERIVED_QUANTITY_FCALL_MUL;
	if (strcmp(token, "/") == 0) return M_DERIVED_QUANTITY_FCALL_DIV;
	
	if (strcmp(token, "^") == 0) return M_DERIVED_QUANTITY_FCALL_POW;
	
	return 0;
}

int m_dq_infix_operator_precedence(int infix_type)
{
	switch (infix_type)
	{
		case M_DERIVED_QUANTITY_FCALL_ADD:
		case M_DERIVED_QUANTITY_FCALL_SUB: return 10;
		
		case M_DERIVED_QUANTITY_FCALL_MUL:
		case M_DERIVED_QUANTITY_FCALL_DIV: return 20;
		
		case M_DERIVED_QUANTITY_FCALL_POW: return 40;
	}

    return -1;
}

// 1 for left, 0 for right
int m_dq_infix_associativity(int infix_type)
{
	switch (infix_type)
	{
		case M_DERIVED_QUANTITY_FCALL_ADD:
		case M_DERIVED_QUANTITY_FCALL_SUB:
		case M_DERIVED_QUANTITY_FCALL_MUL:
		case M_DERIVED_QUANTITY_FCALL_DIV: return 1;
	}
	
	return 0;
}

m_derived_quantity *new_m_derived_quantity_unary(int unary_type, m_derived_quantity *rhs)
{
	if (!rhs) return NULL;
	
	m_derived_quantity *lhs = (m_derived_quantity*)m_alloc(sizeof(m_derived_quantity));
	
	if (!lhs) return NULL;
	
	lhs->type = unary_type;
	lhs->val.sub_dqs = m_alloc(sizeof(m_derived_quantity*) * 1);
	
	if (!lhs->val.sub_dqs)
	{
		m_free(lhs);
		return NULL;
	}
	
	lhs->val.sub_dqs[0] = rhs;
	lhs->cached = 0;
	lhs->constant = rhs->constant;
	
	return lhs;
}

m_derived_quantity *new_m_derived_quantity_binary(int binary_type, m_derived_quantity *arg_1, m_derived_quantity *arg_2)
{
	if (!arg_1 || !arg_2) return NULL;
	
	m_derived_quantity *bin = (m_derived_quantity*)m_alloc(sizeof(m_derived_quantity));
	
	if (!bin) return NULL;
	
	bin->type = binary_type;
	bin->val.sub_dqs = m_alloc(sizeof(m_derived_quantity*) * 2);
	
	if (!bin->val.sub_dqs)
	{
		m_free(bin);
		return NULL;
	}
	
	bin->val.sub_dqs[0] = arg_1;
	bin->val.sub_dqs[1] = arg_2;
	
	bin->cached = 0;
	bin->constant = arg_2->constant && arg_1->constant;
	
	return bin;
}

#define UNARY_BINDING_POWER 30

m_derived_quantity *new_m_derived_quantity_from_tokens_rec_pratt(
    m_token_ll *tokens,
    m_token_ll **next_token,
    m_token_ll *tokens_end,
    int min_binding_power,
    int depth)
{
	if (!tokens || !tokens->data)
		return NULL;
	
	int line = tokens->line;
	
	if (depth > DQ_PARSE_MAX_DEPTH)
	{
		printf("Error (line %d): expression too deep\n", line);
		return NULL;
	}
	
	m_token_ll *current = tokens;
	m_token_ll *nt = NULL;
	m_derived_quantity *lhs = NULL;
	m_derived_quantity *rhs = NULL;
	m_derived_quantity *bin = NULL;
	
	int precedence;
	int left_binding_power;
	int right_binding_power;
	
	
	int unary_type = m_dq_token_unary_type(current->data);
	int infix_type;
	
	if (token_is_number(current->data))
	{
		lhs = new_m_derived_quantity_const_float(token_to_float(tokens->data));
		if (!lhs) return NULL;
		
		current = current->next;
	}
	else if (strcmp(current->data, "(") == 0)
	{
		lhs = new_m_derived_quantity_from_tokens_rec_pratt(
				  current->next,
				  &current,
				  tokens_end,
				  0,
				  depth + 1);
		
		if (!lhs) return NULL;
		
		if (!current || strcmp(current->data, ")") != 0)
		{
			printf("Error (line %d): malformed expression\n", line);
			goto pratt_bail;
		}
		
		current = current->next;
	}
	else if (unary_type)
	{
		rhs = new_m_derived_quantity_from_tokens_rec_pratt(
				current->next,
				&nt,
				tokens_end,
				UNARY_BINDING_POWER,
				depth + 1);
		
		if (!rhs) goto pratt_bail;
		
		lhs = new_m_derived_quantity_unary(unary_type, rhs);
		
		if (!lhs) goto pratt_bail;
		
		current = nt;
	}
	else if (token_is_name(current->data))
	{
		lhs = new_m_derived_quantity_reference(current->data);
		
		current = current->next;
	}
	else
	{
		printf("Error (line %d): unexpected \"%s\"\n", line, current->data);
		goto pratt_bail;
	}
	
	while (current && current != tokens_end)
	{
		infix_type = m_dq_token_infix_type(current->data);
		
		if (!infix_type) break;

		precedence = m_dq_infix_operator_precedence(infix_type);
		left_binding_power = precedence;
		right_binding_power = precedence + m_dq_infix_associativity(infix_type);

		if (left_binding_power < min_binding_power) break;

		current = current->next;

		rhs = new_m_derived_quantity_from_tokens_rec_pratt(
				current,
				&nt,
				tokens_end,
				right_binding_power,
				depth + 1);

		if (!rhs) goto pratt_bail;
			
		bin = new_m_derived_quantity_binary(infix_type, lhs, rhs);
		
		if (!bin) goto pratt_bail;
		
		lhs = bin;
		current = nt;
	}
	
	if (next_token)
		*next_token = current;

	return lhs;

pratt_bail:
	// free anything allocated
	return NULL;
}

m_derived_quantity *new_m_derived_quantity_from_tokens_rec(m_token_ll *tokens, m_token_ll **next_token, m_token_ll *tokens_end, int depth)
{
	if (!tokens || !tokens->data)
		return NULL;
	
	//printf("Parsing DQ (depth %d). Token: \"%s\"\n", depth, tokens->data);
	
	if (depth > DQ_PARSE_MAX_DEPTH)
	{
		printf("Error (line %d): expression too deep\n", tokens->line);
		return NULL;
	}
	
	if (tokens == tokens_end)
	{
		printf("Error (line %d): expression terminated early\n", tokens->line);
		return NULL;
	}
	
	m_token_ll *current = tokens;
	
	// Just ignore brackets, who cares
	if (strcmp(current->data, "(") == 0 || strcmp(current->data, ")") == 0)
	{
		return new_m_derived_quantity_from_tokens_rec(tokens->next, next_token, tokens_end, depth + 1);
	}
	
	m_derived_quantity *dq = (m_derived_quantity*)m_alloc(sizeof(m_derived_quantity));
	
	if (!dq) return NULL;
	
	int len = strlen(current->data);
	
	int unary_call = 0;
	int binary_call = 0;
	
	if (token_is_number(current->data))
	{
		dq->type = M_DERIVED_QUANTITY_CONST_FLT;
		dq->val.val_float = token_to_float(tokens->data);
	}
	else if (strcmp(current->data, "+") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_ADD;
		binary_call = 1;
	}
	else if (strcmp(current->data, "-") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_SUB;
		binary_call = 1;
	}
	else if (strcmp(current->data, "*") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_MUL;
		binary_call = 1;
	}
	else if (strcmp(current->data, "/") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_DIV;
		binary_call = 1;
	}
	else if (strcmp(current->data, "abs") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_ABS;
		unary_call = 1;
	}
	else if (strcmp(current->data, "sqr") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_SQR;
		unary_call = 1;
	}
	else if (strcmp(current->data, "sqrt") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_SQRT;
		unary_call = 1;
	}
	else if (strcmp(current->data, "exp") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_EXP;
		unary_call = 1;
	}
	else if (strcmp(current->data, "ln") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_LN;
		unary_call = 1;
	}
	else if (strcmp(current->data, "sin") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_SIN;
		unary_call = 1;
	}
	else if (strcmp(current->data, "sinh") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_SINH;
		unary_call = 1;
	}
	else if (strcmp(current->data, "cos") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_COS;
		unary_call = 1;
	}
	else if (strcmp(current->data, "cosh") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_COSH;
		unary_call = 1;
	}
	else if (strcmp(current->data, "tan") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_TAN;
		unary_call = 1;
	}
	else if (strcmp(current->data, "tanh") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_TAN;
		unary_call = 1;
	}
	else  if (strcmp(current->data, "pow") == 0)
	{
		dq->type = M_DERIVED_QUANTITY_FCALL_POW;
		binary_call = 1;
	}
	else
	{
		dq->type = M_DERIVED_QUANTITY_REFERENCE;
		dq->val.ref_name = m_strndup(current->data, len);
	}
	
	m_token_ll *nt;
	
	if (unary_call)
	{
		dq->val.sub_dqs = (m_derived_quantity**)m_alloc(sizeof(m_derived_quantity*) * 1);
		if (!dq->val.sub_dqs) // alloc fail; delete everything and bail
		{
			m_free(dq);
			return NULL;
		}
		dq->val.sub_dqs[0] = new_m_derived_quantity_from_tokens_rec(tokens->next, next_token, tokens_end, depth + 1);
	}
	else if (binary_call)
	{
		dq->val.sub_dqs = (m_derived_quantity**)m_alloc(sizeof(m_derived_quantity*) * 2);
		if (!dq->val.sub_dqs) // alloc fail; delete everything and bail
		{
			m_free(dq);
			return NULL;
		}
		
		dq->val.sub_dqs[0] = new_m_derived_quantity_from_tokens_rec(tokens->next, &nt, tokens_end, depth + 1);
		dq->val.sub_dqs[1] = new_m_derived_quantity_from_tokens_rec(nt, next_token, tokens_end, depth + 1);
	}
	else if (next_token)
	{
		*next_token = current->next;
	}
	
	/*printf("Result (depth %d): ", depth);
	switch (dq->type)
	{
		case M_DERIVED_QUANTITY_CONST_FLT:
			printf(" constant, %.04f\n", dq->val.val_float);
			break;
		case M_DERIVED_QUANTITY_REFERENCE:
			printf(" reference, \"%s\"\n", dq->val.ref_name);
			break;
		default:
			printf(" function call\n");
			break;
	}*/
	
	return dq;
}

m_derived_quantity *new_m_derived_quantity_from_tokens(m_token_ll *tokens, m_token_ll *tokens_end)
{
	m_token_ll *next_token;
	m_derived_quantity *dq = new_m_derived_quantity_from_tokens_rec_pratt(tokens, &next_token, tokens_end, 0, 0);
	
	int anything = 0;
	m_token_ll *check = next_token;
	if (next_token != tokens_end)
	{
		// check if there's anyhting of substance
		while (check && check != tokens_end && !anything)
		{
			if (next_token->data && (strcmp(next_token->data, ")") != 0 && strcmp(next_token->data, "\n")))
				anything = 1;
			check = check->next;
		}
		if (anything)
			printf("Error: expression finished early, on token \"%s\", rather than \"%s\"\n", next_token->data, tokens_end->data);
	}
	
	return dq;
}

int m_dq_type_monotonicity(int type)
{
	switch (type)
	{
		case M_DERIVED_QUANTITY_CONST_FLT: 	return  1;
		case M_DERIVED_QUANTITY_REFERENCE: 	return  1;
		case M_DERIVED_QUANTITY_NEG: 		return -1;
		case M_DERIVED_QUANTITY_FCALL_ADD: 	return  1;
		case M_DERIVED_QUANTITY_FCALL_SUB: 	return  0;
		case M_DERIVED_QUANTITY_FCALL_MUL: 	return  0;
		case M_DERIVED_QUANTITY_FCALL_DIV: 	return  0;
		case M_DERIVED_QUANTITY_FCALL_ABS: 	return  0;
		case M_DERIVED_QUANTITY_FCALL_SQR: 	return  1;
		case M_DERIVED_QUANTITY_FCALL_SQRT: return  1;
		case M_DERIVED_QUANTITY_FCALL_EXP: 	return  1;
		case M_DERIVED_QUANTITY_FCALL_LN: 	return  1;
		case M_DERIVED_QUANTITY_FCALL_POW: 	return  0;
		case M_DERIVED_QUANTITY_FCALL_SIN: 	return  0;
		case M_DERIVED_QUANTITY_FCALL_SINH: return  1;
		case M_DERIVED_QUANTITY_FCALL_COS: 	return  0;
		case M_DERIVED_QUANTITY_FCALL_COSH: return  0;
		case M_DERIVED_QUANTITY_FCALL_TAN: 	return  0;
		case M_DERIVED_QUANTITY_FCALL_TANH: return  1;
		case M_DERIVED_QUANTITY_FCALL_ASIN: return  1;
		case M_DERIVED_QUANTITY_FCALL_ACOS: return -1;
		case M_DERIVED_QUANTITY_FCALL_ATAN: return  1;
	}
	
	return 0;
}

#define DQ_BOUND_MAX_DEPTH 16

float m_dq_max_rec(m_derived_quantity *dq, m_parameter_pll *params, int depth);

float m_dq_min_rec(m_derived_quantity *dq, m_parameter_pll *params, int depth)
{
	m_parameter_pll *current;
	m_parameter *rp;
	int found;
	
	float x_min;
	float x_max;
	float y_min;
	float y_min_d;
	float y_max;
	float y_max_d;
	
	float p1, p2, p3, p4;
	
	float z;
	
	int k;
	
	int monotonicity = 0;
	
	float ret;
	
	if (!dq)
	{
		printf("m_dq_min_rec: NULL pointer! return -FLT_MAX\n");
		ret = -FLT_MAX;
		goto dq_min_ret;
	}
	if (depth > DQ_BOUND_MAX_DEPTH)
	{
		printf("m_dq_min_rec: reached max depth. return -FLT_MAX\n");
		ret = -FLT_MAX;
		goto dq_min_ret;
	}
	
	printf("m_dq_min_rec (depth %d): %s\n", depth, m_dq_type_to_str(dq->type));
	
	if (dq->constant && dq->cached)
	{
		printf("result constant & cached. return cached val %f\n", dq->cached_val);
		ret = dq->cached_val;
		goto dq_min_ret;
	}
	
	if (dq->type == M_DERIVED_QUANTITY_CONST_FLT)
	{
		ret = dq->val.val_float;
		goto dq_min_ret;
	}
	
	if (dq->type == M_DERIVED_QUANTITY_REFERENCE)
	{
		if (m_derived_quantity_refers_constant(dq))
		{
			ret = m_derived_quantity_compute(dq, NULL);
			goto dq_min_ret;
		}
		
		current = params;
		
		found = 0;
		while (current && !found)
		{
			if (current->data && strcmp(current->data->name, dq->val.ref_name) == 0)
			{
				rp = current->data;
				found = 1;
			}
			
			current = current->next;
		}
		
		if (found)
		{
			if (rp->min_dq && depth < DQ_BOUND_MAX_DEPTH)
			{
				ret = m_derived_quantity_compute(rp->min_dq, params);
				goto dq_min_ret;
			}
			else
			{
				ret = rp->min;
				goto dq_min_ret;
			}
		}
		else
		{
			ret = -FLT_MAX;
			goto dq_min_ret;
		}
	}
	
	int arity = m_derived_quantity_arity(dq);
	
	if (arity == 1 && (!dq->val.sub_dqs || !dq->val.sub_dqs[0]))
	{
		ret = -FLT_MAX;
		goto dq_min_ret;
	}
	
	if (arity == 2 && (!dq->val.sub_dqs || !dq->val.sub_dqs[0] || !dq->val.sub_dqs[1]))
	{
		ret = -FLT_MAX;
		goto dq_min_ret;
	}
	
	monotonicity = m_dq_type_monotonicity(dq->type);
	
	if (arity >= 1)
	{
		if (dq->val.sub_dqs[0]->constant && dq->val.sub_dqs[0]->cached)
		{
			x_min = dq->val.sub_dqs[0]->cached_val;
			x_max = dq->val.sub_dqs[0]->cached_val;
		}
		else
		{
			if (monotonicity >= 0)
				x_min = m_dq_min_rec(dq->val.sub_dqs[0], params, depth + 1);
			if (monotonicity <= 0)
				x_max = m_dq_max_rec(dq->val.sub_dqs[0], params, depth + 1);
		}
	}
	
	
	if (arity > 1)
	{
		if (dq->val.sub_dqs[1]->constant && dq->val.sub_dqs[1]->cached)
		{
			y_min = dq->val.sub_dqs[1]->cached_val;
			y_max = dq->val.sub_dqs[1]->cached_val;
		}
		else
		{
			//if (monotonicity >= 0)
				y_min = m_dq_min_rec(dq->val.sub_dqs[1], params, depth + 1);
			//if (monotonicity <= 0)
				y_max = m_dq_max_rec(dq->val.sub_dqs[1], params, depth + 1);
		}
	}
	
	switch (dq->type)
	{
		case M_DERIVED_QUANTITY_NEG:
			ret = -x_max;
			goto dq_min_ret;
		
		case M_DERIVED_QUANTITY_FCALL_ADD:
			ret = (x_min + y_min);
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SQRT:
			ret = sqrtf(x_min);
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_LN:
			x_min = x_min;
			
			if (x_min <= 0)
			{
				ret = -FLT_MAX;
				goto dq_min_ret;
			}
			
			ret = log(x_min);
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_ASIN:
			if (x_min < -1)
			{
				ret = -M_PI / 2;
				goto dq_min_ret;
			}
			
			if (x_min > 1)
			{
				ret = M_PI / 2;
				goto dq_min_ret;
			}
			
			ret = asin(x_min);
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_ACOS:
			if (x_max < -1)
			{
				ret = M_PI;
				goto dq_min_ret;
			}
			
			if (x_max > 1)
			{
				ret = 0.0f;
				goto dq_min_ret;
			}
			
			ret = acos(x_max);
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_ATAN:
			x_min = x_min;
			
			ret = atan(x_min);
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_TANH:
			ret = tanh(x_min);
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SINH:
			ret = log(x_min);
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_EXP:
			ret = log(x_min);
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SUB:
			ret = (x_min - y_max);
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SQR:
			x_min = x_min;
			x_max = x_max;
			
			if (x_min < 0)
			{
				if (x_max > 0)
				{
					ret = 0.0;
					goto dq_min_ret;
				}
				else
				{
					ret = x_max * x_max;
					goto dq_min_ret;
				}
			}
			else
			{
				ret = x_min * x_min;
				goto dq_min_ret;
			}
			
		case M_DERIVED_QUANTITY_FCALL_COSH:
			x_min = x_min;
			x_max = x_max;
			
			if (x_min < 0)
			{
				if (x_max > 0)
				{
					ret = 1.0;
					goto dq_min_ret;
				}
				else
				{
					ret = cosh(x_max);
					goto dq_min_ret;
				}
			}
			else
			{
				ret = cosh(x_min);
				goto dq_min_ret;
			}
			
		case M_DERIVED_QUANTITY_FCALL_ABS:
			x_min = x_min;
			x_max = x_max;
			
			if (x_min < 0)
			{
				if (x_max > 0)
				{
					ret = 0.0;
					goto dq_min_ret;
				}
				else
				{
					ret = -x_max;
					goto dq_min_ret;
				}
			}
			else
			{
				ret = x_min;
				goto dq_min_ret;
			}
			
		case M_DERIVED_QUANTITY_FCALL_MUL:
			x_min = x_min;
			x_max = x_max;
			y_min = y_min;
			y_max = y_max;
			
			p1 = x_min * y_min;
			p2 = x_min * y_max;
			p3 = x_max * y_min;
			p4 = x_max * y_max;
			
			z = p1;
			if (p2 < z) z = p2;
			if (p3 < z) z = p3;
			if (p4 < z) z = p4;
			
			ret = z;
			
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_DIV:
			x_min = x_min;
			x_max = x_max;
			y_min = y_min;
			y_max = y_max;
			
			// if y can cross 0, oh no
			if (y_min <= 0.0f && 0.0f <= y_max)
			{
				if (x_min > 0.0f)
				{
					ret = -FLT_MAX;
					goto dq_min_ret;
				}
				else if (x_max == 0.0f && x_min == 0.0f)
				{
					ret = 0.0;
					goto dq_min_ret;
				}
			}
			
			// otherwise, clamp y away from 0 for safety
			if (y_min < 0.0f && y_min > -1e-20f)
				y_min = -1e-20f;
			if (y_min > 0.0f && y_min < 1e-20f)
				y_min = 1e-20f;
			
			if (y_max < 0.0f && y_max > -1e-20f)
				y_max = -1e-20f;
			if (y_max > 0.0f && y_max < 1e-20f)
				y_max = 1e-20f;
			
			y_min_d = (fabsf(y_min) < 1e-20) ? FLT_MAX : 1.0 / y_min;
			y_max_d = (fabsf(y_max) < 1e-20) ? FLT_MAX : 1.0 / y_max;
			
			p1 = x_min * y_min_d;
			p2 = x_min * y_max_d;
			p3 = x_max * y_min_d;
			p4 = x_max * y_max_d;
			
			z = p1;
			if (p2 < z) z = p2;
			if (p3 < z) z = p3;
			if (p4 < z) z = p4;
			
			ret = z;
			
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_POW:
			x_min = x_min;
			x_max = x_max;
			y_min = y_min;
			y_max = y_max;
			
			// negative bases for powers cause sign nonsense.
			// ignore them. accept that results will be wrong
			if (x_min < 0)
				x_min = 0;
			if (x_max < 0)
				x_max = 0;
			
			p1 = pow(x_min, y_min);
			p2 = pow(x_min, y_max);
			p3 = pow(x_max, y_min);
			p4 = pow(x_max, y_max);
			
			z = p1;
			if (p2 < z) z = p2;
			if (p3 < z) z = p3;
			if (p4 < z) z = p4;
			
			ret = z;
			
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SIN:
			/* min */
			x_min = x_min;
			x_max = x_max;
			
			if (x_max - x_min > (2.0*M_PI))
			{
				ret = -1;
				goto dq_min_ret;
			}
			
			k = (int)ceilf((x_min + M_PI/2) / (2.0*M_PI));
			
			if (- (0.5*M_PI) + k * (2.0*M_PI) <= x_max)
			{
				ret = -1;
				goto dq_min_ret;
			}
			
			p1 = sin(x_min);
			p2 = sin(x_max);
			
			if (p2 < p1)
			{
				ret = p2;
				goto dq_min_ret;
			}
			
			ret = p1;
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_COS:
			x_min = x_min + (0.5*M_PI);
			x_max = x_max + (0.5*M_PI);
			
			if (x_max - x_min > (2.0*M_PI))
			{
				ret = -1;
				goto dq_min_ret;
			}
			
			k = (int)ceilf((x_min + (0.5*M_PI)) / (2.0*M_PI));
			
			if (-(0.5*M_PI) + k * (2.0*M_PI) <= x_max)
			{
				ret = -1;
				goto dq_min_ret;
			}
			
			p1 = sin(x_min);
			p2 = sin(x_max);
			
			if (p2 < p1)
			{
				ret = p2;
				goto dq_min_ret;
			}
			
			ret = p1;
			
			goto dq_min_ret;
			
		case M_DERIVED_QUANTITY_FCALL_TAN:
			x_min = x_min;
			x_max = x_max;
			
			if (x_max - x_min > (2.0*M_PI))
			{
				ret = -FLT_MAX;
				goto dq_min_ret;
			}
			
			p1 = tan(x_min);
			p2 = tan(x_max);
			
			if (p2 < p1)
			{
				ret = p2;
				goto dq_min_ret;
			}
			
			ret = p1;
			
			goto dq_min_ret;
		
		default:
			ret = -FLT_MAX;
			goto dq_min_ret;
	}
	
dq_min_ret:
	printf("(depth %d) (%s) min: %f\n", depth, m_dq_type_to_str(dq->type), ret);
	return ret;
}

float m_dq_min(m_derived_quantity *dq, m_parameter_pll *params)
{
	return m_dq_min_rec(dq, params, 0);
}

float m_dq_max_rec(m_derived_quantity *dq, m_parameter_pll *params, int depth)
{
	m_parameter_pll *current;
	m_parameter *rp;
	int found;
	
	float ret;
	
	float x_min;
	float x_max;
	float y_min;
	float y_min_d;
	float y_max;
	float y_max_d;
	
	float p1, p2, p3, p4;
	
	float z;
	
	int k;
	int k_min;
	int k_max;
	int monotonicity;
	
	if (!dq)
	{
		printf("m_dq_max_rec: NULL pointer! return -FLT_MAX\n");
		ret = FLT_MAX;
		goto dq_max_ret;
	}
	if (depth > DQ_BOUND_MAX_DEPTH)
	{
		printf("m_dq_max_rec: reached max depth. return -FLT_MAX\n");
		ret = FLT_MAX;
		goto dq_max_ret;
	}
	
	printf("m_dq_max_rec (depth %d): %s\n", depth, m_dq_type_to_str(dq->type));
	
	if (dq->constant && dq->cached)
	{
		printf("result constant & cached. return cached val %f\n", dq->cached_val);
		ret = dq->cached_val;
		goto dq_max_ret;
	}
	
	if (dq->type == M_DERIVED_QUANTITY_CONST_FLT)
	{
		ret = dq->val.val_float;
		goto dq_max_ret;
	}
	
	if (dq->type == M_DERIVED_QUANTITY_REFERENCE)
	{
		if (m_derived_quantity_refers_constant(dq))
		{
			ret = m_derived_quantity_compute(dq, NULL);
			goto dq_max_ret;
		}
		
		current = params;
		
		found = 0;
		while (current && !found)
		{
			if (current->data && strcmp(current->data->name, dq->val.ref_name) == 0)
			{
				rp = current->data;
				found = 1;
			}
			
			current = current->next;
		}
		
		if (found)
		{
			if (rp->max_dq && depth < DQ_BOUND_MAX_DEPTH)
			{
				ret = m_derived_quantity_compute(rp->max_dq, params);
				goto dq_max_ret;
			}
			else
			{
				ret = rp->max;
				goto dq_max_ret;
			}
		}
		else
		{
			ret = -FLT_MAX;
			goto dq_max_ret;
		}
	}
	
	int arity = m_derived_quantity_arity(dq);
	
	if (arity == 1 && (!dq->val.sub_dqs || !dq->val.sub_dqs[0]))
	{
		ret = -FLT_MAX;
		goto dq_max_ret;
	}
	
	if (arity == 2 && (!dq->val.sub_dqs || !dq->val.sub_dqs[0] || !dq->val.sub_dqs[1]))
	{
		ret = -FLT_MAX;
		goto dq_max_ret;
	}
	
	monotonicity = m_dq_type_monotonicity(dq->type);
	
	if (arity >= 1)
	{
		if (dq->val.sub_dqs[0]->constant && dq->val.sub_dqs[0]->cached)
		{
			x_min = dq->val.sub_dqs[0]->cached_val;
			x_max = dq->val.sub_dqs[0]->cached_val;
		}
		else
		{
			//if (monotonicity <= 0)
				x_min = m_dq_min_rec(dq->val.sub_dqs[0], params, depth + 1);
			//if (monotonicity >= 0)
				x_max = m_dq_max_rec(dq->val.sub_dqs[0], params, depth + 1);
		}
	}
	
	if (arity > 1)
	{
		if (dq->val.sub_dqs[1]->constant && dq->val.sub_dqs[1]->cached)
		{
			y_min = dq->val.sub_dqs[1]->cached_val;
			y_max = dq->val.sub_dqs[1]->cached_val;
		}
		else
		{
			//if (monotonicity <= 0)
				y_min = m_dq_min_rec(dq->val.sub_dqs[1], params, depth + 1);
			//if (monotonicity >= 0)
				y_max = m_dq_max_rec(dq->val.sub_dqs[1], params, depth + 1);
		}
	}
	
	switch (dq->type)
	{
		case M_DERIVED_QUANTITY_NEG:
			ret = -x_min;
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_ADD:
			ret = (x_max + y_max);
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SQRT:
			ret = sqrtf(x_max);
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_LN:
			ret = log(x_max);
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_ASIN:
			x_max = x_max;
			
			if (x_max < -1)
			{
				ret = -M_PI / 2;
				goto dq_max_ret;
			}
			
			if (x_max > 1)
			{
				ret = M_PI / 2;
				goto dq_max_ret;
			}
			
			ret = asin(x_max);
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_ACOS:
			x_min = x_min;
			
			if (x_min < -1)
			{
				ret = M_PI;
				goto dq_max_ret;
			}
			
			if (x_min > 1)
			{
				ret = 0.0f;
				goto dq_max_ret;
			}
			
			ret = acos(x_min);
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_ATAN:
			x_max = x_max;
			
			ret = atan(x_max);
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_TANH:
			ret = tanh(x_max);
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SINH:
			ret = log(x_max);
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_EXP:
			ret = log(x_max);
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SUB:
			ret = (x_max - y_min);
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SQR:
			x_min = x_min;
			x_max = x_max;
			
			p1 = x_min * x_min;
			p2 = x_max * x_max;
			
			if (p2 > p1)
			{
				ret = p2;
				goto dq_max_ret;
			}
			
			ret = p1;
			
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_COSH:
			x_min = x_min;
			x_max = x_max;
			
			p1 = cosh(x_min);
			p2 = cosh(x_max);
			
			if (p2 > p1)
			{
				ret = p2;
				goto dq_max_ret;
			}
			
			ret = p1;
			
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_ABS:
			x_min = x_min;
			x_max = x_max;
			
			p1 = fabsf(x_min);
			p2 = fabsf(x_max);
			
			if (p2 > p1)
			{
				ret = p2;
				goto dq_max_ret;
			}
			
			ret = p1;
			
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_MUL:
			x_min = x_min;
			x_max = x_max;
			y_min = y_min;
			y_max = y_max;
			
			p1 = x_min * y_min;
			p2 = x_min * y_max;
			p3 = x_max * y_min;
			p4 = x_max * y_max;
			
			z = p1;
			if (p2 > z) z = p2;
			if (p3 > z) z = p3;
			if (p4 > z) z = p4;
			
			ret = z;
			
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_DIV:
			x_min = x_min;
			x_max = x_max;
			y_min = y_min;
			y_max = y_max;
			
			// if y can cross 0, oh no
			if (y_min <= 0.0f && 0.0f <= y_max)
			{
				if (x_max < 0.0f)
				{
					ret = FLT_MAX;
					goto dq_max_ret;
				}
				else if (x_max == 0.0f && x_min == 0.0f)
				{
					ret = 0.0;
					goto dq_max_ret;
				}
			}
			
			// otherwise, clamp y away from 0 for safety
			if (y_min < 0.0f && y_min > -1e-20f)
				y_min = -1e-20f;
			if (y_min > 0.0f && y_min < 1e-20f)
				y_min = 1e-20f;
			
			if (y_max < 0.0f && y_max > -1e-20f)
				y_max = -1e-20f;
			if (y_max > 0.0f && y_max < 1e-20f)
				y_max = 1e-20f;
			
			y_min_d = (fabsf(y_min) < 1e-20) ? FLT_MAX : 1.0 / y_min;
			y_max_d = (fabsf(y_max) < 1e-20) ? FLT_MAX : 1.0 / y_max;
			
			p1 = x_min * y_min_d;
			p2 = x_min * y_max_d;
			p3 = x_max * y_min_d;
			p4 = x_max * y_max_d;
			
			z = p1;
			if (p2 > z) z = p2;
			if (p3 > z) z = p3;
			if (p4 > z) z = p4;
			
			ret = z;
			
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_POW:
			x_min = x_min;
			x_max = x_max;
			y_min = y_min;
			y_max = y_max;
			
			// don't do anything silly with pow. you will get wrong results.
			if (x_min < 0)
				x_min = 0;
			if (y_min < 0)
				y_min = 0;
			
			if (x_max < 0)
				x_max = 0;
			if (y_max < 0)
				y_max = 0;
			
			p1 = pow(x_min, y_min);
			p2 = pow(x_min, y_max);
			p3 = pow(x_max, y_min);
			p4 = pow(x_max, y_max);
			
			z = p1;
			if (p2 > z) z = p2;
			if (p3 > z) z = p3;
			if (p4 > z) z = p4;
			
			ret = z;
			
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SIN:
			if (x_max - x_min > (2.0*M_PI))
			{
				ret = 1;
				goto dq_max_ret;
			}
			
			k = (int)ceilf((x_min - (0.5*M_PI)) / (2.0*M_PI));
			
			if ((0.5*M_PI) + k * (2.0*M_PI) <= x_max)
			{
				ret = 1;
				goto dq_max_ret;
			}
			
			p1 = sin(x_min);
			p2 = sin(x_max);
			
			if (p2 > p1)
			{
				ret = p2;
				goto dq_max_ret;
			}
			
			ret = p1;
			
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_COS:
			x_min = x_min + (0.5*M_PI);
			x_max = x_max + (0.5*M_PI);
			
			if (x_max - x_min > (2.0*M_PI))
			{
				ret = 1;
				goto dq_max_ret;
			}
			
			k = (int)ceilf((x_min - (0.5*M_PI)) / (2.0*M_PI));
			
			if ((0.5*M_PI) + k * (2.0*M_PI) <= x_max)
			{
				ret = 1;
				goto dq_max_ret;
			}
			
			p1 = sin(x_min);
			p2 = sin(x_max);
			
			if (p2 > p1)
			{
				ret = p2;
				goto dq_max_ret;
			}
			
			ret = p1;
			
			goto dq_max_ret;
			
		case M_DERIVED_QUANTITY_FCALL_TAN:
			x_min = x_min;
			x_max = x_max;
			
			if (x_max - x_min > (2.0*M_PI))
			{
				ret = FLT_MAX;
				goto dq_max_ret;
			}
			
			p1 = tan(x_min);
			p2 = tan(x_max);
			
			if (p2 > p1)
			{
				ret = p2;
				goto dq_max_ret;
			}
			
			ret = p1;
			
			goto dq_max_ret;
		
		default:
			ret = FLT_MAX;
			goto dq_max_ret;
	}
	
dq_max_ret:
	printf("(depth %d) (%s) max: %f\n", depth, m_dq_type_to_str(dq->type), ret);
	return ret;
}

float m_dq_max(m_derived_quantity *dq, m_parameter_pll *params)
{
	return m_dq_max_rec(dq, params, 0);
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

m_interval m_derived_quantity_compute_range_rec(m_derived_quantity *dq, m_parameter_pll *params, int depth)
{
	m_parameter_pll *current;
	m_parameter *rp;
	int found;
	
	float x_min;
	float x_max;
	float y_min;
	float y_min_d;
	float y_max;
	float y_max_d;
	
	float p1, p2, p3, p4;
	
	float z;
	
	int k;
	
	int monotonicity = 0;
	
	m_interval ret;
	
	m_interval x_int;
	m_interval y_int;
	m_interval y_int_d;
	
	int p_c = 0;
	
	if (!dq)
	{
		ret = m_interval_real_line();
		goto dq_int_ret;
	}
	if (depth > DQ_BOUND_MAX_DEPTH)
	{
		ret = m_interval_real_line();
		goto dq_int_ret;
	}
	
	//printf("compute range (depth %d): %s\n", depth, m_dq_type_to_str(dq->type));
	
	if (dq->constant && dq->cached)
	{
		//printf("result constant & cached. return cached val %f\n", dq->cached_val);
		ret = m_interval_singleton(dq->cached_val);
		goto dq_int_ret;
	}
	
	if (dq->type == M_DERIVED_QUANTITY_CONST_FLT)
	{
		ret = m_interval_singleton(dq->val.val_float);
		goto dq_int_ret;
	}
	
	if (dq->type == M_DERIVED_QUANTITY_REFERENCE)
	{
		if (m_derived_quantity_refers_constant(dq))
		{
			if (dq->cached)
				ret = m_interval_singleton(dq->cached_val);
			else
				ret = m_interval_singleton(m_derived_quantity_compute(dq, NULL));
			
			goto dq_int_ret;
		}
		
		current = params;
		
		found = 0;
		while (current && !found)
		{
			if (current->data && strcmp(current->data->name, dq->val.ref_name) == 0)
			{
				rp = current->data;
				found = 1;
			}
			
			current = current->next;
		}
		
		if (found)
		{
			if (rp->min_dq && rp->max_dq && depth < DQ_BOUND_MAX_DEPTH)
				ret = m_interval_ab(m_derived_quantity_compute_rec(rp->min_dq, params, depth + 1), m_derived_quantity_compute_rec(rp->max_dq, params, depth + 1));
			else if (rp->min_dq && depth < DQ_BOUND_MAX_DEPTH)
				ret = m_interval_ab(m_derived_quantity_compute_rec(rp->min_dq, params, depth + 1), rp->max);
			else if (rp->max_dq && depth < DQ_BOUND_MAX_DEPTH)
				ret = m_interval_ab(rp->min, m_derived_quantity_compute_rec(rp->max_dq, params, depth + 1));
			else
				ret = m_interval_ab(rp->min, rp->max);
		}
		else
		{
			ret = m_interval_real_line();
		}
		
		goto dq_int_ret;
	}
	
	int arity = m_derived_quantity_arity(dq);
	
	if (arity == 1 && (!dq->val.sub_dqs || !dq->val.sub_dqs[0]))
	{
		ret = m_interval_real_line();
		goto dq_int_ret;
	}
	
	if (arity == 2 && (!dq->val.sub_dqs || !dq->val.sub_dqs[0] || !dq->val.sub_dqs[1]))
	{
		ret = m_interval_real_line();
		goto dq_int_ret;
	}
	
	monotonicity = m_dq_type_monotonicity(dq->type);
	
	if (arity >= 1) x_int = m_derived_quantity_compute_range_rec(dq->val.sub_dqs[0], params, depth + 1);
	if (arity >  1) y_int = m_derived_quantity_compute_range_rec(dq->val.sub_dqs[1], params, depth + 1);
	
	switch (dq->type)
	{
		case M_DERIVED_QUANTITY_NEG:
			ret = m_interval_ab(-x_int.b, -x_int.a);
			goto dq_int_ret;
		
		case M_DERIVED_QUANTITY_FCALL_ADD:
			ret = m_interval_ab(x_int.a + y_int.a, x_int.b + y_int.b);
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SQRT:
			ret = m_interval_ab(sqrt(x_int.a), sqrt(x_int.b));
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_LN:
			if (x_int.a <= 0)
				ret.a = -FLT_MAX;
			else
				ret.a = log(x_int.a);
			
			if (x_int.b <= 0)
				ret.b = -FLT_MAX;
			else
				ret.b = log(x_int.b);
			
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_ASIN:
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
			
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_ACOS:
			if (x_int.b < -1)
				ret.a = M_PI;
			else if (x_int.b > 1)
				ret.a = 0.0f;
			else
				ret.a = acos(x_int.b);
			
			if (x_int.b < -1)
				ret.b = M_PI;
			else if (x_int.b > 1)
				ret.b = 0.0f;
			else
				ret.b = acos(x_int.b);
			
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_ATAN:
			ret = m_interval_ab(atan(x_int.a), atan(x_int.b));
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_TANH:
			ret =  m_interval_ab(tanh(x_int.a), tanh(x_int.b));
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SINH:
			ret =  m_interval_ab(sinh(x_int.a), sin(x_int.b));
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_EXP:
			ret =  m_interval_ab(exp(x_int.a), exp(x_int.b));
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SUB:
			ret =  m_interval_ab(x_int.a - y_int.b, x_int.b - y_int.a);
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SQR:
			if (x_int.a < 0)
			{
				if (x_int.b > 0)
				{
					ret.a = 0.0;
				}
				else
				{
					ret.a = x_int.b * x_int.b;
				}
			}
			else
			{
				ret.a = x_int.a * x_int.a;
			}
			
			p1 = x_int.a * x_int.a;
			p2 = x_int.b * x_int.b;
			
			if (p2 > p1)
				ret.b = p2;
			else
				ret.b = p1;
			
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_COSH:
			if (x_int.a < 0)
			{
				if (x_int.b > 0)
				{
					ret.a = 1.0;
				}
				else
				{
					ret.a = cosh(x_int.b);
				}
			}
			else
			{
				ret.a = cosh(x_int.a);
			}
			
			p1 = cosh(x_int.a);
			p2 = cosh(x_int.b );
			
			if (p2 > p1)
				ret.b = p2;
			else
				ret.b = p1;
			
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_ABS:
			if (x_int.a < 0)
			{
				if (x_int.b > 0)
				{
					ret.a = 0.0;
				}
				else
				{
					ret.a = -x_int.b;
				}
			}
			else
			{
				ret.a = x_int.a;
			}
			
			p1 = fabs(x_int.a);
			p2 = fabs(x_int.b);
			
			if (p2 > p1)
				ret.b = p2;
			else
				ret.b = p1;
			
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_MUL:
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
			
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_DIV:
			// if y can cross 0, oh no
			if (y_int.a <= 0.0f && 0.0f <= y_int.b)
			{
				if (x_int.a > 0.0f)
				{
					ret = m_interval_real_line();
					goto dq_int_ret;
				}
				else if (x_int.b == 0.0f && x_int.a == 0.0f)
				{
					ret = m_interval_singleton(0.0f);
					goto dq_int_ret;
				}
			}
			
			// otherwise, clamp y away from 0 for safety
			if (y_int.a < 0.0f && y_int.a > -1e-20f)
				y_int.a = -1e-20f;
			if (y_int.a > 0.0f && y_int.a < 1e-20f)
				y_int.a = 1e-20f;
			
			if (y_int.b < 0.0f && y_int.b > -1e-20f)
				y_int.b = -1e-20f;
			if (y_int.b > 0.0f && y_int.b < 1e-20f)
				y_int.b = 1e-20f;
			
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
			
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_POW:
			
			// negative bases for powers cause sign nonsense.
			// ignore them. accept that results will be wrong
			if (x_int.a < 0)
				x_int.a = 0;
			if (x_int.b < 0)
				x_int.b = 0;
			
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
			
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_SIN:
			/* min */
			
			if (x_int.b - x_int.a > (2.0*M_PI))
			{
				ret = m_interval_ab(-1, 1);
				goto dq_int_ret;
			}
			
			k = (int)ceilf((x_int.a + M_PI/2) / (2.0*M_PI));
			
			if (-(0.5*M_PI) + k * (2.0*M_PI) <= x_int.b)
			{
				ret.a = -1;
			}
			else
			{
				p1 = sin(x_int.a);
				p2 = sin(x_int.b);
				
				p_c = 1;
				
				if (p2 < p1)
					ret.a = p2;
				else
					ret.a = p1;
			}
			
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
				
				if (p2 > p1)
					ret.b = p2;
				else
					ret.b = p1;
			}
			
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_COS:
			x_int.a = x_int.a + (0.5*M_PI);
			x_int.b = x_int.b + (0.5*M_PI);
			
			if (x_int.b - x_int.a > (2.0*M_PI))
			{
				ret = m_interval_ab(-1, 1);
				goto dq_int_ret;
			}
			
			k = (int)ceilf((x_int.a + M_PI/2) / (2.0*M_PI));
			
			if (-(0.5*M_PI) + k * (2.0*M_PI) <= x_int.b)
			{
				ret.a = -1;
			}
			else
			{
				p1 = sin(x_int.a);
				p2 = sin(x_int.b);
				
				p_c = 1;
				
				if (p2 < p1)
					ret.a = p2;
				else
					ret.a = p1;
			}
			
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
				
				if (p2 > p1)
					ret.b = p2;
				else
					ret.b = p1;
			}
			
			goto dq_int_ret;
			
		case M_DERIVED_QUANTITY_FCALL_TAN:
			if (x_int.b - x_int.a > (2.0*M_PI))
			{
				ret = m_interval_real_line();
			}
			else
			{
				k = (int)ceilf((x_int.a - (0.5*M_PI)) / (2.0*M_PI));
				
				if ((0.5*M_PI) + k * (2.0*M_PI) <= x_int.b)
				{
					ret = m_interval_real_line();
				}
				else
				{
					p1 = tan(x_int.a);
					p2 = tan(x_int.b);
					
					ret.a = (p1 < p2) ? p1 : p2;
					ret.b = (p1 > p2) ? p1 : p2;
				}
			}
			
			goto dq_int_ret;
		
		default:
			ret = m_interval_real_line();
			goto dq_int_ret;
	}
	
dq_int_ret:
	//printf("(depth %d) (%s) interval: [%.06f, %.06f]\n", depth, m_dq_type_to_str(dq->type), ret.a, ret.b);
	return ret;
}

m_interval m_derived_quantity_compute_range(m_derived_quantity *dq, m_parameter_pll *params)
{
	return m_derived_quantity_compute_range_rec(dq, params, 0);
}

#include "kest_int.h"

#ifndef PRINTLINES_ALLOWED
#define PRINTLINES_ALLOWED 0
#endif

static const char *FNAME = "kest_expr_parser.c";

int kest_expression_token_unary_type(char *token)
{
	if (!token) return 0;
	
	if (strcmp(token,  "-"   ) == 0) return KEST_EXPR_NEG;
	if (strcmp(token, "abs"  ) == 0) return KEST_EXPR_ABS;
	if (strcmp(token, "sqr"  ) == 0) return KEST_EXPR_SQR;
	if (strcmp(token, "sqrt" ) == 0) return KEST_EXPR_SQRT;
	if (strcmp(token, "exp"  ) == 0) return KEST_EXPR_EXP;
	if (strcmp(token, "ln"   ) == 0) return KEST_EXPR_LN;
	if (strcmp(token, "log"  ) == 0) return KEST_EXPR_LN;
	if (strcmp(token, "sin"  ) == 0) return KEST_EXPR_SIN;
	if (strcmp(token, "cos"  ) == 0) return KEST_EXPR_COS;
	if (strcmp(token, "tan"  ) == 0) return KEST_EXPR_TAN;
	if (strcmp(token, "sinh" ) == 0) return KEST_EXPR_SINH;
	if (strcmp(token, "cosh" ) == 0) return KEST_EXPR_COSH;
	if (strcmp(token, "tanh" ) == 0) return KEST_EXPR_TANH;
	if (strcmp(token, "asin" ) == 0) return KEST_EXPR_ASIN;
	if (strcmp(token, "acos" ) == 0) return KEST_EXPR_ACOS;
	if (strcmp(token, "atan" ) == 0) return KEST_EXPR_ATAN;
	if (strcmp(token, "log10") == 0) return KEST_EXPR_LOG10;
	if (strcmp(token, "erf"  ) == 0) return KEST_EXPR_ERF;
	
	return 0;
}

int kest_expression_token_infix_type(char *token)
{
	if (!token) return 0;
	
	if (strcmp(token, "+") == 0) return KEST_EXPR_ADD;
	if (strcmp(token, "-") == 0) return KEST_EXPR_SUB;
	if (strcmp(token, "*") == 0) return KEST_EXPR_MUL;
	if (strcmp(token, "/") == 0) return KEST_EXPR_DIV;
	if (strcmp(token, "^") == 0) return KEST_EXPR_POW;
	
	return 0;
}


int kest_expression_token_binary_function_type(char *token)
{
	if (!token) return 0;

	if (strcmp(token, "pow") == 0) return KEST_EXPR_POW;
	if (strcmp(token, "min") == 0) return KEST_EXPR_MIN;
	if (strcmp(token, "max") == 0) return KEST_EXPR_MAX;

	return 0;
}

int kest_expression_token_function_type(char *token, int arity)
{
	switch (arity)
	{
		case 1: return kest_expression_token_unary_type(token);
		case 2: return kest_expression_token_binary_function_type(token);
	}

	return 0;
}

int kest_expression_infix_operator_precedence(int infix_type)
{
	switch (infix_type)
	{
		case KEST_EXPR_ADD:
		case KEST_EXPR_SUB: return SUMSUB_PRECEDENCE;
		case KEST_EXPR_MUL:
		case KEST_EXPR_DIV: return MULDIV_PRECEDENCE;
		case KEST_EXPR_POW: return POWER_PRECEDENCE;
	}

    return -1;
}

// 1 for left, 0 for right
int kest_expression_infix_associativity(int infix_type)
{
	switch (infix_type)
	{
		case KEST_EXPR_ADD:
		case KEST_EXPR_SUB:
		case KEST_EXPR_MUL:
		case KEST_EXPR_DIV: return 1;
	}

	return 0;
}


kest_expression *kest_parse_expression_rec_pratt(kest_eff_parsing_state *ps,
    kest_token_ll *tokens,
    kest_token_ll **next_token,
    kest_token_ll *tokens_end,
    int min_binding_power,
    int depth);

kest_expression *kest_parse_expression_function_call(kest_eff_parsing_state *ps,
    kest_token_ll *function_token,
    kest_token_ll **next_token,
    kest_token_ll *tokens_end,
    int depth)
{
	if (!function_token || !function_token->data || !function_token->next)
		return NULL;

	kest_token_ll *current = function_token->next;
	kest_token_ll *nt = NULL;
	kest_expression *args[KEST_EXPR_MAX_ARITY];
	int argc = 0;
	int function_type = 0;

	if (!token_is_char(current->data, '('))
		return NULL;

	current = current->next;

	if (!current || current == tokens_end)
	{
		kest_parser_error_at(ps, function_token, "Malformed function call");
		return NULL;
	}

	while (current && current != tokens_end)
	{
		if (token_is_char(current->data, ')'))
			break;

		if (argc >= KEST_EXPR_MAX_ARITY)
		{
			kest_parser_error_at(ps, current, "Too many arguments to \"%s\"", function_token->data);
			return NULL;
		}

		args[argc] = kest_parse_expression_rec_pratt(ps,
				current,
				&nt,
				tokens_end,
				0,
				depth + 1);

		if (!args[argc])
			return NULL;

		argc++;
		current = nt;

		if (!current || current == tokens_end)
		{
			kest_parser_error_at(ps, function_token, "Malformed function call");
			return NULL;
		}

		if (token_is_char(current->data, ','))
		{
			current = current->next;

			if (!current || current == tokens_end || token_is_char(current->data, ')'))
			{
				kest_parser_error_at(ps, function_token, "Malformed function call");
				return NULL;
			}

			continue;
		}

		if (token_is_char(current->data, ')'))
			break;

		kest_parser_error_at(ps, current, "Expected \",\" or \")\"");
		return NULL;
	}

	if (!current || current == tokens_end || !token_is_char(current->data, ')'))
	{
		kest_parser_error_at(ps, function_token, "Malformed function call");
		return NULL;
	}

	function_type = kest_expression_token_function_type(function_token->data, argc);

	if (!function_type)
	{
		kest_parser_error_at(ps, function_token, "Unknown function \"%s\" with arity %d", function_token->data, argc);
		return NULL;
	}

	if (next_token)
		*next_token = current->next;

	switch (argc)
	{
		case 1: return kest_expr_new_unary(function_type, args[0]);
		case 2: return kest_expr_new_binary(function_type, args[0], args[1]);
	}

	return NULL;
}

kest_expression *kest_parse_expression_rec_pratt(kest_eff_parsing_state *ps,
    kest_token_ll *tokens,
    kest_token_ll **next_token,
    kest_token_ll *tokens_end,
    int min_binding_power,
    int depth)
{
	if (!tokens || !tokens->data)
		return NULL;
	
	int line = tokens->line;
	
	if (depth > KEST_EXPR_REC_MAX_DEPTH)
	{
		max_depth_bp(NULL);
		kest_parser_error_at(ps, tokens, "Expression too deep.");
		return NULL;
	}
	
	kest_token_ll *current = tokens;
	kest_token_ll *nt = NULL;
	kest_expression *lhs = NULL;
	kest_expression *rhs = NULL;
	kest_expression *bin = NULL;
	
	int precedence;
	int left_binding_power;
	int right_binding_power;
	
	
	int unary_type = kest_expression_token_unary_type(current->data);
	int infix_type;
	
	if (token_is_number(current->data))
	{
		lhs = kest_expr_new_const(token_to_float(tokens->data));
		if (!lhs) return NULL;
		
		current = current->next;
	}
	else if (strcmp(current->data, "(") == 0)
	{
		lhs = kest_parse_expression_rec_pratt(ps,
				  current->next,
				  &current,
				  tokens_end,
				  0,
				  depth + 1);
		
		if (!lhs) return NULL;
		
		if (!current || strcmp(current->data, ")") != 0)
		{
			kest_parser_error_at(ps, current, "Malformed expression");
			goto pratt_bail;
		}

		current = current->next;
	}
	else if (token_is_name(current->data)
		  && current->next
		  && token_is_char(current->next->data, '('))
	{
		lhs = kest_parse_expression_function_call(ps,
				current,
				&nt,
				tokens_end,
				depth + 1);

		if (!lhs) goto pratt_bail;

		current = nt;
	}
	else if (unary_type)
	{
		rhs = kest_parse_expression_rec_pratt(ps,
				current->next,
				&nt,
				tokens_end,
				UNARY_PRECEDENCE,
				depth + 1);
		
		if (!rhs) goto pratt_bail;
		
		lhs = kest_expr_new_unary(unary_type, rhs);
		
		if (!lhs) goto pratt_bail;
		
		current = nt;
	}
	else if (token_is_name(current->data))
	{
		lhs = kest_expr_new_reference(current->data);
		
		current = current->next;
	}
	else
	{
		kest_parser_error_at(ps, current, "Unexpected \"%s\"", current->data);
		goto pratt_bail;
	}
	
	while (current && current != tokens_end)
	{
		infix_type = kest_expression_token_infix_type(current->data);
		
		if (!infix_type) break;

		precedence = kest_expression_infix_operator_precedence(infix_type);
		left_binding_power = precedence;
		right_binding_power = precedence + kest_expression_infix_associativity(infix_type);

		if (left_binding_power < min_binding_power) break;

		current = current->next;

		rhs = kest_parse_expression_rec_pratt(ps,
				current,
				&nt,
				tokens_end,
				right_binding_power,
				depth + 1);

		if (!rhs) goto pratt_bail;
			
		bin = kest_expr_new_binary(infix_type, lhs, rhs);
		
		if (!bin) goto pratt_bail;
		
		lhs = bin;
		current = nt;
	}
	
	if (next_token)
		*next_token = current;

	return lhs;

pratt_bail:
	// free anything allocated .. ?
	return NULL;
}

kest_expression *kest_parse_expression(kest_eff_parsing_state *ps, kest_token_ll *tokens, kest_token_ll *tokens_end)
{
	kest_token_ll *next_token;
	kest_expression *expr = kest_parse_expression_rec_pratt(ps, tokens, &next_token, tokens_end, 0, 0);
	
	int anything = 0;
	kest_token_ll *check = next_token;
	
	if (expr)
	{
		kest_expression_detect_constants(expr);
	}
	
	ps->current_token = next_token;
	
	return expr;
}

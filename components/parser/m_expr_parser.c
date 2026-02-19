#include "m_int.h"

int m_expression_token_unary_type(char *token)
{
	if (!token) return 0;
	
	if (strcmp(token,  "-"  ) == 0) return M_EXPR_NEG;
	if (strcmp(token, "abs" ) == 0) return M_EXPR_ABS;
	if (strcmp(token, "sqr" ) == 0) return M_EXPR_SQR;
	if (strcmp(token, "sqrt") == 0) return M_EXPR_SQRT;
	if (strcmp(token, "exp" ) == 0) return M_EXPR_EXP;
	if (strcmp(token, "sin" ) == 0) return M_EXPR_SIN;
	if (strcmp(token, "cos" ) == 0) return M_EXPR_COS;
	if (strcmp(token, "tan" ) == 0) return M_EXPR_TAN;
	if (strcmp(token, "sinh") == 0) return M_EXPR_SINH;
	if (strcmp(token, "cosh") == 0) return M_EXPR_COSH;
	if (strcmp(token, "tanh") == 0) return M_EXPR_TANH;
	if (strcmp(token, "asin") == 0) return M_EXPR_ASIN;
	if (strcmp(token, "acos") == 0) return M_EXPR_ACOS;
	if (strcmp(token, "atan") == 0) return M_EXPR_ATAN;
	
	return 0;
}

int m_expression_token_infix_type(char *token)
{
	if (!token) return 0;
	
	if (strcmp(token, "+") == 0) return M_EXPR_ADD;
	if (strcmp(token, "-") == 0) return M_EXPR_SUB;
	if (strcmp(token, "*") == 0) return M_EXPR_MUL;
	if (strcmp(token, "/") == 0) return M_EXPR_DIV;
	if (strcmp(token, "^") == 0) return M_EXPR_POW;
	
	return 0;
}

int m_expression_infix_operator_precedence(int infix_type)
{
	switch (infix_type)
	{
		case M_EXPR_ADD:
		case M_EXPR_SUB: return SUMSUB_PRECEDENCE;
		case M_EXPR_MUL:
		case M_EXPR_DIV: return MULDIV_PRECEDENCE;
		case M_EXPR_POW: return POWER_PRECEDENCE;
	}

    return -1;
}

// 1 for left, 0 for right
int m_expression_infix_associativity(int infix_type)
{
	switch (infix_type)
	{
		case M_EXPR_ADD:
		case M_EXPR_SUB:
		case M_EXPR_MUL:
		case M_EXPR_DIV: return 1;
	}
	
	return 0;
}

m_expression *new_m_expression_from_tokens_rec_pratt(
    m_token_ll *tokens,
    m_token_ll **next_token,
    m_token_ll *tokens_end,
    int min_binding_power,
    int depth)
{
	if (!tokens || !tokens->data)
		return NULL;
	
	int line = tokens->line;
	
	if (depth > M_EXPR_REC_MAX_DEPTH)
	{
		printf("Error (line %d): expression too deep\n", line);
		return NULL;
	}
	
	m_token_ll *current = tokens;
	m_token_ll *nt = NULL;
	m_expression *lhs = NULL;
	m_expression *rhs = NULL;
	m_expression *bin = NULL;
	
	int precedence;
	int left_binding_power;
	int right_binding_power;
	
	
	int unary_type = m_expression_token_unary_type(current->data);
	int infix_type;
	
	if (token_is_number(current->data))
	{
		lhs = new_m_expression_const(token_to_float(tokens->data));
		if (!lhs) return NULL;
		
		current = current->next;
	}
	else if (strcmp(current->data, "(") == 0)
	{
		lhs = new_m_expression_from_tokens_rec_pratt(
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
		rhs = new_m_expression_from_tokens_rec_pratt(
				current->next,
				&nt,
				tokens_end,
				UNARY_PRECEDENCE,
				depth + 1);
		
		if (!rhs) goto pratt_bail;
		
		lhs = new_m_expression_unary(unary_type, rhs);
		
		if (!lhs) goto pratt_bail;
		
		current = nt;
	}
	else if (token_is_name(current->data))
	{
		lhs = new_m_expression_reference(current->data);
		
		current = current->next;
	}
	else
	{
		printf("Error (line %d): unexpected \"%s\"\n", line, current->data);
		goto pratt_bail;
	}
	
	while (current && current != tokens_end)
	{
		infix_type = m_expression_token_infix_type(current->data);
		
		if (!infix_type) break;

		precedence = m_expression_infix_operator_precedence(infix_type);
		left_binding_power = precedence;
		right_binding_power = precedence + m_expression_infix_associativity(infix_type);

		if (left_binding_power < min_binding_power) break;

		current = current->next;

		rhs = new_m_expression_from_tokens_rec_pratt(
				current,
				&nt,
				tokens_end,
				right_binding_power,
				depth + 1);

		if (!rhs) goto pratt_bail;
			
		bin = new_m_expression_binary(infix_type, lhs, rhs);
		
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

m_expression *new_m_expression_from_tokens(m_token_ll *tokens, m_token_ll *tokens_end)
{
	m_token_ll *next_token;
	m_expression *expr = new_m_expression_from_tokens_rec_pratt(tokens, &next_token, tokens_end, 0, 0);
	
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
	
	return expr;
}

#ifndef M_PARSE_EXPR_H_
#define M_PARSE_EXPR_H_

#define SUMSUB_PRECEDENCE 10
#define MULDIV_PRECEDENCE 20
#define  UNARY_PRECEDENCE 30
#define  POWER_PRECEDENCE 40

m_expression *new_m_expression_from_tokens(m_token_ll *tokens, m_token_ll *tokens_end);

#endif

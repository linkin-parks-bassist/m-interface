#ifndef M_PARSE_EXPR_H_
#define M_PARSE_EXPR_H_

#define SUMSUB_PRECEDENCE 10
#define MULDIV_PRECEDENCE 20
#define  UNARY_PRECEDENCE 30
#define  POWER_PRECEDENCE 40

#define M_EFF_PARSER_MEM_POOL_SIZE_KB 128

m_expression *m_parse_expression(m_eff_parsing_state *ps, m_token_ll *tokens, m_token_ll *tokens_end);

#endif

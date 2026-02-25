#ifndef M_EXPR_SCOPE_H_
#define M_EXPR_SCOPE_H_

#define M_SCOPE_ENTRY_TYPE_EXPR 	0
#define M_SCOPE_ENTRY_TYPE_PARAM 	1

struct m_expression;
struct m_parameter;

typedef struct {
	const char *name;
	int type;
	union {
		struct m_expression *expr;
		struct m_parameter *param;
	} val;
} m_expr_scope_entry;

m_expr_scope_entry *m_new_expr_scope_entry_expr(const char *name, struct m_expression *expr);
m_expr_scope_entry *m_new_expr_scope_entry_param(struct m_parameter *param);

DECLARE_LINKED_PTR_LIST(m_expr_scope_entry);

// like, really this should be a hash table but, i honestly 
// don't expect them to get particularly large so im
// just going to use my default, beloved linked list
typedef struct {
	m_expr_scope_entry_pll *entries;
} m_expr_scope;

m_expr_scope *m_new_expr_scope();
int m_expr_scope_add_expr(m_expr_scope *scope, const char *name, struct m_expression *expr);
int m_expr_scope_add_param(m_expr_scope *scope, struct m_parameter *param);

m_expr_scope_entry *m_expr_scope_fetch(m_expr_scope *scope, const char *name);

#endif

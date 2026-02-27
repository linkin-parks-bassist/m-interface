#ifndef M_EXPR_SCOPE_H_
#define M_EXPR_SCOPE_H_

#define M_SCOPE_ENTRY_TYPE_EXPR 	0
#define M_SCOPE_ENTRY_TYPE_PARAM 	1
#define M_SCOPE_ENTRY_TYPE_SETTING 	2

struct m_expression;
struct m_parameter;
struct m_setting;

typedef struct {
	const char *name;
	int type;
	union {
		struct m_expression *expr;
		struct m_parameter *param;
		struct m_setting *setting;
	} val;
} m_expr_scope_entry;

m_expr_scope_entry *m_new_expr_scope_entry_expr(const char *name, struct m_expression *expr);
m_expr_scope_entry *m_new_expr_scope_entry_param(struct m_parameter *param);
m_expr_scope_entry *m_new_expr_scope_entry_setting(struct m_setting *setting);

DECLARE_LINKED_PTR_LIST(m_expr_scope_entry);

// like, really this should be a hash table but, i honestly 
// don't expect them to get particularly large so im
// just going to use my default, beloved linked list
typedef struct {
	m_expr_scope_entry_pll *entries;
} m_expr_scope;

m_expr_scope *m_new_expr_scope();
int m_expr_scope_init(m_expr_scope *scope);
int m_expr_scope_add_expr(m_expr_scope *scope, const char *name, struct m_expression *expr);
int m_expr_scope_add_param(m_expr_scope *scope, struct m_parameter *param);
int m_expr_scope_add_setting(m_expr_scope *scope,struct m_setting *setting);

struct m_parameter_pll;
struct m_setting_pll;

int m_expr_scope_add_params(m_expr_scope *scope, struct m_parameter_pll *params);
int m_expr_scope_add_settings(m_expr_scope *scope, struct m_setting_pll *settings);

m_expr_scope_entry *m_expr_scope_fetch(m_expr_scope *scope, const char *name);

#endif

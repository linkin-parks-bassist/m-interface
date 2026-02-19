#ifndef M_INT_PARAMETER_H_
#define M_INT_PARAMETER_H_

#include <stddef.h>
#include <stdint.h>

#define PARAM_WIDGET_VIRTUAL_POT  0
#define PARAM_WIDGET_HSLIDER 	  1
#define PARAM_WIDGET_VSLIDER 	  2
#define PARAM_WIDGET_VSLIDER_TALL 3

#define PARAM_NAM_ENG_MAX_LEN 254

#define PARAMETER_SCALE_LINEAR		0
#define PARAMETER_SCALE_LOGARITHMIC	1

typedef struct m_parameter_id
{
	uint16_t profile_id;
	uint16_t transformer_id;
	uint16_t parameter_id;
} m_parameter_id;

struct m_derived_quantity;

typedef struct m_parameter
{
	float value;
	float min;
	float max;
	
	struct m_derived_quantity *min_dq;
	struct m_derived_quantity *max_dq;
	
	int scale;
	
	int updated;
	float old_value;
	float new_value;
	
	float max_velocity;
	
	m_parameter_id id;
	
	float factor;
	
	int widget_type;
	char *name;
	char *name_internal;
	char *units;
	
	int group;
} m_parameter;

m_parameter *new_m_parameter_wni(const char *name, char *name_internal, float value, float min, float max);

typedef struct m_parameter_pll {
	m_parameter *data;
	struct m_parameter_pll *next;
} m_parameter_pll;

m_parameter_pll *m_parameter_pll_append(m_parameter_pll *pll, m_parameter *param);
int m_parameter_pll_safe_append(m_parameter_pll **list_ptr, m_parameter *x);

int m_extract_parameters(m_parameter_pll **list, struct m_ast_node *sect);
int m_parameters_assign_ids(m_parameter_pll *list);

void clone_parameter(m_parameter *dest, m_parameter *src);
m_parameter *m_parameter_make_clone(m_parameter *src);

typedef struct m_setting_option
{
	uint16_t value;
	const char *name;
} m_setting_option;

typedef struct m_setting_id
{
	uint16_t profile_id;
	uint16_t transformer_id;
	uint16_t setting_id;
} m_setting_id;

#define TRANSFORMER_SETTING_ENUM 	0
#define TRANSFORMER_SETTING_BOOL 	1
#define TRANSFORMER_SETTING_INT 	2

#define TRANSFORMER_SETTING_PAGE_SETTINGS 0
#define TRANSFORMER_SETTING_PAGE_MAIN 	  1

typedef struct m_setting
{
	int value;
	
	int updated;
	int old_value;
	int new_value;
	
	int type;
	int page;
	
	m_setting_id id;
	
	int min;
	int max;
	
	int n_options;
	m_setting_option *options;
	
	int widget_type;
	const char *name;
	const char *units;
	
	int group;
} m_setting;

#endif


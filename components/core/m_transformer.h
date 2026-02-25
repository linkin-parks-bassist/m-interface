#ifndef M_INT_TRANSFORMER_H_
#define M_INT_TRANSFORMER_H_

#define TRANSFORMER_MODE_FULL_SPECTRUM  0
#define TRANSFORMER_MODE_UPPER_SPECTRUM	1
#define TRANSFORMER_MODE_LOWER_SPECTRUM	2
#define TRANSFORMER_MODE_BAND			3

#define TRANSFORMER_WET_MIX_PID	0xFFFF

#define TRANSFORMER_BAND_LP_CUTOFF_PID 	0xFFFE
#define TRANSFORMER_BAND_HP_CUTOFF_PID 	0xFFFD
#define TRANSFORMER_BAND_MODE_SID 		0xFFFF

struct m_profile;
struct m_ui_page;

typedef struct m_transformer
{
	uint16_t type;
	uint16_t id;
	
	m_parameter wet_mix;
	
	m_setting band_mode;
	m_parameter band_lp_cutoff;
	m_parameter band_hp_cutoff;
	m_parameter band_center;
	m_parameter band_width;
	
	int position;
	int block_position;
	
	m_parameter_pll *parameters;
	m_setting_pll *settings;
	
	struct m_profile *profile;
	
	#ifdef M_ENABLE_UI
	struct m_ui_page *view_page;
	#endif
	
	m_effect_desc *eff;
	m_expr_scope *scope;
	
	#ifdef M_USE_FREERTOS
	SemaphoreHandle_t mutex;
	#endif
} m_transformer;

const char *m_transformer_name(m_transformer *trans);

DECLARE_LINKED_PTR_LIST(m_transformer);

int init_transformer(m_transformer *trans);

int transformer_set_id(m_transformer *trans, uint16_t profile_id, uint16_t transformer_id);
int transformer_rectify_param_ids(m_transformer *trans);

m_parameter *transformer_add_parameter(m_transformer *trans);
m_setting *transformer_add_setting(m_transformer *trans);

int init_default_transformer_by_type(m_transformer *trans, uint16_t type, uint16_t profile_id, uint16_t transformer_id);
int init_transformer_from_effect_desc(m_transformer *trans, m_effect_desc *eff);

#ifdef M_ENABLE_UI
int transformer_init_ui_page(m_transformer *trans, struct m_ui_page *parent);

void add_transformer_from_menu(lv_event_t *e);
#endif

int request_append_transformer(uint16_t type, m_transformer *local);
#ifdef USE_TEENSY
void transformer_receive_id(m_message message, m_response response);
#endif

int clone_transformer(m_transformer *dest, m_transformer *src);
void free_transformer(m_transformer *trans);

m_parameter *transformer_get_parameter(m_transformer *trans, int n);
m_setting *transformer_get_setting(m_transformer *trans, int n);

int m_fpga_transfer_batch_append_transformer(
		m_transformer *trans,
		const m_eff_resource_report *cxt,
		m_eff_resource_report *report,
		m_fpga_transfer_batch *batch
	);


int m_transformer_update_fpga_registers(m_transformer *trans);

m_expr_scope *m_transformer_create_scope(m_transformer *trans);

#endif

#ifndef M_PROFILE_H_
#define M_PROFILE_H_

#define PROFILE_NAME_MAX_LEN 128

#define M_PROFILE_MUTEX_TIMEOUT_MS 10


struct m_glide_button_pll;
struct m_menu_item_pll;

struct m_sequence;

typedef struct m_profile
{
	char *name;
	char *fname;
	uint16_t id;
	
	m_pipeline pipeline;
	
	#ifdef M_ENABLE_SEQUENCES
	struct m_sequence *sequence;
	#endif
	
	int active;
	int unsaved_changes;
	
	#ifdef M_ENABLE_REPRESENTATIONS
	m_representation_pll *representations;
	#endif
	
	#ifdef M_ENABLE_UI
	struct m_ui_page *view_page;
	#endif
	
	m_parameter volume;
	
	#ifdef M_USE_FREERTOS
	SemaphoreHandle_t mutex;
	#endif
} m_profile;

DECLARE_LINKED_PTR_LIST(m_profile);

int init_m_profile(m_profile *profile);
int init_m_pipeline(m_pipeline *pipeline);

int profile_set_id(m_profile *profile, uint16_t id);

int m_profile_set_default_name_from_id(m_profile *profile);

m_transformer *m_profile_append_transformer_type(m_profile *profile, uint16_t type);
m_transformer *m_profile_append_transformer_eff(m_profile *profile, m_effect_desc *eff);
int m_profile_remove_transformer(m_profile *profile, uint16_t id);

int m_profile_move_transformer(m_profile *profile, int new_pos, int old_pos);

int clone_profile(m_profile *dest, m_profile *src);
void gut_profile(m_profile *profile);
void free_profile(m_profile *profile);

int m_profile_set_active(m_profile *profile);
int m_profile_set_inactive(m_profile *profile);

struct m_menu_item;

int m_profile_add_representation(m_profile *profile, m_representation *rep);
int m_profile_remove_representation(m_profile *profile, m_representation *rep);

int m_profile_add_name_representation(m_profile *profile, m_representation *rep);
int m_profile_remove_name_representation(m_profile *profile, m_representation *rep);

int m_profile_add_id_representation(m_profile *profile, m_representation *rep);
int m_profile_remove_id_representation(m_profile *profile, m_representation *rep);

#ifdef M_ENABLE_GLOBAL_CONTEXT
m_profile *create_new_profile();
#endif

int m_profile_save(m_profile *profile);

int m_profile_update_representations		(m_profile *profile);
int m_profile_update_name_representations	(m_profile *profile);
int m_profile_update_id_representations		(m_profile *profile);

int m_profile_create_fpga_transfer_batch(m_profile *profile, m_fpga_transfer_batch *batch);
int m_profile_if_active_update_fpga(m_profile *profile);
int m_profile_program_fpga(m_profile *profile);

#endif

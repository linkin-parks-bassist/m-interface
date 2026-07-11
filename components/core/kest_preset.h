#ifndef KEST_PRESET_H_
#define KEST_PRESET_H_

#define PRESET_NAME_MAX_LEN 128

#define KEST_PRESET_MUTEX_TIMEOUT_MS 10

struct kest_sequence;
struct kest_active_button;

typedef struct kest_preset
{
	char *name;
	char fname[KEST_FILENAME_LEN];
	uint16_t id;
	
	int has_fname;
	kest_pipeline pipeline;
	
	#ifdef KEST_ENABLE_SEQUENCES
	struct kest_sequence *sequence;
	#endif
	
	int alive;
	int active;
	int pending;
	int unsaved_changes;
	
	#ifdef KEST_ENABLE_REPRESENTATIONS
	kest_representation_pll *representations;
	kest_representation file_rep;
	#endif
	
	#ifdef KEST_ENABLE_UI
	struct kest_ui_page *view_page;
	struct kest_active_button *button;
	#endif
	
	kest_parameter volume;
	
	#ifdef KEST_USE_FREERTOS
	SemaphoreHandle_t mutex;
	#endif
} kest_preset;

DECLARE_LINKED_PTR_LIST(kest_preset);

int init_m_preset(kest_preset *preset);
int init_m_pipeline(kest_pipeline *pipeline);

int preset_set_id(kest_preset *preset, uint16_t id);
int kest_preset_rectify_ids(kest_preset *preset);

int kest_preset_set_default_name_from_id(kest_preset *preset);

kest_effect *kest_preset_append_effect_eff(kest_preset *preset, kest_effect_desc *eff);
int kest_preset_remove_effect(kest_preset *preset, uint16_t id);

int kest_preset_move_effect(kest_preset *preset, int new_pos, int old_pos);

int clone_preset(kest_preset *dest, kest_preset *src);
void gut_preset(kest_preset *preset);
void free_preset(kest_preset *preset);
void kest_free_preset(kest_preset *preset);

int kest_preset_set_active(kest_preset *preset);
int kest_preset_set_inactive(kest_preset *preset);

int kest_preset_clear_pending(kest_preset *preset);

struct kest_menu_item;

int kest_preset_add_representation(kest_preset *preset, kest_representation *rep);
int kest_preset_remove_representation(kest_preset *preset, kest_representation *rep);

int kest_preset_add_name_representation(kest_preset *preset, kest_representation *rep);
int kest_preset_remove_name_representation(kest_preset *preset, kest_representation *rep);

int kest_preset_add_id_representation(kest_preset *preset, kest_representation *rep);
int kest_preset_remove_id_representation(kest_preset *preset, kest_representation *rep);

#ifdef KEST_ENABLE_GLOBAL_CONTEXT
kest_preset *create_new_preset();
#endif

int kest_preset_save(kest_preset *preset);

int kest_preset_update_representations		(kest_preset *preset);
int kest_preset_update_name_representations	(kest_preset *preset);
int kest_preset_update_id_representations		(kest_preset *preset);

int kest_preset_create_fpga_transfer_batch(kest_preset *preset, kest_fpga_transfer_batch *batch);
int kest_preset_if_active_update_fpga(kest_preset *preset);
int kest_preset_if_active_reprogram_fpga(kest_preset *preset);
int kest_preset_program_fpga(kest_preset *preset);

int kest_preset_update_positions(kest_preset *preset);

void kest_preset_file_rep_update(void *representer, void *representee);

kest_effect *kest_preset_get_effect_by_id(kest_preset *preset, int id);

int kest_preset_activate_dma(kest_preset *preset);
int kest_preset_deactivate_dma(kest_preset *preset);

int kest_preset_activate_lfos(kest_preset *preset);
int kest_preset_deactivate_lfos(kest_preset *preset);

DECLARE_POOL(kest_preset);
extern kest_allocator kest_preset_allocator;
extern kest_preset_pool kest_preset_mem_pool;

int kest_preset_handle_name_change(kest_preset *preset);

#endif

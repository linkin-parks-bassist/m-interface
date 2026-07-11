#ifndef KEST_INT_SEQUENCE_H_
#define KEST_INT_SEQUENCE_H_


typedef struct seq_kest_preset_pll
{
	kest_preset *data;
	struct seq_kest_preset_pll *next;
	struct seq_kest_preset_pll *prev;
} seq_kest_preset_pll;

typedef struct kest_sequence
{
	char *name;
	int active;
	
	seq_kest_preset_pll *presets;
	seq_kest_preset_pll *position;
	
	#ifdef KEST_ENABLE_UI
	kest_ui_page *view_page;
	#endif
	
	struct kest_menu_item_pll *listings;
	char fname[KEST_FILENAME_LEN];
	int has_fname;
	
	int unsaved_changes;
	int main_sequence;
	
	#ifdef KEST_ENABLE_REPRESENTATIONS
	kest_representation_pll *representations;
	kest_representation file_rep;
	#endif
} kest_sequence;

DECLARE_LINKED_PTR_LIST(kest_sequence);

typedef kest_sequence_pll sequence_ll;

int init_m_sequence(kest_sequence *sequence);

int kest_sequence_set_default_name(kest_sequence *sequence, uint32_t n);

int sequence_append_preset(kest_sequence *sequence, kest_preset *preset);
seq_kest_preset_pll *sequence_append_preset_rp(kest_sequence *sequence, kest_preset *preset);
int kest_sequence_move_preset(kest_sequence *sequence, int pos, int new_pos);

int kest_sequence_remove_preset(kest_sequence *sequence, kest_preset *preset);
int kest_sequence_delete_preset(kest_sequence *sequence, kest_preset *preset);

int kest_sequence_add_menu_listing(kest_sequence *sequence, struct kest_menu_item *listing);

void free_sequence(kest_sequence *sequence);

int kest_sequence_begin(kest_sequence *sequence);
int kest_sequence_begin_at(kest_sequence *sequence, kest_preset *preset);
int kest_sequence_regress(kest_sequence *sequence);
int kest_sequence_advance(kest_sequence *sequence);
int kest_sequence_stop(kest_sequence *sequence);
int kest_sequence_stop_from_preset(kest_sequence *sequence);

int kest_sequence_activate_at(kest_sequence *sequence, kest_preset *preset);

int kest_sequence_add_representation(kest_sequence *sequence, kest_representation *rep);
int kest_sequence_update_representations(kest_sequence *sequence);

void kest_sequence_file_rep_update(void *representer, void *representee);

DECLARE_POOL(kest_sequence);
extern kest_allocator kest_sequence_allocator;
extern kest_sequence_pool kest_sequence_mem_pool;

int kest_sequence_handle_name_change(kest_sequence *sequence);

#endif

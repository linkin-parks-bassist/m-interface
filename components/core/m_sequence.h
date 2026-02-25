#ifndef M_INT_SEQUENCE_H_
#define M_INT_SEQUENCE_H_


typedef struct seq_profile_ll
{
	m_profile *data;
	struct seq_profile_ll *next;
	struct seq_profile_ll *prev;
} seq_profile_ll;

typedef struct m_sequence
{
	char *name;
	int active;
	
	seq_profile_ll *profiles;
	seq_profile_ll *position;
	
	#ifdef M_ENABLE_UI
	m_ui_page *view_page;
	#endif
	
	struct m_menu_item_pll *listings;
	char *fname;
	
	int unsaved_changes;
	int main_sequence;
	
	m_representation_pll *representations;
} m_sequence;

DECLARE_LINKED_PTR_LIST(m_sequence);

typedef m_sequence_pll sequence_ll;

int init_m_sequence(m_sequence *sequence);

int sequence_append_profile(m_sequence *sequence, m_profile *profile);
seq_profile_ll *sequence_append_profile_rp(m_sequence *sequence, m_profile *profile);
int sequence_move_profile(m_sequence *sequence, int pos, int new_pos);

int m_sequence_add_menu_listing(m_sequence *sequence, struct m_menu_item *listing);

void free_sequence(m_sequence *sequence);

int m_sequence_begin(m_sequence *sequence);
int m_sequence_begin_at(m_sequence *sequence, m_profile *profile);
int m_sequence_regress(m_sequence *sequence);
int m_sequence_advance(m_sequence *sequence);
int m_sequence_stop(m_sequence *sequence);
int m_sequence_stop_from_profile(m_sequence *sequence);

int m_sequence_activate_at(m_sequence *sequence, m_profile *profile);

int m_sequence_add_representation(m_sequence *sequence, m_representation *rep);
int m_sequence_update_representations(m_sequence *sequence);

#endif

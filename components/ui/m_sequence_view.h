#ifndef M_INT_SEQUENCE_VIEW_H_
#define M_INT_SEQUENCE_VIEW_H_

typedef struct
{
	m_sequence *sequence;
	
	m_active_button_array *array;
	
	m_button *play;
	m_button *plus;
	m_button *save;
	
	m_representation rep;
} m_sequence_view_str;

int init_sequence_view	   (m_ui_page *page);
int configure_sequence_view(m_ui_page *page, void *data);
int create_sequence_view_ui(m_ui_page *page);
int refresh_sequence_view  (m_ui_page *page);
int free_sequence_view_ui  (m_ui_page *page);
int sequence_view_free_all (m_ui_page *page);

int sequence_view_append_profile(m_ui_page *page, m_profile *sequence);

int create_sequence_view_for(m_sequence *sequence);

void sequence_view_rep_update(void *representer, void *representee);
void sequence_view_profile_button_rep_update(void *representer, void *representee);

#endif

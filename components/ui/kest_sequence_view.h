#ifndef KEST_INT_SEQUENCE_VIEW_H_
#define KEST_INT_SEQUENCE_VIEW_H_

struct kest_menu_item;

typedef struct
{
	kest_sequence *sequence;
	
	kest_active_button_array *array;
	
	kest_button *play;
	kest_button *plus;
	kest_button *save;
	
	struct kest_menu_item *menu_item;
} kest_sequence_view_str;

int init_sequence_view	   (kest_ui_page *page);
int configure_sequence_view(kest_ui_page *page, void *data);
int create_sequence_view_ui(kest_ui_page *page);
int refresh_sequence_view  (kest_ui_page *page);
int free_sequence_view_ui  (kest_ui_page *page);
int sequence_view_free_all (kest_ui_page *page);

int sequence_view_append_preset(kest_ui_page *page, kest_preset *sequence);

int create_sequence_view_for(kest_sequence *sequence);

#endif

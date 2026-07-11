#include "kest_int.h"

#ifndef PRINTLINES_ALLOWED
#define PRINTLINES_ALLOWED 0
#endif

static const char *FNAME = "kest_sequence_view.c";

int create_sequence_view_for(kest_sequence *sequence)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	sequence->view_page = kest_alloc(sizeof(kest_ui_page));
	
	if (!sequence->view_page)
		return ERR_ALLOC_FAIL;
	
	init_sequence_view(sequence->view_page);
	configure_sequence_view(sequence->view_page, sequence);
	
	return NO_ERROR;
}

int seq_view_clicked_cb(kest_active_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	kest_preset *preset = button->data;
	
	if (!preset)
		return ERR_BAD_ARGS;
	
	if (!preset->view_page)
	{
		if (!create_preset_view_for(preset))
			return ERR_ALLOC_FAIL;
	}
	
	enter_ui_page_forwards(preset->view_page);
	
	return NO_ERROR;
}

int seq_view_moved_cb(kest_active_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	kest_preset *preset = (kest_preset*)button->data;
	
	if (!button->array || !button->array->parent || !button->array->parent->data_struct)
		return ERR_BAD_ARGS;
	
	kest_sequence_view_str *str = (kest_sequence_view_str*)button->array->parent->data_struct;
	
	kest_sequence *sequence = str->sequence;
	
	kest_sequence_move_preset(sequence, button->index, button->prev_index);
	
	return NO_ERROR;
}

int init_sequence_view(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	init_ui_page(page);
	
	page->type = KEST_UI_PAGE_SEQ_VIEW;
	
	kest_sequence_view_str *str = kest_alloc(sizeof(kest_sequence_view_str));
	
	if (!str)
		return ERR_ALLOC_FAIL;
	
	memset(str, 0, sizeof(kest_sequence_view_str));
	
	page->data_struct = str;
	
	page->configure = configure_sequence_view;
	page->create_ui = create_sequence_view_ui;
	
	str->array = kest_active_button_array_new();
	
	kest_active_button_array_set_length(str->array, 32);
	
	str->array->flags |= KEST_ACTIVE_BUTTON_ARRAY_FLAG_DELETEABLE;
	str->array->flags |= KEST_ACTIVE_BUTTON_ARRAY_FLAG_MOVEABLE;
	
	str->array->clicked_cb = seq_view_clicked_cb;
	str->array->moved_cb   = seq_view_moved_cb;
	
	return NO_ERROR;
}

void seq_play_cb(lv_event_t *e)
{
	kest_ui_page *page = lv_event_get_user_data(e);
	
	if (!page)
		return;
	
	kest_sequence_view_str *str = (kest_sequence_view_str*)page->data_struct;
	
	if (!str)
		return;
	
	if (!str->sequence)
	{
		KEST_PRINTF("ERROR: no sequence\n");
		return;
	}
	
	if (!str->sequence->active)
		kest_sequence_begin(str->sequence);
	
	return;
}

void seq_plus_cb(lv_event_t *e)
{
	KEST_PRINTF("seq_plus_cb, line %d\n", __LINE__);
	
	kest_ui_page *page = lv_event_get_user_data(e);
	
	if (!page) return;
	
	KEST_PRINTF("seq_plus_cb, line %d\n", __LINE__);
	
	kest_sequence_view_str *str = (kest_sequence_view_str*)page->data_struct;
	
	if (!str) return;
	
	KEST_PRINTF("seq_plus_cb, line %d\n", __LINE__);
	
	kest_preset *new_preset = create_new_preset();
	
	if (!new_preset) return;
	
	KEST_PRINTF("seq_plus_cb, line %d\n", __LINE__);
	
	new_preset->sequence = str->sequence;
	
	seq_kest_preset_pll *node = sequence_append_preset_rp(str->sequence, new_preset);
	
	if (!node) return;
	
	KEST_PRINTF("seq_plus_cb, line %d\n", __LINE__);
	
	kest_active_button *button = kest_active_button_array_append_new(str->array, node->data, node->data->name);
	
	new_preset->button = button;
	
	KEST_PRINTF("seq_plus_cb, line %d\n", __LINE__);
	if (page->ui_created)
		kest_active_button_create_ui(button, page->container);
	
	return;
}

void seq_save_cb(lv_event_t *e)
{
	kest_ui_page *page = lv_event_get_user_data(e);
	
	if (!page)
		return;
	
	kest_sequence_view_str *str = (kest_sequence_view_str*)page->data_struct;
	
	if (!str)
		return;
	
	kest_sequence *sequence = str->sequence;
	
	if (sequence == &global_cxt.main_sequence)
	{
		cxt_save_all_presets(&global_cxt);
	}
	else
	{
		save_sequence(sequence);
	}
	
	return;
}

int seq_view_free_cb(kest_active_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	
	
	return NO_ERROR;
}

int seq_view_delete_cb(kest_active_button *button)
{
	KEST_PRINTF("seq_view_delete_cb\n");
	if (!button) return ERR_NULL_PTR;
	if (!button->data) return ERR_BAD_ARGS;
	
	if (!button->array) return ERR_BAD_ARGS;
	if (!button->array->parent) return ERR_BAD_ARGS;
	if (!button->array->parent->data_struct) return ERR_BAD_ARGS;
	
	kest_sequence_view_str *str = (kest_sequence_view_str*)button->array->parent->data_struct;
	kest_sequence *sequence = str->sequence;
	
	if (!sequence) return ERR_NULL_PTR;
	
	kest_preset *preset = (kest_preset*)button->data;
	
	kest_sequence_delete_preset(sequence, preset);
	
	// no dangling pointers tyvm
	button->data = NULL;
	
	
	KEST_PRINTF("seq_view_delete_cb done\n");
	return NO_ERROR;
}

void sequence_view_set_name(lv_event_t *e)
{
	kest_ui_page *page = (kest_ui_page*)lv_event_get_user_data(e);
	kest_sequence_view_str *str = (kest_sequence_view_str*)page->data_struct;
	
	const char *new_name = lv_textarea_get_text(page->panel->title);
	
	if (str->sequence->name)
		kest_free(str->sequence->name);
	
	str->sequence->name = kest_strndup(new_name, PRESET_NAME_MAX_LEN);
	
	kest_event_log(kest_event_sequence_name_change(str->sequence));
	
	return;
}

void sequence_view_revert_name(lv_event_t *e)
{
	kest_ui_page *page = (kest_ui_page*)lv_event_get_user_data(e);
	kest_sequence_view_str *str = (kest_sequence_view_str*)page->data_struct;
	
	lv_textarea_set_text(page->panel->title, str->sequence->name);
	
	lv_obj_clear_state(page->panel->title, LV_STATE_FOCUSED);
	lv_obj_add_state(page->container, LV_STATE_FOCUSED);
	
	hide_keyboard();
	
	return;
}

int configure_sequence_view(kest_ui_page *page, void *data)
{
	if (!page)
		return ERR_NULL_PTR;
	
	kest_sequence *sequence = (kest_sequence*)data;
	
	if (!sequence)
		return ERR_BAD_ARGS;
	
	kest_sequence_view_str *str = (kest_sequence_view_str*)page->data_struct;
	
	if (!str)
		return ERR_ALLOC_FAIL;
	
	str->sequence = sequence;
	
	sequence->view_page = page;
	
	page->panel = new_panel();
	
	ui_page_add_back_button(page);
	
	str->array->parent = page;
	
	//str->array->free_cb 	 = seq_view_free_cb;
	str->array->delete_cb  	 = seq_view_delete_cb;
	str->array->clicked_cb   = seq_view_clicked_cb;
	str->array->moved_cb 	 = seq_view_moved_cb;
	
	str->play = ui_page_add_bottom_button(page, LV_SYMBOL_PLAY, seq_play_cb);
	str->plus = ui_page_add_bottom_button(page, LV_SYMBOL_PLUS, seq_plus_cb);
	str->save = ui_page_add_bottom_button(page, LV_SYMBOL_SAVE, seq_save_cb);
	
	page->panel->text = sequence->name;
	
	if (sequence != &global_cxt.main_sequence)
		ui_page_set_title_rw(page, sequence_view_set_name, sequence_view_revert_name);
	
	seq_kest_preset_pll *current = sequence->presets;
	kest_preset *preset = NULL;
	
	kest_active_button *button = NULL;
	kest_preset_view_str *preset_view_str = NULL;
	
	while (current)
	{
		preset = current->data;
		if (preset)
		{
			if (!preset->view_page)
			{
				create_preset_view_for(preset);
			}
			
			if (preset->view_page)
			{
				preset->view_page->parent = page;
			}
			
			button = kest_active_button_array_append_new(str->array, current->data, current->data->name);
			
			if (!button)
			{
				// die
				return ERR_ALLOC_FAIL;
			}
			
			preset->button = button;
		}
		
		current = current->next;
	}
	
	page->configured = 1;
	
	return NO_ERROR;
}

int create_sequence_view_ui(kest_ui_page *page)
{
	KEST_PRINTF("create_sequence_view_ui\n");
	if (!page)
		return ERR_NULL_PTR;
	
	kest_sequence_view_str *str = (kest_sequence_view_str*)page->data_struct;
	
	if (!str)
		return ERR_BAD_ARGS;
	
	ui_page_create_base_ui(page);
	
	#ifndef USE_SDCARD
	kest_button_disable(str->save);
	#endif
	
	kest_active_button_array_create_ui(str->array, page->container);
	
	//create_glide_button_array_ui(str->buttons, page->container);
	
	page->ui_created = 1;
	
	KEST_PRINTF("create_sequence_view_ui done\n");
	return NO_ERROR;
}

int refresh_sequence_view(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	
	
	return NO_ERROR;
}

int free_sequence_view_ui(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	
	
	return NO_ERROR;
}

int sequence_view_free_all(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	
	
	return NO_ERROR;
	
}

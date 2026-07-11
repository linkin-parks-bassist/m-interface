#include "kest_int.h"

static const char *FNAME = "kest_sequence_list.c";

#define PRINTLINES_ALLOWED 0

kest_menu_item *create_sequence_listing_menu_item(char *text, kest_sequence *sequence, kest_ui_page *parent)
{
	kest_menu_item *item = kest_alloc(sizeof(kest_menu_item));
	
	if (!item || !sequence)
		return NULL;
	
	init_menu_item(item);
	
	item->type = MENU_ITEM_SEQUENCE_LISTING;
	if (text)
		item->text = kest_strndup(text, MENU_ITEM_TEXT_MAX_LEN);
	else if (sequence->name)
		item->text = kest_strndup(sequence->name, MENU_ITEM_TEXT_MAX_LEN);
	else
		item->text = "Sequence";
	
	item->linked_page_indirect = &sequence->view_page;
	item->data = sequence;
	
	if (sequence)
		kest_sequence_add_menu_listing(sequence, item);
	
	item->parent = parent;
	
	return item;
}

int sequence_listing_menu_item_refresh_active(kest_menu_item *item)
{
	KEST_PRINTF("sequence_listing_menu_item_refresh_active\n");
	if (!item)
		return ERR_NULL_PTR;
	
	if (!item->extra)
		return NO_ERROR;
		
	if (!item->extra[1])
		return NO_ERROR;
	
	if (item->data && ((kest_sequence*)item->data)->active)
	{
		KEST_PRINTF("sequence is active. going about it\n");
		
		if (item->extra && item->extra[0] && item->extra[1])
		{
			lv_label_set_text(item->extra[1], LV_SYMBOL_PLAY);
			lv_obj_clear_flag(item->extra[0], LV_OBJ_FLAG_HIDDEN);
			lv_obj_clear_flag(item->extra[0], LV_OBJ_FLAG_CLICKABLE);
		}
	}
	else
	{
		KEST_PRINTF("sequence is not active. hiding play\n");
		if (item->extra && item->extra[0] && item->extra[1])
		{
			lv_label_set_text(item->extra[1], LV_SYMBOL_TRASH);
			lv_obj_add_flag(item->extra[0], LV_OBJ_FLAG_HIDDEN);
			lv_obj_add_flag(item->extra[0], LV_OBJ_FLAG_CLICKABLE);
		}
	}
	
	KEST_PRINTF("sequence_listing_menu_item_refresh_active done\n");
	return NO_ERROR;
}

int sequence_listing_menu_item_change_name(kest_menu_item *item, char *name)
{
	if (!item)
		return ERR_NULL_PTR;
	
	if (!item->extra)
		return NO_ERROR;
	
	item->text = kest_strndup(name, 32);
	
	lv_label_set_text(item->label, item->text);
	
	return NO_ERROR;
}

int init_sequence_list(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	//kest_printf("init_sequence_list\n");
	init_menu_page(page);
	
	page->configure = configure_sequence_list;
	
	//kest_printf("init_sequence_list done\n");
	return NO_ERROR;
}

void sequence_list_add_cb(lv_event_t *e)
{
	kest_ui_page *page = lv_event_get_user_data(e);
	
	static int any_new_sequences = 0;
	static int next_sequence_n;
	
	if (!page)
		return;
	
	kest_menu_page_str *str = page->data_struct;
	
	if (!str)
		return;
	
	kest_sequence *new_sequence = kest_context_add_sequence_rp(&global_cxt);
	
	if (!new_sequence)
	{
		KEST_PRINTF("ERROR: Couldn't create new sequence\n");
		return;
	}
	
	if (!any_new_sequences)
	{
		next_sequence_n = kest_cxt_get_sequence_count(&global_cxt);
		any_new_sequences = 1;
	}
	else
	{
		next_sequence_n++;
	}
	
	kest_sequence_set_default_name(new_sequence, next_sequence_n);
	
	create_sequence_view_for(new_sequence);
	
	if (!new_sequence->view_page)
	{
		KEST_PRINTF_FORCE("Failed to create sequence view page for sequence\n");
		return;
	}
	
	kest_sequence_view_str *view_str = (kest_sequence_view_str*)new_sequence->view_page->data_struct;
	
	kest_menu_item *new_listing = create_sequence_listing_menu_item(new_sequence->name, new_sequence, page);
	
	if (!new_listing)
	{
		KEST_PRINTF("Failed to create sequence listing menu item\n");
		return;
	}
	
	new_sequence->view_page->parent = page;
	if (view_str) view_str->menu_item = new_listing;
	//enter_ui_page_forwards(new_sequence->view_page);
	
	menu_page_add_item(str, new_listing);
	create_menu_item_ui(new_listing, page->container);
	sequence_listing_menu_item_refresh_active(new_listing);
	
	kest_queue_sequence_save(new_sequence);
}

int configure_sequence_list(kest_ui_page *page, void *data)
{
	KEST_PRINTF("Configure sequence list\n");
	if (!page)
		return ERR_NULL_PTR;
	
	if (page->configured)
		return NO_ERROR;
	
	page->parent = (kest_ui_page*)data;
	
	kest_menu_page_str *str = page->data_struct;
	
	if (!str)
		return ERR_BAD_ARGS;
	
	str->name = "Sequences";
	
	page->panel = new_panel();
	page->panel->text = str->name;
	page->container_type = CONTAINER_TYPE_STD_BTN_LIST;
	
	ui_page_add_back_button(page);
	
	ui_page_add_bottom_button(page, LV_SYMBOL_PLUS, sequence_list_add_cb);
	
	sequence_ll *current = global_cxt.sequences;
	KEST_PRINTF("current = global_cxt.sequences = %p\n", current);
	kest_menu_item_pll *nl = NULL;
	kest_menu_item *item = NULL;
	
	int i = 0;
	while (current)
	{
		KEST_PRINTF("current = %p, current->data = %p\n",
			current, current->data);
		if (current->data)
		{
			KEST_PRINTF("Add list item for sequence %d, %p = %s\n", i, current->data, current->data->name);
			KEST_PRINTF("Sequence view page pointer: %p, dbl ptr: %p\n", current->data->view_page, &current->data->view_page);
			item = create_sequence_listing_menu_item(current->data->name, current->data, page);
			menu_page_add_item(str, item);
			
			if (current->data->view_page && current->data->view_page->data_struct)
				((kest_sequence_view_str*)current->data->view_page->data_struct)->menu_item = item;
		}
		
		current = current->next;
		i++;
	}
	
	page->configured = 1;
	
	return NO_ERROR;
}

int free_sequence_list_ui(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	
	
	return NO_ERROR;
}

void sequence_listing_delete_button_cb(lv_event_t *e)
{
	kest_menu_item *item = lv_event_get_user_data(e);
	
	if (!item)
		return;
	
	kest_sequence *sequence = (kest_sequence*)item->data;
	
	if (!sequence)
		return;
	
	if (!sequence->active)
	{
		cxt_remove_sequence(&global_cxt, sequence);
		
		menu_page_remove_item(item->parent, item);
	}
}

void disappear_sequence_listing_delete_button(lv_timer_t *timer)
{
	kest_menu_item *item = lv_timer_get_user_data(timer);
	
	lv_obj_add_flag(item->extra[0], LV_OBJ_FLAG_HIDDEN);
	
	item->timer = NULL;
}

void menu_item_sequence_listing_released_cb(lv_event_t *e)
{
	KEST_PRINTF("menu_item_sequence_listing_released_cb\n");
	kest_menu_item *item = (kest_menu_item*)lv_event_get_user_data(e);
	
	if (!item)
		return;
	
	int ret_val;
	kest_sequence *sequence = item->data;
	
	if (!sequence)
	{
		KEST_PRINTF("Error: sequence listing has no associated sequence!\n");
	}
	
	if (!item->long_pressed)
	{
		if (!sequence->view_page)
		{
			ret_val = create_sequence_view_for(sequence);
			if (ret_val != NO_ERROR)
			{
				KEST_PRINTF("Error creating sequence view for sequence: %s\n", kest_error_code_to_string(ret_val));
				return;
			}
		}
		
		sequence->view_page->parent = item->parent;
		enter_ui_page_forwards(sequence->view_page);
	}
	else
	{
		if (sequence && !sequence->active)
		{
			item->timer = lv_timer_create(disappear_sequence_listing_delete_button, STANDARD_DEL_BTN_REMAIN_MS, item);
			lv_timer_set_repeat_count(item->timer, 1);
		}
	}
	
	item->long_pressed = 0;
}

void menu_item_sequence_listing_long_pressed_cb(lv_event_t *e)
{
	kest_menu_item *item = (kest_menu_item*)lv_event_get_user_data(e);
	
	if (!item)
		return;
	
	item->long_pressed = 1;
	
	kest_sequence *sequence = item->data;
	
	if (sequence && !sequence->active)
	{
		if (item->extra && item->extra[0])
			lv_obj_clear_flag(item->extra[0], LV_OBJ_FLAG_HIDDEN);
	}
	else
	{
		if (item->extra && item->extra[0])
			lv_obj_clear_flag(item->extra[0], LV_OBJ_FLAG_HIDDEN);
	}
}

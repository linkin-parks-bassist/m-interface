#include "kest_int.h"

#define PRINTLINES_ALLOWED 0

static const char *FNAME = "kest_menu.c";

IMPLEMENT_LINKED_PTR_LIST(kest_menu_item);

kest_parameter_widget *input_gain_widget = NULL;
kest_parameter_widget *output_gain_widget = NULL;

int init_menu_item(kest_menu_item *item)
{
	if (!item)
		return ERR_NULL_PTR;
	
	memset(item, 0, sizeof(kest_menu_item));
	
	return NO_ERROR;
}

void menu_page_link_clicked_cb(lv_event_t *e)
{
	kest_menu_item *item = (kest_menu_item*)lv_event_get_user_data(e);
	
	if (!item)
		return;
	
	if (!item->linked_page && !item->linked_page_indirect)
		return;
	
	if (item->linked_page_indirect)
		enter_ui_page_indirect_forwards(item->linked_page_indirect);
	else if (item->linked_page)
		enter_ui_page_forwards(item->linked_page);
}

int configure_menu_item(kest_menu_item *item)
{
	if (!item)
		return ERR_NULL_PTR;
	
	switch (item->type)
	{			
		case MENU_ITEM_PAGE_LINK:
			if (item->linked_page && !item->linked_page->configured)
				configure_ui_page(item->linked_page, item->lp_configure_arg);
			break;
	
		case MENU_ITEM_PRESET_LISTING:
		case MENU_ITEM_SEQUENCE_LISTING:
		case MENU_ITEM_PAGE_LINK_INDIRECT:
			if (item->linked_page_indirect && *item->linked_page_indirect && !(*item->linked_page_indirect)->configured)
				configure_ui_page(*item->linked_page_indirect, item->lp_configure_arg);
			break;
		
		case MENU_ITEM_CALLBACK_BUTTON:
			if (item->linked_page_indirect && *item->linked_page_indirect && !(*item->linked_page_indirect)->configured)
				configure_ui_page(*item->linked_page_indirect, item->lp_configure_arg);
			break;
		
		case MENU_ITEM_PARAMETER_WIDGET:
		case MENU_ITEM_PAD:
			break;
			
		case MENU_ITEM_DANGER_BUTTON:
			break;
		
		default:
			return ERR_BAD_ARGS;
	}
	
	return NO_ERROR;
}


int delete_menu_item_ui(kest_menu_item *item)
{
	if (!item)
		return ERR_NULL_PTR;
	
	if (item->timer)
	{
		lv_timer_del(item->timer);
	}
	
	lv_anim_del(item, NULL);
	
	lv_obj_del_async(item->obj);
	
	return NO_ERROR;
}

int free_menu_item(kest_menu_item *item)
{
	//if (item->text)
	//	kest_free(item->text);
	if (item->desc)
		kest_free(item->desc);
	
	kest_free(item);
	
	return NO_ERROR;
}

int refresh_menu_item(kest_menu_item *item)
{
	if (!item)
		return ERR_NULL_PTR;
	
	switch (item->type)
	{
		case MENU_ITEM_PRESET_LISTING:
			//preset_listing_menu_item_refresh_active(item);
			break;
		
		default:
			break;
	}
	
	return NO_ERROR;
}

void parameter_widget_change_cb_settings_wrapper(lv_event_t *e)
{
	KEST_PRINTF("parameter_widget_change_cb_settings_wrapper\n");
	kest_menu_item *item = lv_event_get_user_data(e);
	
	if (!item)
		return;
	
	#ifdef KEST_USE_FREERTOS
	xSemaphoreTake(state_mutex, portMAX_DELAY);
	parameter_widget_change_cb_inner(item->data);
	xSemaphoreGive(state_mutex);
	#endif
}

void danger_button_value_changed_cb(lv_event_t *e)
{
	kest_menu_item *item = lv_event_get_user_data(e);
	
	if (!item)
		return;
	
	if (!item->extra)
		return;
	
	if (!item->extra[0])
		return;
	
	/*
	const char *button_text = lv_msgbox_get_active_btn_text(item->extra[0]);
	
	if (strncmp(DANGER_BUTTON_CONFIRM_TEXT, button_text, strlen(DANGER_BUTTON_CONFIRM_TEXT) + 1) == 0)
	{
		if (item->action_cb)
			item->action_cb(item->cb_arg);
	}
	
	lv_msgbox_close(item->extra[0]);*/
}

void danger_button_activate_popup_cb(lv_event_t *e)
{
	kest_menu_item *item = lv_event_get_user_data(e);
	
	if (!item)
		return;
	
	if (!item->parent)
		return;
	/*
	if (!item->extra)
	{
		item->extra = kest_alloc(sizeof(lv_obj_t*));
		if (!item->extra)
			return;
	}
	
	static const char *btns[] = {DANGER_BUTTON_CONFIRM_TEXT, DANGER_BUTTON_CANCEL_TEXT, NULL};
	
	item->extra[0] = lv_msgbox_create(item->parent->screen, item->text, "Are you sure?", btns, 1);
	lv_obj_add_event_cb(item->extra[0], danger_button_value_changed_cb, LV_EVENT_VALUE_CHANGED, item);
	lv_obj_center(item->extra[0]);*/
}

kest_menu_item *create_preset_listing_menu_item(char *text, kest_preset *preset, kest_ui_page *parent)
{
	kest_menu_item *item = kest_alloc(sizeof(kest_menu_item));
	
	if (!item || !preset)
		return NULL;
	
	init_menu_item(item);
	
	item->type = MENU_ITEM_PRESET_LISTING;
	if (text)
		item->text = kest_strndup(text, MENU_ITEM_TEXT_MAX_LEN);
	else
		item->text = "Preset";
	
	item->linked_page_indirect = &preset->view_page;
	item->data = preset;
	
	item->parent = parent;
	
	return item;
}

int preset_listing_menu_item_refresh_active(kest_menu_item *item)
{
	KEST_PRINTF("preset_listing_menu_item_refresh_active\n");
	if (!item)
		return ERR_NULL_PTR;
	
	if (!item->extra)
		return NO_ERROR;
		
	if (!item->extra[1])
		return NO_ERROR;
	
	if (item->data && ((kest_preset*)item->data)->active)
	{
		KEST_PRINTF("preset is active. going about it\n");
		lv_label_set_text(item->extra[1], LV_SYMBOL_PLAY);
		lv_obj_clear_flag(item->extra[0], LV_OBJ_FLAG_HIDDEN);
		lv_obj_clear_flag(item->extra[0], LV_OBJ_FLAG_CLICKABLE);
	}
	else
	{
		KEST_PRINTF("preset is not active. hiding play\n");
		lv_label_set_text(item->extra[1], LV_SYMBOL_TRASH);
		lv_obj_add_flag(item->extra[0], LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(item->extra[0], LV_OBJ_FLAG_CLICKABLE);
	}
	
	KEST_PRINTF("preset_listing_menu_item_refresh_active done\n");
	return NO_ERROR;
}

int preset_listing_menu_item_change_name(kest_menu_item *item, char *name)
{
	if (!item)
		return ERR_NULL_PTR;
	
	if (!item->extra)
		return NO_ERROR;
	
	item->text = strndup(name, 32);
	
	lv_label_set_text(item->label, item->text);
	
	return NO_ERROR;
}

void preset_listing_delete_button_cb(lv_event_t *e)
{
	kest_menu_item *item = lv_event_get_user_data(e);
	
	if (!item)
		return;
	
	kest_preset *preset = (kest_preset*)item->data;
	
	if (!preset)
		return;
	
	if (!preset->active)
	{
		cxt_remove_preset(&global_cxt, preset);
		
		menu_page_remove_item(item->parent, item);
	}
}

void disappear_preset_listing_delete_button(lv_timer_t *timer)
{
	kest_menu_item *item = lv_timer_get_user_data(timer);
	
	lv_obj_add_flag(item->extra[0], LV_OBJ_FLAG_HIDDEN);
	
	item->timer = NULL;
}

void menu_item_preset_listing_released_cb(lv_event_t *e)
{
	kest_menu_item *item = (kest_menu_item*)lv_event_get_user_data(e);
	
	if (!item)
		return;
	
	kest_preset *preset = item->data;
	
	if (!item->long_pressed)
	{
		if (preset && preset->view_page)
		{
			preset->view_page->parent = &global_cxt.pages.main_menu;
			
			enter_ui_page_forwards(preset->view_page);
		}
	}
	else
	{
		if (preset && !preset->active)
		{
			item->timer = lv_timer_create(disappear_preset_listing_delete_button, STANDARD_DEL_BTN_REMAIN_MS, item);
			lv_timer_set_repeat_count(item->timer, 1);
		}
	}
	
	item->long_pressed = 0;
}

void menu_item_preset_listing_long_pressed_cb(lv_event_t *e)
{
	kest_menu_item *item = (kest_menu_item*)lv_event_get_user_data(e);
	
	if (!item)
		return;
	
	item->long_pressed = 1;
	
	kest_preset *preset = item->data;
	
	if (preset && !preset->active)
	{
		lv_obj_clear_flag(item->extra[0], LV_OBJ_FLAG_HIDDEN);
	}
	else
	{
		lv_obj_clear_flag(item->extra[0], LV_OBJ_FLAG_HIDDEN);
	}
}

int create_menu_item_ui(kest_menu_item *item, lv_obj_t *parent)
{
	if (!item)
		return ERR_NULL_PTR;
	
	switch (item->type)
	{
		case MENU_ITEM_PAGE_LINK:
		case MENU_ITEM_PAGE_LINK_INDIRECT:
			create_standard_button_click(&item->obj, &item->label, parent, item->text, menu_page_link_clicked_cb, item);
			break;
		
		case MENU_ITEM_CALLBACK_BUTTON:
			create_standard_button_click(&item->obj, &item->label, parent, item->text, item->click_cb, item->cb_arg);
			break;
		
		case MENU_ITEM_DANGER_BUTTON:
			create_standard_button_click(&item->obj, &item->label, parent, item->text, danger_button_activate_popup_cb, item);
			break;
		
		case MENU_ITEM_SEQUENCE_LISTING:
			create_standard_button_long_press_release(&item->obj, &item->label, parent, item->text,
				menu_item_sequence_listing_long_pressed_cb, item,
				menu_item_sequence_listing_released_cb, item);
			
			item->extra = kest_alloc(sizeof(lv_obj_t*) * 2);
			
			if (!item->extra)
				return ERR_ALLOC_FAIL;
				
			item->extra[0] = lv_btn_create(item->obj);
			
			lv_obj_align(item->extra[0], LV_ALIGN_RIGHT_MID, 10, 0);
			lv_obj_set_size(item->extra[0], 0.75 * STANDARD_BUTTON_HEIGHT, 0.75 * STANDARD_BUTTON_HEIGHT);
			lv_obj_add_event_cb(item->extra[0], sequence_listing_delete_button_cb, LV_EVENT_CLICKED, item);
			lv_obj_add_flag(item->extra[0], LV_OBJ_FLAG_HIDDEN);
			
			item->extra[1] = lv_label_create(item->extra[0]);
			lv_label_set_text(item->extra[1], LV_SYMBOL_TRASH);
			
			lv_obj_center(item->extra[1]);
			break;
		
		case MENU_ITEM_PRESET_LISTING:
			create_standard_button_long_press_release(&item->obj, &item->label, parent, item->text,
				menu_item_preset_listing_long_pressed_cb, item,
				menu_item_preset_listing_released_cb, item);
			
			item->extra = kest_alloc(sizeof(lv_obj_t*) * 2);
			
			if (!item->extra)
				return ERR_ALLOC_FAIL;
				
			item->extra[0] = lv_btn_create(item->obj);
			
			lv_obj_align(item->extra[0], LV_ALIGN_RIGHT_MID, 10, 0);
			lv_obj_set_size(item->extra[0], 0.75 * STANDARD_BUTTON_HEIGHT, 0.75 * STANDARD_BUTTON_HEIGHT);
			lv_obj_add_event_cb(item->extra[0], preset_listing_delete_button_cb, LV_EVENT_CLICKED, item);
			lv_obj_add_flag(item->extra[0], LV_OBJ_FLAG_HIDDEN);
			
			item->extra[1] = lv_label_create(item->extra[0]);
			lv_label_set_text(item->extra[1], LV_SYMBOL_TRASH);
			
			lv_obj_center(item->extra[1]);
			break;
		
		case MENU_ITEM_PARAMETER_WIDGET:
			parameter_widget_create_ui_ncbsf(item->data, parent, 1.0f);
			lv_obj_add_event_cb(((kest_parameter_widget*)item->data)->obj, parameter_widget_change_cb_settings_wrapper, LV_EVENT_VALUE_CHANGED, item);
			break;
		
		case MENU_ITEM_PAD:
			item->obj = lv_obj_create(parent);
			lv_obj_remove_style_all(item->obj);
			lv_obj_set_size(item->obj, 1, (int)item->data);
		
		default:
			return ERR_BAD_ARGS;
	}
	
	return NO_ERROR;
}

kest_menu_item *create_pad_menu_item(int pad_height)
{
	kest_menu_item *item = kest_alloc(sizeof(kest_menu_item));
	
	if (!item)
		return NULL;
	
	init_menu_item(item);
	
	item->type = MENU_ITEM_PAD;
	item->data = (void*)pad_height;
	
	return item;
}

kest_menu_item *create_page_link_menu_item(char *text, kest_ui_page *linked_page, kest_ui_page *parent)
{
	kest_menu_item *item = kest_alloc(sizeof(kest_menu_item));
	
	if (!item)
		return NULL;
	
	KEST_PRINTF("create_page_link_menu_item. parent = %p\n", parent);
	
	init_menu_item(item);
	
	item->type = MENU_ITEM_PAGE_LINK;
	item->text = kest_strndup(text, MENU_ITEM_TEXT_MAX_LEN);
	
	item->linked_page = linked_page;
	item->parent = parent;
	
	return item;
}

kest_menu_item *create_page_linindirect_k_menu_item(char *text, kest_ui_page **linked_page, kest_ui_page *parent)
{
	kest_menu_item *item = kest_alloc(sizeof(kest_menu_item));
	
	if (!item)
		return NULL;
	
	init_menu_item(item);
	
	item->type = MENU_ITEM_PAGE_LINK_INDIRECT;
	item->text = kest_strndup(text, MENU_ITEM_TEXT_MAX_LEN);
	item->linked_page_indirect = linked_page;
	item->parent = parent;
	
	return item;
}

kest_menu_item *create_parameter_widget_menu_item(kest_parameter *param, kest_ui_page *parent)
{
	if (!param)
		return NULL;
	
	kest_menu_item *item = kest_alloc(sizeof(kest_menu_item));
	
	if (!item)
		return NULL;
	
	init_menu_item(item);
		
	item->type = MENU_ITEM_PARAMETER_WIDGET;
	
	kest_parameter_widget *pw = kest_alloc(sizeof(kest_parameter_widget));
	
	nullify_parameter_widget(pw);
	
	if (!pw)
	{
		free_menu_item(item);
		return NULL;
	}
		
	item->data = pw;
	
	configure_parameter_widget(item->data, param, NULL, parent);
	
	return item;
}

kest_menu_item *create_danger_button_menu_item(void (*action_cb)(void* arg), void *arg, const char *text, kest_ui_page *parent)
{
	if (!action_cb)
		return NULL;
	
	kest_menu_item *item = kest_alloc(sizeof(kest_menu_item));
	
	if (!item)
		return NULL;
	
	init_menu_item(item);
		
	item->type = MENU_ITEM_DANGER_BUTTON;
	
	item->text = kest_strndup(text, 128);
	item->action_cb = action_cb;
	item->cb_arg = arg;
	item->parent = parent;
	
	return item;
}

int init_menu_page_str(kest_menu_page_str *str)
{
	if (!str)
		return ERR_NULL_PTR;
	
	str->type = 0;
	
	str->items 	   = NULL;
	str->next_page = NULL;
	str->data 	   = NULL;
	
	return NO_ERROR;
}

int init_menu_page(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	init_ui_page(page);
	
	kest_menu_page_str *str = kest_alloc(sizeof(kest_menu_page_str));
	
	page->data_struct = str;
	
	if (!str)
		return ERR_ALLOC_FAIL;
	
	page->create_ui	 = create_menu_page_ui;
	page->configure  = configure_menu_page;
	page->free_ui	 = free_menu_page_ui;
	page->enter_page = enter_menu_page;
	
	return init_menu_page_str(str);
}

int configure_menu_page(kest_ui_page *page, void *data)
{
	KEST_PRINTF("configure_menu_page\n");
	if (!page)
		return ERR_NULL_PTR;
	
	if (page->configured)
		return NO_ERROR;
	
	int ret_val = NO_ERROR;
	kest_menu_page_str *str = (kest_menu_page_str*)page->data_struct;
	
	if (!str)
		return ERR_BAD_ARGS;
	
	page->panel = new_panel();
	page->panel->text = str->name;
	page->container_type = CONTAINER_TYPE_STD_BTN_LIST;
	
	ui_page_add_back_button(page);
	
	page->parent = data;
	kest_menu_item_pll *current_item = str->items;
	
	while (current_item)
	{
		configure_menu_item(current_item->data);
		current_item = current_item->next;
	}
	
	if (str->next_page)
		ret_val = configure_menu_page(str->next_page, page);
	
	page->configured = (ret_val == NO_ERROR);
	
	KEST_PRINTF("configure_menu_page done\n");
	return ret_val;
}

int create_menu_page_ui(kest_ui_page *page)
{
	KEST_PRINTF("create_menu_page_ui\n");
	if (!page)
		return ERR_NULL_PTR;
	
	if (page->ui_created)
	{
		return NO_ERROR;
	}
	
	ui_page_create_base_ui(page);
	
	kest_menu_page_str *str = (kest_menu_page_str*)page->data_struct;
	
	if (!str)
	{
		return ERR_BAD_ARGS;
	}
	
	kest_menu_item_pll *current = str->items;
	
	int i = 0;
	while (current)
	{
		KEST_PRINTF("Create menu item %d ui\n", i);
		create_menu_item_ui(current->data, page->container);
		current = current->next;
		i++;
	}
	
	if (str->next_page)
		create_menu_page_ui(str->next_page);
	
	page->ui_created = 1;
	
	KEST_PRINTF("create_menu_page_ui done\n");
	return NO_ERROR;
}


int enter_menu_page(kest_ui_page *page)
{
	KEST_PRINTF("enter_menu_page\n");
	
	if (!page)
		return ERR_NULL_PTR;
	
	refresh_menu_page(page);
	
	kest_menu_page_str *str = (kest_menu_page_str*)page->data_struct;
	
	if (str)
	{
		kest_menu_item_pll *current = str->items;
		
		KEST_PRINTF("Menu page menu items:\n");
		
		while (current)
		{
			if (current->data)
			{
				KEST_PRINTF("Type %d\n", current->data->type);
				if (current->data->type == MENU_ITEM_PARAMETER_WIDGET)
				{
					KEST_PRINTF("Requesting value for menu page parameter widget...\n");
					kest_parameter_widget_refresh((kest_parameter_widget*)current->data->data);
				}
			}
			
			current = current->next;
		}
	}
	
	return NO_ERROR;
}

int refresh_menu_page(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	kest_menu_page_str *str = (kest_menu_page_str*)page->data_struct;
	
	if (!str)
		return ERR_BAD_ARGS;
	
	kest_menu_item_pll *current = str->items;
	
	while (current)
	{
		refresh_menu_item(current->data);
		current = current->next;
	}
	
	return NO_ERROR;
}

int free_menu_page_ui(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	
	
	return ERR_UNIMPLEMENTED;
}

int menu_page_add_item(kest_menu_page_str *str, kest_menu_item *item)
{
	if (!str || !item)
		return ERR_NULL_PTR;
	
	kest_menu_item_pll *nl = kest_menu_item_pll_append(str->items, item);
	
	if (!nl)
		return ERR_ALLOC_FAIL;
	
	str->items = nl;
	
	return NO_ERROR;
}

void enter_main_menu_cb(lv_event_t *e)
{
	enter_ui_page(&global_cxt.pages.main_menu);
}

int init_main_menu(kest_ui_page *page)
{
	init_ui_page(page);
	
	page->create_ui = create_main_menu_ui;
	page->configure = configure_main_menu;
	page->enter_page = enter_main_menu;
	
	page->data_struct = NULL;
	
	page->type = KEST_UI_PAGE_MAIN_MENU;
	
	return NO_ERROR;
}

void kest_msc_button_cb(lv_event_t *e)
{
	int ret_val = kest_sd_toggle_msc();
	
	kest_button *button = lv_event_get_user_data(e);//(kest_button*)arg;
	
	if (sd_msc_mode)
	{
		//kest_button_set_label(button, "Mount SD card");
	}
	else
	{
		//kest_button_set_label(button, "Unmount SD card");
	}
}

int configure_main_menu(kest_ui_page *page, void *data)
{
	if (!page)
		return ERR_NULL_PTR;
	
	page->parent = data;
	
	kest_main_menu_str *str = kest_alloc(sizeof(kest_main_menu_str));
	
	if (!str)
		return ERR_ALLOC_FAIL;
	
	page->data_struct = str;
	
	str->gains_container = NULL;
	
	page->panel = new_panel();
	page->panel->text = "Main Menu";
	page->container_type = CONTAINER_TYPE_STD_BTN_LIST;
	
	input_gain_widget  = &str->input_gain;
	output_gain_widget = &str->output_gain;
	nullify_parameter_widget(&str->input_gain);
	nullify_parameter_widget(&str->output_gain);
	
	configure_parameter_widget( &str->input_gain,  &global_cxt.input_gain, NULL, page);
	configure_parameter_widget(&str->output_gain, &global_cxt.output_gain, NULL, page);
	
	init_button(&str->presets_button);
	init_button(&str->sequences_button);
	init_button(&str->msc_button);
	init_danger_button(&str->erase_sd_card_button, erase_sd_card_void_cb, NULL, page);
	
	kest_button_set_label(&str->presets_button, "Presets");
	kest_button_disable_alignment(&str->presets_button);
	kest_button_set_label(&str->sequences_button, "Sequences");
	kest_button_disable_alignment(&str->sequences_button);
	kest_button_set_label(&str->msc_button, "Unmount SD card");
	kest_button_disable_alignment(&str->msc_button);
	kest_button_set_label(&str->erase_sd_card_button.button, "Erase SD card");
	kest_button_disable_alignment(&str->erase_sd_card_button.button);
	
	button_set_clicked_cb( &str->presets_button, enter_ui_page_forwards_cb, &global_cxt.pages.main_sequence_view);
	button_set_clicked_cb(&str->sequences_button, enter_ui_page_forwards_cb, &global_cxt.pages.sequence_list);
	button_set_clicked_cb(&str->msc_button, kest_msc_button_cb, &str->msc_button);
	
	page->configured = 1;
	
	return NO_ERROR;
}

int create_main_menu_ui(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (page->ui_created)
	{
		return NO_ERROR;
	}
	
	kest_main_menu_str *str = (kest_main_menu_str*)page->data_struct;
	
	if (!str)
	{
		return ERR_BAD_ARGS;
	}
	
	ui_page_create_base_ui(page);
	
	str->top_pad = lv_obj_create(page->container);
	
	lv_obj_remove_style_all(str->top_pad);
	lv_obj_set_size(str->top_pad, LV_PCT(100), 20);
	
	/*
	lv_obj_set_layout(page->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(page->container, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_flex_align(page->container,
		LV_FLEX_ALIGN_SPACE_EVENLY,
		LV_FLEX_ALIGN_CENTER,
		LV_FLEX_ALIGN_START);
	*/
	str->gains_container = lv_obj_create(page->container);
	
	lv_obj_remove_style_all(str->gains_container);
	lv_obj_set_layout(str->gains_container, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow (str->gains_container, LV_FLEX_FLOW_ROW_WRAP);
	lv_obj_set_flex_align(str->gains_container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY);
	
	parameter_widget_create_ui( &str->input_gain, str->gains_container);
	parameter_widget_create_ui(&str->output_gain, str->gains_container);
	
	lv_obj_set_size(str->gains_container, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	
	str->top_pad = lv_obj_create(page->container);
	
	lv_obj_remove_style_all(str->top_pad);
	lv_obj_set_size(str->top_pad, LV_PCT(100), 20);
	
	create_button_ui(&str->presets_button,   page->container);
	create_button_ui(&str->sequences_button, page->container);
	create_button_ui(&str->msc_button, 		 page->container);
	
	kest_danger_button_create_ui(&str->erase_sd_card_button, page->container);
	
	page->ui_created = 1;
	
	return NO_ERROR;
}

int enter_main_menu(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	enter_menu_page(page);
	
	kest_main_menu_str *str = page->data_struct;
	
	if (!str)
		return ERR_BAD_ARGS;
	
	kest_parameter_widget_align_nominal_value(&str->input_gain);
	kest_parameter_widget_refresh(&str->input_gain);
	kest_parameter_widget_align_nominal_value(&str->output_gain);
	kest_parameter_widget_refresh(&str->output_gain);
	
	return NO_ERROR;
}

int menu_page_remove_item(kest_ui_page *page, kest_menu_item *item)
{
	if (!page)
		return ERR_NULL_PTR;
	
	kest_menu_page_str *str = (kest_menu_page_str*)page->data_struct;

	if (!str)
		return ERR_BAD_ARGS;
	
	kest_menu_item_pll *current_item = str->items;
	kest_menu_item_pll *prev_item = NULL;
	
	while (current_item)
	{
		if (current_item->data == item)
		{
			delete_menu_item_ui(current_item->data);
			free_menu_item(current_item->data);
			
			if (prev_item)
				prev_item->next = current_item->next;
			else
				str->items = current_item->next;
			
			kest_free(current_item);
			return NO_ERROR;
		}
		
		prev_item = current_item;
		current_item = current_item->next;
	}
	
	return ERR_BAD_ARGS;
}

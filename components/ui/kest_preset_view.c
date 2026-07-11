#include "kest_int.h"

#define PRINTLINES_ALLOWED 0

static const char *FNAME = "kest_preset_view.c";

kest_ui_page *create_preset_view_for(kest_preset *preset)
{
	KEST_PRINTF("create_preset_view_for(preset = %p)\n", preset);
	if (!preset)
		return NULL;
	
	kest_ui_page *page = kest_alloc(sizeof(kest_ui_page));
	
	if (!page)
		return NULL;
	
	KEST_PRINTF("Calling init_ui_page...\n");
	init_ui_page(page);
	
	KEST_PRINTF("Calling init_preset_view...\n");
	int ret_val = init_preset_view(page);
	
	if (ret_val != NO_ERROR)
	{
		free_preset_view(page);
		return NULL;
	}
	
	KEST_PRINTF("Calling configure_preset_view...\n");
	ret_val = configure_preset_view(page, preset);
	
	if (ret_val != NO_ERROR)
	{
		free_preset_view(page);
		return NULL;
	}
	
	preset->view_page = page;
	
	KEST_PRINTF("create_preset_view_for done\n");
	return page;
}

int preset_view_effect_click_cb(kest_active_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	if (!button->data)
		return ERR_BAD_ARGS;
	
	kest_effect *effect = (kest_effect*)button->data;
	
	enter_ui_page_forwards(((kest_effect*)button->data)->view_page);
	
	return NO_ERROR;
}

int preset_view_effect_moved_cb(kest_active_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	kest_effect *effect = button->data;
	
	if (!effect)
		return ERR_BAD_ARGS;
	
	#ifdef USE_FPGA
	kest_preset *preset = effect->preset;
	
	if (preset)
		kest_preset_move_effect(preset, button->index, button->prev_index);
	#endif
	
	return NO_ERROR;
}

int preset_view_effect_delete_cb(kest_active_button *button)
{
	KEST_PRINTF("preset_view_effect_delete_cb\n");
	if (!button)
		return ERR_NULL_PTR;
	
	kest_effect *effect = button->data;
	
	if (!effect)
		return ERR_BAD_ARGS;
	
	kest_preset *preset = effect->preset;
	
	int ret_val = kest_preset_remove_effect(preset, effect->id);
	
	if (ret_val != NO_ERROR)
		return ret_val;
	
	KEST_PRINTF("preset_view_effect_delete_cb done\n");
	return ret_val;
}

int init_preset_view(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	init_ui_page(page);
	
	page->type = KEST_UI_PAGE_PRESET_VIEW;
	
	kest_preset_view_str *str = kest_alloc(sizeof(kest_preset_view_str));
	
	if (!str)
		return ERR_ALLOC_FAIL;
	
	memset(str, 0, sizeof(kest_preset_view_str));
	
	str->settings_page = malloc(sizeof(kest_ui_page));
	
	if (!str->settings_page)
	{
		kest_free(str);
		return ERR_ALLOC_FAIL;
	}
	
	int ret_val = init_preset_settings_page(str->settings_page);
	
	if (ret_val != NO_ERROR)
	{
		kest_free(str->settings_page);
		kest_free(str);
		return ret_val;
	}
	
	page->data_struct = (void*)str;
	
	str->preset 			= NULL;

	page->configure  			= configure_preset_view;
	page->create_ui  			= create_preset_view_ui;
	page->enter_page 			= enter_preset_view;
	page->free_all				= free_preset_view;
	
	str->save 		= NULL;
	str->plus		= NULL;
	str->play 		= NULL;
	
	str->menu_button 		= NULL;
	str->menu_button_label 	= NULL;
	
	page->panel = new_panel();
	
	str->array = kest_active_button_array_new();
	
	kest_active_button_array_set_length(str->array, 32);
	
	str->array->flags |= KEST_ACTIVE_BUTTON_ARRAY_FLAG_DELETEABLE;
	str->array->flags |= KEST_ACTIVE_BUTTON_ARRAY_FLAG_MOVEABLE;
	
	str->array->clicked_cb    = preset_view_effect_click_cb;
	str->array->moved_cb      = preset_view_effect_moved_cb;
	str->array->del_button_cb = preset_view_effect_delete_cb;
	
	str->rep.representee = NULL;
	str->rep.representer = page;
	str->rep.update = preset_view_rep_update;
	
	return NO_ERROR;
}

static void save_button_cb(lv_event_t *e)
{
	#ifndef USE_SDCARD
	return;
	#endif
	
	kest_ui_page *page = lv_event_get_user_data(e);
	
	if (!page)
		return;
	
	kest_preset_view_str *str = (kest_preset_view_str*)page->data_struct;
	
	if (!str)
		return;
	
	//kest_printf("Saving preset...\n");
	int ret_val;
	
	if ((ret_val = kest_preset_save(str->preset)) == NO_ERROR)
		kest_button_disable(str->save);
}

static void menu_button_cb(lv_event_t *e)
{
	KEST_PRINTF("menu_button_cb\n");
	kest_ui_page *page = lv_event_get_user_data(e);
	
	if (!page)
		return;
	
	kest_preset_view_str *str = (kest_preset_view_str*)page->data_struct;
	
	if (!str)
		return;
	
	kest_preset *preset = str->preset;
	
	if (!preset)
		return;
	
	if (preset->sequence)
	{
		KEST_PRINTF("preset has a sequence; enter sequence view %p\n", preset->sequence->view_page);
		enter_ui_page_backwards(preset->sequence->view_page);
	}
	else
	{
		KEST_PRINTF("preset has no sequence. enter ... whatevery. %p\n", page->parent);
		enter_ui_page_backwards(page->parent);
	}
}

int preset_view_save_name(kest_ui_page *page)
{
	KEST_PRINTF("preset_view_save_name\n");
	if (!page)
		return ERR_NULL_PTR;
	
	kest_preset_view_str *str = (kest_preset_view_str*)page->data_struct;
	
	if (!str)
		return ERR_BAD_ARGS;
	
	if (!page->panel || !str->preset)
		return ERR_BAD_ARGS;
	
	const char *new_name = lv_textarea_get_text(page->panel->title);
	
	if (str->preset->name)
		kest_free(str->preset->name);
	
	str->preset->name = kest_strndup(new_name, PRESET_NAME_MAX_LEN);
	
	kest_event_log(kest_event_preset_name_change(str->preset));
	
	return NO_ERROR;
}


void preset_view_save_name_cb(lv_event_t *e)
{
	kest_ui_page *page = (kest_ui_page*)lv_event_get_user_data(e);
	preset_view_save_name(page);
}

void preset_view_revert_name(lv_event_t *e)
{
	kest_ui_page *page = (kest_ui_page*)lv_event_get_user_data(e);
	kest_preset_view_str *str = (kest_preset_view_str*)page->data_struct;
	
	lv_textarea_set_text(page->panel->title, str->preset->name);
	
	lv_obj_clear_state(page->panel->title, LV_STATE_FOCUSED);
	lv_obj_add_state(page->container, LV_STATE_FOCUSED);
	
	hide_keyboard();
}

void preset_view_enter_settings_page_cb(lv_event_t *e)
{
	kest_ui_page *page = (kest_ui_page*)lv_event_get_user_data(e);
	
	if (!page)
		return;
	
	kest_preset_view_str *str = (kest_preset_view_str*)page->data_struct;
	
	if (!str)
		return;
	
	enter_ui_page_forwards(str->settings_page);
}

void preset_view_enter_main_menu_cb(lv_event_t *e)
{
	enter_ui_page_backwards(&global_cxt.pages.main_menu);
}

void preset_view_play_button_cb(lv_event_t *e)
{
	KEST_PRINTF("preset_view_play_button_cb\n");
	kest_ui_page *page = (kest_ui_page*)lv_event_get_user_data(e);
	
	if (!page)
		return;
	
	kest_preset_view_str *str = (kest_preset_view_str*)page->data_struct;
	
	if (!str)
		return;
	
	if (!str->preset)
	{
		KEST_PRINTF("Preset is NULL\n");
		set_active_preset(NULL);
		return;
	}
	
	if (str->preset->sequence)
	{
		kest_sequence_begin_at(str->preset->sequence, str->preset);
	}
	else
	{
		set_active_preset(str->preset);
	}
}

int configure_preset_view(kest_ui_page *page, void *data)
{
	KEST_PRINTF("configure_preset_view(page = %p, data = %p)\n", page, data);
	if (!page || !data)
		return ERR_NULL_PTR;
	
	if (page->configured)
		return NO_ERROR;
	
	kest_preset *preset = (kest_preset*)data;
	
	kest_preset_view_str *str = page->data_struct;
	
	if (!str)
		return ERR_ALLOC_FAIL;
	
	page->data_struct = str;
	
	str->preset = preset;
	
	preset->view_page = page;
	
	if (preset->sequence)
		page->parent = preset->sequence->view_page;
	
	int ret_val;
	int alloc_fail = 0;
	
	kest_effect *effect;
	
	int i = 0;
	kest_effect_pll *current = preset->pipeline.effects;
	
	KEST_PRINTF("Entering while(current)\n");
	while (current)
	{
		effect = current->data;
		
		kest_active_button_array_append_new(str->array, effect, kest_effect_name(effect));
		if (effect && !effect->view_page)
		{
			ret_val = kest_effect_init_view_page(effect, page);
			if (ret_val != NO_ERROR)
			{
				// kill self
			}
			else
			{
				effect->view_page->create_ui(effect->view_page);
			}
		}
		
		current = current->next;
	}
	
	configure_preset_settings_page(str->settings_page, preset);
	
	str->play = ui_page_add_bottom_button(page, LV_SYMBOL_PLAY, preset_view_play_button_cb);
	str->plus = ui_page_add_bottom_button(page, LV_SYMBOL_PLUS, enter_effect_selector_cb);
	str->save = ui_page_add_bottom_button(page, LV_SYMBOL_SAVE, save_button_cb);
	
	#ifndef USE_SDCARD
	kest_button_disable(str->save);
	#endif
	
	if (!preset->unsaved_changes)
		kest_button_disable(str->save);
	
	ui_page_set_title_rw(page, preset_view_save_name_cb, preset_view_revert_name);
	
	ui_page_add_left_panel_button(page, LV_SYMBOL_LIST, menu_button_cb);
	ui_page_add_right_panel_button(page, LV_SYMBOL_SETTINGS, preset_view_enter_settings_page_cb);
	
	str->rep.representee = preset;
	
	kest_preset_add_representation(preset, &str->rep);
	
	ret_val = NO_ERROR;
	if (alloc_fail)	ret_val = ERR_ALLOC_FAIL;
	else page->configured = 1;
	
	return ret_val;
}

int create_preset_view_ui(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (page->ui_created)
		return NO_ERROR;
	
	page->parent = &global_cxt.pages.main_menu;
	
	kest_preset_view_str *str = (kest_preset_view_str*)page->data_struct;
	
	if (!str)
		return ERR_BAD_ARGS;
	
	kest_preset *preset = str->preset;
	
	if (!preset)
		return ERR_BAD_ARGS;
	
	if (preset->sequence)
		page->parent = preset->view_page;
	
	if (page->panel->text)
	{
		// Prevent a minor memory leak
		// After I figure out ownership/nullification
	}
	
	if (!preset->name)
	{
		KEST_PRINTF("create_preset_view_ui. preset->name = NULL\n");
		kest_preset_set_default_name_from_id(preset);
	}
	
	KEST_PRINTF("create_preset_view_ui. preset->name = %s\n", preset->name);
	
	page->panel->text = preset->name;
	
	ui_page_create_base_ui(page);
	kest_active_button_array_create_ui(str->array, page->container);
	
	KEST_PRINTF("preset->active = %d\n", preset->active);
	
	if (preset->active)
	{
		//kest_button_disable(str->play);
	}
	
	if (!str->preset->unsaved_changes)
	{
		kest_button_disable(str->save);
	}
	
	page->ui_created = 1;
	
	return NO_ERROR;
}

int enter_preset_view(kest_ui_page *page)
{
	KEST_PRINTF("enter_preset_view\n");
	if (!page)
		return ERR_NULL_PTR;
	
	kest_preset_view_str *str = (kest_preset_view_str*)page->data_struct;
	
	if (str)
		global_cxt.working_preset = str->preset;
	KEST_PRINTF("set working preset\n");
	global_cxt.pages.effect_selector.parent = page;
	KEST_PRINTF("enter_preset_view done\n");
	return NO_ERROR;
}

int preset_view_append_effect(kest_ui_page *page, kest_effect *effect)
{
	KEST_PRINTF("preset_view_append_effect\n");
	if (!page)
		return ERR_NULL_PTR;
	
	kest_preset_view_str *str = page->data_struct;
	
	if (!str)
		return ERR_BAD_ARGS;
	
	kest_active_button *button = kest_active_button_array_append_new(str->array, effect, kest_effect_name(effect));
	
	if (page->ui_created)
	{
		kest_active_button_create_ui(button, page->container);
	}
	
	KEST_PRINTF("preset_view_append_effect done; effect->view_page = %p\n", effect->view_page);
	return NO_ERROR;
}

int preset_view_recalculate_indices(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	
	
	return NO_ERROR;
}


int free_preset_view(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	kest_preset_view_str *str = page->data_struct;
	
	return ERR_UNIMPLEMENTED;
	
	return NO_ERROR;
}

int preset_view_refresh_play_button(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	
	
	return NO_ERROR;
}


int preset_view_refresh_save_button(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	return NO_ERROR;
}

int preset_view_set_left_button_mode(kest_ui_page *page, int mode)
{
	if (!page)
		return ERR_NULL_PTR;
	
	kest_preset_view_str *str = (kest_preset_view_str*)page->data_struct;
	
	
	
	return NO_ERROR;
}

void preset_view_rep_update(void *representer, void *representee)
{
	KEST_PRINTF("preset_view_rep_update. representer = %p, representee = %p\n", representer, representee);
	
	kest_ui_page *page = (kest_ui_page*)representer;
	kest_preset *preset = (kest_preset*)representee;
	
	if (!page || !preset)
		return;
	
	ui_page_set_title(page, preset->name);
	
	kest_preset_view_str *str = (kest_preset_view_str*)page->data_struct;
	KEST_PRINTF("preset->active = %d,  str = %p, \n", preset->active, str);
	if (!str)
		return;
	
	if (preset->active)
	{
		//kest_button_disable(str->play);
	}
	else
	{
		kest_button_enable(str->play);
	}
	
	#ifdef USE_SDCARD
	if (preset->unsaved_changes)
	{
		kest_button_enable(str->save);
	}
	else
	{
		kest_button_disable(str->save);
	}
	#endif
	
	KEST_PRINTF("preset_view_rep_update done\n");
}

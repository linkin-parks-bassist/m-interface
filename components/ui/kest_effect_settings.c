#include "kest_int.h"

#ifndef PRINTLINES_ALLOWED
#define PRINTLINES_ALLOWED 0
#endif

static const char *FNAME = "kest_effect_settings.c";

int init_effect_settings_page(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	init_ui_page(page);
	
	page->type = KEST_UI_PAGE_EFFECT_SETTINGS;
	
	page->configure = configure_effect_settings_page;
	page->create_ui = create_effect_settings_page_ui;
	
	page->panel = new_panel();
	
	if (!page->panel)
	{
		return ERR_NULL_PTR;
	}
	
	KEST_PRINTF("Created a panel; %p\n", page->panel);
	
	page->data_struct = kest_alloc(sizeof(effect_settings_page_str));
	
	if (!page->data_struct)
		return ERR_ALLOC_FAIL;
	
	effect_settings_page_str *str = (effect_settings_page_str*)page->data_struct;
	
	str->text = NULL;
	
	nullify_parameter_widget(&str->band_lp_cutoff);
	nullify_parameter_widget(&str->band_hp_cutoff);
	nullify_setting_widget(&str->band_mode);
	
	str->band_control_cont = NULL;
	
	return NO_ERROR;
}

int configure_effect_settings_page(kest_ui_page *page, void *data)
{
	if (!page)
		return ERR_NULL_PTR;
	
	ui_page_add_back_button(page);
	
	kest_effect *effect = (kest_effect*)data;
	
	if (!data)
		return ERR_NULL_PTR;
	
	char title_buf[128];
	
	snprintf(title_buf, 128, "%s Settings", kest_effect_name(effect));
	
	char *title =  kest_strndup(title_buf, 128);
	page->panel->text = title;
	
	effect_settings_page_str *str = (effect_settings_page_str*)page->data_struct;
	
	if (!str)
	{
		return ERR_BAD_ARGS;
	}
	
	str->effect = effect;
	
	configure_parameter_widget(&str->band_lp_cutoff, &effect->band_lp_cutoff, effect->preset, page);
	configure_parameter_widget(&str->band_hp_cutoff, &effect->band_hp_cutoff, effect->preset, page);
	configure_setting_widget(&str->band_mode, &effect->band_mode, effect->preset, page);
	
	page->configured = 1;
	
	return NO_ERROR;
}

void band_control_value_changed_cb(lv_event_t *e)
{
	kest_ui_page *page = lv_event_get_user_data(e);
	
	if (!page)
		return;
	
	effect_settings_page_str *str = (effect_settings_page_str*)page->data_struct;
	
	if (!str)
		return;
	
	if (!str->band_mode.setting || !str->band_mode.setting->options || !str->band_mode.obj)
		return;
	
	kest_setting *setting = str->band_mode.setting;
	
	uint16_t value;
	char dd_value[128];
	
	lv_dropdown_get_selected_str(str->band_mode.obj, dd_value, 128);
	
	KEST_PRINTF("Selected band mode: %s\n", dd_value);
	
	int found = 0;
	for (int i = 0; i < setting->n_options; i++)
	{
		if (strncmp(dd_value, setting->options[i].name, 128) == 0)
		{
			found = 1;
			value = setting->options[i].value;
		}
	}
	
	if (!found)
	{
		// something
		return;
	}
	
	KEST_PRINTF("The associated value is %d\n", value);
	setting->value = value;
	
	refresh_effect_settings_page(page);
	
	#ifdef USE_TEENSY
	kest_message msg = create_m_message(KEST_MESSAGE_SET_SETTING_VALUE, "ssss", setting->id.preset_id, setting->id.effect_id, setting->id.setting_id, value);
	
	queue_msg_to_teensy(msg);
	#endif
}

int create_effect_settings_page_ui(kest_ui_page *page)
{
	KEST_PRINTF("create_effect_settings_page_ui\n");
	if (!page)
		return ERR_NULL_PTR;
	
	ui_page_create_base_ui(page);
	
	lv_obj_set_layout(page->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(page->container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(page->container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START);
	
	effect_settings_page_str *str = (effect_settings_page_str*)page->data_struct;
	
	if (!str)
		return ERR_BAD_ARGS;
	
	KEST_PRINTF("page->container = %p\n", page->container);
	
	//parameter_widget_create_ui(&str->cutoff_freq, page->container);
	
	setting_widget_create_ui_no_callback(&str->band_mode, page->container);
	
	lv_obj_add_event_cb(str->band_mode.obj, band_control_value_changed_cb, LV_EVENT_VALUE_CHANGED, page);
	
	str->band_control_cont = lv_obj_create(page->container);
	lv_obj_remove_style_all(str->band_control_cont);
	
	lv_obj_set_layout(str->band_control_cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(str->band_control_cont, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(str->band_control_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY);
	//lv_obj_set_flex_grow(str->band_control_cont, 1);
	
	parameter_widget_create_ui(&str->band_hp_cutoff, global_cxt.pages.backstage);
	parameter_widget_create_ui(&str->band_lp_cutoff, global_cxt.pages.backstage);
	
	refresh_effect_settings_page(page);
	
	page->ui_created = 1;
	
	KEST_PRINTF("create_effect_settings_page_ui done\n");
	return NO_ERROR;
}


int refresh_effect_settings_page(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	effect_settings_page_str *str = (effect_settings_page_str*)page->data_struct;
	
	if (!str)
		return ERR_NULL_PTR;
	
	if (str->band_mode.setting && str->band_control_cont)
	{
		switch (str->band_mode.setting->value)
		{
			case EFFECT_MODE_FULL_SPECTRUM:
				if (str->band_lp_cutoff.container)
				{
					lv_obj_set_parent(str->band_lp_cutoff.container, global_cxt.pages.backstage);
				}
				
				if (str->band_hp_cutoff.container)
				{
					lv_obj_set_parent(str->band_hp_cutoff.container, global_cxt.pages.backstage);
				}
				
				break;
				
			case EFFECT_MODE_UPPER_SPECTRUM:
				if (str->band_lp_cutoff.container)
				{
					lv_obj_set_parent(str->band_lp_cutoff.container, global_cxt.pages.backstage);
				}
				
				if (str->band_hp_cutoff.container)
				{
					lv_obj_set_parent(str->band_hp_cutoff.container, str->band_control_cont);
				}
				
				break;
			
			case EFFECT_MODE_LOWER_SPECTRUM:
				if (str->band_lp_cutoff.container)
				{
					lv_obj_set_parent(str->band_lp_cutoff.container, str->band_control_cont);
				}
				
				if (str->band_hp_cutoff.container)
				{
					lv_obj_set_parent(str->band_hp_cutoff.container, global_cxt.pages.backstage);
				}
				
				break;
				
			case EFFECT_MODE_BAND:
				if (str->band_hp_cutoff.container)
				{
					lv_obj_set_parent(str->band_hp_cutoff.container, str->band_control_cont);
				}
				
				if (str->band_lp_cutoff.container)
				{
					lv_obj_set_parent(str->band_lp_cutoff.container, str->band_control_cont);
				}
				
				
				break;
			
			default:
				return ERR_BAD_ARGS;

		}
		
		lv_obj_set_size(str->band_control_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
		lv_obj_update_layout(str->band_control_cont);
	}
	
	lv_obj_update_layout(page->container);
	lv_obj_update_layout(page->screen);
	
	return NO_ERROR;
}


int free_effect_settings_page_ui(kest_ui_page *page)
{
	return ERR_UNIMPLEMENTED;
	
	if (!page)
		return ERR_NULL_PTR;
		
	return NO_ERROR;
}


int effect_settings_page_free_all(kest_ui_page *page)
{
	return ERR_UNIMPLEMENTED;
	
	if (!page)
		return ERR_NULL_PTR;
		
	return NO_ERROR;
}

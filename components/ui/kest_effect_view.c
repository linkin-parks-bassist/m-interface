#include "kest_int.h"

//#ifndef PRINTLINES_ALLOWED
#define PRINTLINES_ALLOWED 0
//#endif

static const char *FNAME = "kest_effect_view.c";

kest_ui_page *create_effect_view_for(kest_effect *effect)
{
	KEST_PRINTF("create_effect_view_for(effect = %p)\n", effect);
	
	if (!effect)
		return NULL;
	
	kest_ui_page *page = kest_alloc(sizeof(kest_ui_page));
	
	if (!page)
		return NULL;
	
	init_ui_page(page);
	
	int ret_val = init_effect_view(page);
	
	if (ret_val != NO_ERROR)
	{
		free_effect_view(page);
		return NULL;
	}
	
	ret_val = configure_effect_view(page, effect);
	
	if (ret_val != NO_ERROR)
	{
		free_effect_view(page);
		return NULL;
	}
	
	KEST_PRINTF("create_effect_view_for done\n");
	return page;
}

int init_effect_view(kest_ui_page *page)
{
	KEST_PRINTF("init_effect_view(page = %p)\n", page);
	if (!page)
		return ERR_NULL_PTR;
	
	init_ui_page(page);
	
	page->type = KEST_UI_PAGE_EFFECT_VIEW;
	
	page->panel = new_panel();
	
	if (!page->panel)
		return ERR_ALLOC_FAIL;
	
	kest_effect_view_str *str = kest_alloc(sizeof(kest_effect_view_str));
	
	page->data_struct = str;
	
	if (!str)
		return ERR_ALLOC_FAIL;
	
	str->effect 			   = NULL;
	str->parameter_widgets = NULL;
	str->setting_widgets   = NULL;
	
	str->container 		   = NULL;
	
	page->configure  		 = configure_effect_view;
	page->create_ui  		 = create_effect_view_ui;
	page->enter_page 		 = enter_effect_view;
	
	for (int i = 0; i < EFFECT_VIEW_MAX_GROUPS; i++)
	{
		str->group_containers[i] = NULL;
		str->group_inhabited[i] = 0;
	}
	
	str->settings_page = kest_alloc(sizeof(kest_ui_page));
	
	if (!str->settings_page)
		return ERR_ALLOC_FAIL;
	
	init_effect_settings_page(str->settings_page);
	
	return NO_ERROR;
}

void effect_view_enter_settings_cb(lv_event_t *e)
{
	kest_ui_page *page = lv_event_get_user_data(e);
	
	if (!page)
		return;
	
	kest_effect_view_str *str = (kest_effect_view_str*)page->data_struct;
	
	if (!str)
		return;
	
	enter_ui_page_forwards(str->settings_page);
}

int configure_effect_view(kest_ui_page *page, void *data)
{
	KEST_PRINTF("Conpfigure effect view... page = %p, data = %p\n", page, data);
	if (!page || !data)
	{
		if (page)
			page->data_struct = NULL;
		return ERR_NULL_PTR;
	}
	
	if (page->configured)
		return NO_ERROR;
	
	ui_page_add_parent_button(page);
	ui_page_add_right_panel_button(page, LV_SYMBOL_SETTINGS, effect_view_enter_settings_cb);
	
	kest_effect *effect = (kest_effect*)data;
	
	page->panel->text = kest_effect_name(effect);
	
	kest_effect_view_str *str = page->data_struct;
	
	if (!str)
		return ERR_BAD_ARGS;
	
	str->effect = effect;
	
	kest_parameter_widget *pw;
	kest_setting_widget *sw;
	int ret_val;
	
	kest_parameter_pll *current_param = effect->parameters;
	
	int i = 0;
	int group;
	while (current_param)
	{
		if (current_param->data)
		{
			pw = kest_alloc(sizeof(kest_parameter_widget));
		
			if (!pw)
				return ERR_ALLOC_FAIL;
			
			nullify_parameter_widget(pw);
			ret_val = configure_parameter_widget(pw, current_param->data, effect->preset, page);
			
			str->parameter_widgets = kest_parameter_widget_pll_append(str->parameter_widgets, pw);
			
			group = current_param->data->group;
			
			if (0 <= group && group < EFFECT_VIEW_MAX_GROUPS)
			{
				str->group_inhabited[group]++;
			}
		}
		
		current_param = current_param->next;
	}
	
	kest_setting_pll *current_setting = effect->settings;
	
	i = 0;
	while (current_setting)
	{
		KEST_PRINTF("Considering setting %d, at %p...\n", i + 1, current_setting->data);
		if (current_setting->data && current_setting->data->page == EFFECT_SETTING_PAGE_MAIN)
		{
			sw = kest_alloc(sizeof(kest_setting_widget));
		
			if (!sw)
				return ERR_ALLOC_FAIL;
			
			nullify_setting_widget(sw);
			ret_val = configure_setting_widget(sw, current_setting->data, effect->preset, page);
			
			str->setting_widgets = kest_setting_widget_pll_append(str->setting_widgets, sw);
			
			group = current_setting->data->group;
			
			if (0 <= group && group < EFFECT_VIEW_MAX_GROUPS)
				str->group_inhabited[group] = 1;
		}
		
		current_setting = current_setting->next;
		i++;
	}
	
	configure_effect_settings_page(str->settings_page, effect);
	str->settings_page->parent = page;
	
	page->configured = 1;
	
	#ifdef KEST_ENABLE_REPRESENTATIONS
	effect->page_rep.representer = page;
	#endif
	
	return NO_ERROR;
}

int create_effect_view_ui(kest_ui_page *page)
{
	KEST_PRINTF("create_effect_view_ui\n");
	if (!page)
		return ERR_NULL_PTR;
	
	if (page->ui_created)
		return NO_ERROR;
	
	ui_page_create_base_ui(page);
	
	kest_effect_view_str *str = (kest_effect_view_str*)page->data_struct;
	
	if (!str)
		return ERR_BAD_ARGS;
	
	if (!str->effect)
		return ERR_BAD_ARGS;
	
	page->panel->text = kest_effect_name(str->effect);
    
    lv_obj_set_layout(page->container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(page->container, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(page->container, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY);
	
	for (int i = 0; i < EFFECT_VIEW_MAX_GROUPS; i++)
	{
		KEST_PRINTF("str->group_inhabited[%d] = %d\n", i, str->group_inhabited[i]);
		if (str->group_inhabited[i])
		{
			str->group_containers[i] = lv_obj_create(page->container);
			
			lv_obj_remove_style_all(str->group_containers[i]);
			lv_obj_set_flex_flow (str->group_containers[i], LV_FLEX_FLOW_ROW_WRAP);
			lv_obj_set_flex_align(str->group_containers[i], LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_SPACE_EVENLY);
			lv_obj_set_size(str->group_containers[i], 0, 0);
		}
	}
	KEST_PRINTF("\n");
	
	kest_setting_widget_pll *current_setting = str->setting_widgets;
	
	KEST_PRINTF("\n");
	int group;
	int i = 0;
	
	KEST_PRINTF("\n");
	while (current_setting)
	{
		KEST_PRINTF("\n");
		if (current_setting->data)
		{
			KEST_PRINTF("\n");
			if (current_setting->data->setting)
			{
				KEST_PRINTF("current_setting = %p\n", current_setting);
				KEST_PRINTF("current_setting->data = %p\n", current_setting->data);
				KEST_PRINTF("current_setting->data->setting = %p\n", current_setting->data->setting);
				KEST_PRINTF("current_setting->data->setting->group = %d\n", current_setting->data->setting->group);
				group = current_setting->data->setting->group;
				if (0 <= group && group < EFFECT_VIEW_MAX_GROUPS)
				{
					setting_widget_create_ui(current_setting->data, str->group_containers[group]);
				}
				else
				{
					setting_widget_create_ui(current_setting->data, page->container);
				}
				KEST_PRINTF("\n");
			}
			KEST_PRINTF("\n");
		}
		current_setting = current_setting->next;
		i++;
	}
	
	KEST_PRINTF("\n");
	kest_parameter_widget_pll *current_param = str->parameter_widgets;
	i = 0;
	
	KEST_PRINTF("\n");
	
	while (current_param)
	{
		if (current_param->data)
		{
			if (current_param->data->param)
			{
				group = current_param->data->param->group;
				KEST_PRINTF("Creating parameter widget for parameter \"%s\" (%s). Group = %d\n",
					current_param->data->param->name, current_param->data->param->name_internal, group);
				if (0 <= group && group < EFFECT_VIEW_MAX_GROUPS)
				{
					parameter_widget_create_ui_in_group(current_param->data, str->group_containers[group], str->group_inhabited[group]);
				}
				else
				{
					parameter_widget_create_ui(current_param->data, page->container);
				}
			}
		}
		current_param = current_param->next;
		i++;
	}
	
	KEST_PRINTF("\n");
	for (int i = 0; i < EFFECT_VIEW_MAX_GROUPS; i++)
	{
		if (str->group_containers[i] && str->group_inhabited[i])
		{
			lv_obj_set_width(str->group_containers[i],  LV_SIZE_CONTENT);
			lv_obj_set_height(str->group_containers[i], LV_SIZE_CONTENT);
			
			lv_obj_set_style_max_width(str->group_containers[i], STANDARD_CONTAINER_WIDTH - 20, 0);
		}
	}
	
	KEST_PRINTF("\n");
	create_effect_settings_page_ui(str->settings_page);
	
	page->ui_created = 1;
	
	KEST_PRINTF("\n");
	KEST_PRINTF("create_effect_view_ui done\n");
	return NO_ERROR;
}

int enter_effect_view(kest_ui_page *page)
{
	#ifdef USE_TEENSY
	effect_view_request_parameter_values(page);
	#endif
	return NO_ERROR;
}

int clear_effect_view()
{
	return ERR_UNIMPLEMENTED;
}

int free_effect_view_ui(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	lv_obj_del(page->screen);
	
	return NO_ERROR;
}

int free_effect_view(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	kest_effect_view_str *str = (kest_effect_view_str*)page->data_struct;
	
	if (str)
		kest_parameter_widget_pll_destroy(str->parameter_widgets, free_parameter_widget);
	
	if (page->screen)
		lv_obj_del(page->screen);
	
	kest_free(page);
	
	return NO_ERROR;
}

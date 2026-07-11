#include "kest_int.h"

#define PRINTLINES_ALLOWED 0

#include "kest_param_update.h"

static const char *FNAME = "kest_parameter_widget.c";

IMPLEMENT_LINKED_PTR_LIST(kest_parameter_widget);
IMPLEMENT_LINKED_PTR_LIST(kest_setting_widget);

int parameter_widget_update_value(kest_parameter_widget *pw);

void param_widget_rep_update(void *representer, void *representee)
{
	KEST_PRINTF("param_widget_rep_update\n");
	kest_parameter_widget *pw = representer;
	kest_parameter *param = representee;
	
	if (!pw || !param)
		return;
	
	parameter_widget_update_value(pw);
	parameter_widget_update_value_label(pw);
	
	KEST_PRINTF("param_widget_rep_update done\n");
	return;
}

int nullify_parameter_widget(kest_parameter_widget *pw)
{
	if (!pw)
		return ERR_NULL_PTR;
	
	memset(pw, 0, sizeof(kest_parameter_widget));
	
	pw->param 		= NULL;
	
	pw->obj 		= NULL;
	pw->name_label  = NULL;
	pw->val_label   = NULL;
	pw->container   = NULL;
	pw->parent		= NULL;
	
	pw->val_label_text[0] = 0;
	
	pw->rep.representer = pw;
	pw->rep.representee = NULL;
	pw->rep.update = param_widget_rep_update;
	
	pw->nominal_value = 0.0f;
	pw->pressed = 0;
	pw->driven = 0;
	
	return NO_ERROR;
}

void format_parameter_widget_value_label_v(kest_parameter_widget *pw, float v)
{
	if (!pw || !pw->param)
		return;
	
	int i = format_float(pw->val_label_text, v, PARAM_WIDGET_LABEL_BUFSIZE);
	
	if (pw->param->units && i < PARAM_WIDGET_LABEL_BUFSIZE)
	{
		snprintf(&pw->val_label_text[i], PARAM_WIDGET_LABEL_BUFSIZE - i, "%s", pw->param->units);
	}
}

void format_parameter_widget_value_label(kest_parameter_widget *pw)
{
	if (!pw || !pw->param)
		return;
	
	int i = format_float(pw->val_label_text, pw->nominal_value, PARAM_WIDGET_LABEL_BUFSIZE);
	
	if (pw->param->units && i < PARAM_WIDGET_LABEL_BUFSIZE)
	{
		snprintf(&pw->val_label_text[i], PARAM_WIDGET_LABEL_BUFSIZE - i, "%s", pw->param->units);
	}
}

int parameter_widget_update_value(kest_parameter_widget *pw)
{
	if (!pw)
		return ERR_NULL_PTR;
	
	if (!pw->param || !pw->obj)
		return ERR_BAD_ARGS;
	
	kest_parameter *param = pw->param;
	kest_scope *scope = NULL;
	kest_effect *effect = NULL;
	
	float alpha = pw->driven ? 0.2 : 0.1;
	float val = pw->nominal_value * alpha + kest_parameter_evaluate(param) * (1.0f - alpha);
	pw->nominal_value = val;
	
	KEST_PRINTF("parameter_widget_update_value; parameter %d.%d.%d, \"%s\", value %f, nominal value %f. pressed = %d\n",
		param->id.preset_id, param->id.effect_id, param->id.parameter_id,
		param->name ? param->name : "(NULL)", param->value, val, pw->pressed);
	
	uint32_t ival;
	
	kest_interval range = kest_parameter_get_range(pw->param);
	
	float min = range.a;
	float max = range.b;
	
	if (val > max)
	{
		val = max;
	}
	
	if (val < min)
	{
		val = min;
	}
	
	KEST_PRINTF("min/max for PW: %.03f, %.03f\n", min, max);
	
	if (fabsf(max - min) < 1e-6)
	{
		ival = (int)PARAMETER_WIDGET_RANGE_SIZE/2;
	}
	else
	{
		if (pw->param->scale == PARAMETER_SCALE_LOGARITHMIC)
		{
			
			ival = PARAMETER_WIDGET_RANGE_SIZE * ((logf(val) - logf(min)) /
												 (logf(max) - logf(min)));
		}
		else
		{
			ival = PARAMETER_WIDGET_RANGE_SIZE * ((val - min) /
												 (max - min));
		}
	}
	
	switch (pw->param->widget_type)
	{
		case PARAM_WIDGET_VSLIDER_TALL:
		case PARAM_WIDGET_VSLIDER:
		case PARAM_WIDGET_HSLIDER:
			lv_slider_set_value(pw->obj, ival, LV_ANIM_ON);
			break;
			
		default:
			lv_arc_set_value(pw->obj, ival);
			break;
	}
	
	return NO_ERROR;
}

void parameter_widget_update_value_label_v(kest_parameter_widget *pw, float v)
{
	if (!pw)
		return;
	
	format_parameter_widget_value_label_v(pw, v);
	
	int len = strlen(pw->val_label_text);
	
	if (pw->val_label)
		lv_label_set_text(pw->val_label, pw->val_label_text);
	
	return;
}

void parameter_widget_update_value_label(kest_parameter_widget *pw)
{
	if (!pw)
		return;
	
	format_parameter_widget_value_label(pw);
	
	int len = strlen(pw->val_label_text);
	
	if (pw->val_label)
		lv_label_set_text(pw->val_label, pw->val_label_text);
	
	return;
}

int configure_parameter_widget(kest_parameter_widget *pw, kest_parameter *param, kest_preset *preset, kest_ui_page *parent)
{
	KEST_PRINTF("configure_parameter_widget(pw = %p, param = %p = \"%s\", preset = %p, parent = %p)\n", pw, param, param->name, preset, parent);
	if (!pw || !param)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("param->id = %d.%d.%d\n", param->id.preset_id, param->id.effect_id, param->id.parameter_id);
	
	pw->param = param;
	pw->preset = preset;
	
	pw->parent = parent;
	
	param->pw = pw;
	
	KEST_PRINTF("param->pw = %p\n", param->pw);
	
	#ifdef KEST_ENABLE_REPRESENTATIONS
	pw->rep.representee = param;
	param->widget_rep.representer = pw;
	kest_representation_ptr_list_append(&param->reps, &pw->rep);
	#endif
	
	pw->driven = param->driver_index != KEST_PARAMETER_UNDRIVEN;
	
	pw->nominal_value = kest_parameter_evaluate(pw->param);
	format_float(pw->val_label_text, pw->nominal_value, PARAM_WIDGET_LABEL_BUFSIZE);
	
	return NO_ERROR;
}

void kest_parameter_widget_refresh_timer_wrapper(lv_timer_t *timer)
{
	kest_parameter_widget* pw = lv_timer_get_user_data(timer);
	
	if (!pw)
		return;
	
	kest_parameter_widget_refresh(pw);
	
	if (pw->timer && fabs(pw->nominal_value - pw->param->value) < 0.001)
	{
		pw->nominal_value = pw->param->value;
		lv_timer_del(pw->timer);
		pw->timer = NULL;
	}
}


void kest_parameter_widget_refresh_async_wrapper(void *pw)
{
	kest_parameter_widget_refresh((kest_parameter_widget*)pw);
}

int kest_parameter_widget_refresh_async(kest_parameter_widget *pw)
{
	if (!pw)
		return ERR_NULL_PTR;
	
	if (kest_system_time_ms < pw->last_refresh_ms)
		return NO_ERROR;
	
	kest_ui_async_call(kest_parameter_widget_refresh_async_wrapper, (void*)pw);
	
	return NO_ERROR;
}

int kest_parameter_widget_refresh(kest_parameter_widget *pw)
{
	KEST_PRINTF("kest_parameter_widget_refresh(pw = %p)\n", pw);
	
	if (!pw)
		return ERR_NULL_PTR;
	
	if (!pw->param)
		return ERR_BAD_ARGS;
	
	int ret_val = NO_ERROR;
	
	int32_t ms = kest_system_time_ms();
	pw->last_refresh_ms = ms;
	
	KEST_PRINTF("pw->pressed = %d\n", pw->pressed);
	KEST_PRINTF("pw->nominal_value = %f, pw->param->value = %f\n", pw->nominal_value, pw->param->value);
	
	if (!pw->pressed)
	{
		ret_val = parameter_widget_update_value(pw);
		
		if (ret_val != NO_ERROR)
			return ret_val;
		
		// Apparently this function doesn't return, lol
		/*ret_val = */parameter_widget_update_value_label(pw);
		
		//if (!pw->driven && fabs(pw->nominal_value - pw->param->value) > 0.001 && !pw->timer)
		//	pw->timer = lv_timer_create(kest_parameter_widget_refresh_timer_wrapper, 10, pw);
	}
	
	return ret_val;
}
#define PRINTLINES_ALLOWED 0

void parameter_widget_refresh(kest_parameter_widget *pw)
{
	if (!pw)
	{
		return;
	}
	
	if (!pw->pressed)
	{
		parameter_widget_update_value(pw);
		parameter_widget_update_value_label(pw);
	}
	
	
	int32_t ms = kest_system_time_ms();
	pw->last_refresh_ms = ms;
}

void parameter_widget_refresh_cb(lv_event_t *event)
{
	kest_parameter_widget *pw = lv_event_get_user_data(event);
	
	if (!pw)
	{
		KEST_PRINTF("NULL pw pointer");
		return;
	}
	
	parameter_widget_update_value(pw);
	parameter_widget_update_value_label(pw);
	
	
	int32_t ms = kest_system_time_ms();
	pw->last_refresh_ms = ms;
}

void parameter_widget_change_cb_inner(kest_parameter_widget *pw)
{
	KEST_PRINTF("parameter_widget_change_cb_inner(pw = %p)\n", pw);
	if (!pw)
	{
		KEST_PRINTF("NULL pw pointer passed to parameter_widget_change_cb_inner");
		return;
	}
	
	kest_parameter *param = pw->param;
	
	if (!param)
	{
		KEST_PRINTF("parameter_widget_change_cb_inner called on parameter widget with NULL parameter");
		return;
	}
	
	float val = 0.0f;
	
	kest_interval range = kest_parameter_get_range(param);
	
	float min = range.a;
	float max = range.b;
	
	switch (param->widget_type)
	{
		case PARAM_WIDGET_VSLIDER_TALL:
		case PARAM_WIDGET_VSLIDER:
		case PARAM_WIDGET_HSLIDER:
			val = (float)lv_slider_get_value(pw->obj);
			break;
			
		default:
			val = (float)lv_arc_get_value(pw->obj);
			break;
	}
	
	val /= PARAMETER_WIDGET_RANGE_SIZE;
	
	switch (param->scale)
	{
		case PARAMETER_SCALE_LOGARITHMIC:
			float lnmin = logf(min);
			float lnmax = logf(max);
			
			val = expf(lnmin + val * (lnmax - lnmin));
			break;
		
		default:
			val = min + val * (max - min);
			break;
	}
	
	KEST_PRINTF("Calculated nominal value %s%.05f\n", val >= 0 ? " " : "", val);
	
	pw->nominal_value = val;
	
	parameter_widget_update_value_label_v(pw, val);
	
	kest_parameter_trigger_update(param, val);
	
	if (pw->preset)
	{
		pw->preset->unsaved_changes = 1;
	}
	else if (param->id.preset_id == CONTEXT_PRESET_ID)
	{
		// do something
	}
	
	int32_t ms = kest_system_time_ms();
	pw->last_refresh_ms = ms;
}

void parameter_widget_change_cb(lv_event_t *event)
{
	kest_parameter_widget *pw = lv_event_get_user_data(event);
	
	if (!pw)
	{
		KEST_PRINTF("NULL pw pointer passed to parameter_widget_change_cb");
		return;
	}
	
	parameter_widget_change_cb_inner(pw);
}

void parameter_widget_pressing_cb(lv_event_t *event)
{
	kest_parameter_widget *pw = lv_event_get_user_data(event);
	
	if (pw)
	{
		pw->pressed = 1;
		
		if (pw->param)
		{
			pw->param->driver_override = 1;
		}
	}
}


void parameter_widget_released_cb(lv_event_t *event)
{
	kest_parameter_widget *pw = lv_event_get_user_data(event);
	
	if (pw)
	{
		pw->pressed = 0;
		
		if (pw->param)
		{
			pw->param->driver_override = 0;
		}
	}
}

int parameter_widget_create_ui(kest_parameter_widget *pw, lv_obj_t *parent)
{
	if (!pw)
		return ERR_NULL_PTR;
	
	int ret_val;
	if ((ret_val = parameter_widget_create_ui_ncbsf(pw, parent, 1.0f)) != NO_ERROR)
		return ret_val;
	
	lv_obj_add_event_cb(pw->obj, parameter_widget_change_cb, LV_EVENT_VALUE_CHANGED, pw);
	
	lv_obj_add_event_cb(pw->obj, parameter_widget_pressing_cb, LV_EVENT_PRESSING, pw);
	lv_obj_add_event_cb(pw->obj, parameter_widget_released_cb, LV_EVENT_RELEASED, pw);
	
	int32_t ms = kest_system_time_ms();
	pw->last_refresh_ms = ms;
	
	return NO_ERROR;
}


int parameter_widget_create_ui_in_group(kest_parameter_widget *pw, lv_obj_t *parent, int occupants)
{
	if (!pw)
		return ERR_NULL_PTR;
	
	float sf = 1.0;
	
	if (occupants > 2)
	{
		sf = 0.5f * (float)occupants;
	}
	
	int ret_val;
	if ((ret_val = parameter_widget_create_ui_ncbsf(pw, parent, sf)) != NO_ERROR)
		return ret_val;
	
	lv_obj_add_event_cb(pw->obj, parameter_widget_change_cb, LV_EVENT_VALUE_CHANGED, pw);
	
	lv_obj_add_event_cb(pw->obj, parameter_widget_pressing_cb,  LV_EVENT_PRESSING, pw);
	lv_obj_add_event_cb(pw->obj, parameter_widget_released_cb, LV_EVENT_RELEASED, pw);
	
	return NO_ERROR;
}

int parameter_widget_create_ui_ncbsf(kest_parameter_widget *pw, lv_obj_t *parent, float sf)
{
	
	KEST_PRINTF("parameter_widget_create_ui_no_callback(pw = %p, parent = %p)\n", pw, parent);
	if (!pw || !pw->param || !parent)
		return ERR_NULL_PTR;
	
	pw->container = lv_obj_create(parent);
	KEST_PRINTF("pw->container = %p\n", pw->container);
	lv_obj_remove_style_all(pw->container);
	
	KEST_PRINTF("parameter_widget_create_ui_no_callback, parameter %d.%d.%d at %p\n",
		pw->param->id.preset_id, 
		pw->param->id.effect_id, 
		pw->param->id.parameter_id,
		pw->param);
	KEST_PRINTF("parameter name: \"%s\"\n",
		pw->param->name);
	KEST_PRINTF("cname: (%s)\n",
		pw->param->name_internal ? pw->param->name_internal : "NULL");
	KEST_PRINTF("param->min_expr = %p,\n",
		pw->param->min_expr);
	KEST_PRINTF("param->max_expr = %p\n",
		pw->param->max_expr);
	
	int height = 0;
	int width = 0;
	
	switch (pw->param->widget_type)
	{	
		case PARAM_WIDGET_HSLIDER:
			pw->obj = lv_slider_create(pw->container);
			
			height = (int)((float)HSLIDER_SIZE_H * sf);
			width  = (int)((float)HSLIDER_SIZE_W * sf);
			
			lv_obj_align(pw->obj, LV_ALIGN_CENTER, 0, 20);
			lv_obj_set_size(pw->obj, width, height);
			
			lv_obj_set_size(pw->container, HSLIDER_SIZE_W + HSLIDER_PAD_W, HSLIDER_SIZE_H + HSLIDER_PAD_H);
			
			pw->name_label = lv_label_create(pw->container);
			lv_label_set_text(pw->name_label, pw->param->name);
			lv_obj_align_to(pw->name_label, pw->obj, LV_ALIGN_OUT_TOP_LEFT, 20, -35);
			
			pw->val_label = lv_label_create(pw->container);
			lv_obj_align_to(pw->val_label, pw->obj, LV_ALIGN_OUT_TOP_RIGHT, 30, -35);
			
			lv_slider_set_range(pw->obj, 0, (int)PARAMETER_WIDGET_RANGE_SIZE);
			break;
		
		case PARAM_WIDGET_VSLIDER:
			pw->obj = lv_slider_create(pw->container);
			
			height = (int)((float)VSLIDER_SIZE_H * sf);
			width  = (int)((float)VSLIDER_SIZE_W * sf);
			
			lv_obj_align(pw->obj, LV_ALIGN_CENTER, 0, VSLIDER_PAD_H / 2);
			lv_obj_set_size(pw->obj, width, height);
			
			lv_obj_set_size(pw->container, VSLIDER_SIZE_W + VSLIDER_PAD_W, VSLIDER_SIZE_H + VSLIDER_PAD_H);
			
			pw->name_label = lv_label_create(pw->container);
			lv_label_set_text(pw->name_label, pw->param->name);
			lv_obj_align_to(pw->name_label, pw->obj, LV_ALIGN_OUT_TOP_MID, 0, 20);
			
			pw->val_label = lv_label_create(pw->container);
			lv_obj_align_to(pw->val_label, pw->obj, LV_ALIGN_OUT_BOTTOM_LEFT, 15, 0);
			
			lv_slider_set_range(pw->obj, 0, (int)PARAMETER_WIDGET_RANGE_SIZE);
			break;
		
		case PARAM_WIDGET_VSLIDER_TALL:
			pw->obj = lv_slider_create(pw->container);
			
			
			height = 			  VSLIDER_TALL_SIZE_H;
			width  = (int)((float)VSLIDER_TALL_SIZE_W * sf);
			
			lv_obj_align(pw->obj, LV_ALIGN_CENTER, 0, 30);
			lv_obj_set_size(pw->obj, width, height);
			
			lv_obj_set_size(pw->container, VSLIDER_TALL_SIZE_W + VSLIDER_TALL_PAD_W, VSLIDER_TALL_SIZE_H + VSLIDER_TALL_PAD_H);
			
			pw->name_label = lv_label_create(pw->container);
			lv_label_set_text(pw->name_label, pw->param->name);
			lv_obj_align_to(pw->name_label, pw->obj, LV_ALIGN_TOP_MID, 0, -30);
			
			pw->val_label = lv_label_create(pw->container);
			lv_obj_align_to(pw->val_label, pw->obj, LV_ALIGN_OUT_BOTTOM_MID, -5, 15);
			
			lv_slider_set_range(pw->obj, 0, (int)PARAMETER_WIDGET_RANGE_SIZE);
			break;
			
		default:
			pw->obj = lv_arc_create(pw->container);
			
			height = (int)((float)VIRTUAL_POT_SIZE_H * sf) + VPOT_PAD_H;
			width  = (int)((float)VIRTUAL_POT_SIZE_W * sf) + VPOT_PAD_W;
			
			lv_obj_set_size(pw->container, width, height);
			
			lv_obj_align(pw->obj, LV_ALIGN_CENTER, 0, -15);
			lv_obj_set_size(pw->obj, VIRTUAL_POT_SIZE_W, VIRTUAL_POT_SIZE_H);
			
			lv_arc_set_rotation(pw->obj, 135);
			lv_arc_set_bg_angles(pw->obj, 0, 270);
			lv_arc_set_range(pw->obj, 0, (int)PARAMETER_WIDGET_RANGE_SIZE);
			
			pw->name_label = lv_label_create(pw->container);
			lv_label_set_text(pw->name_label, pw->param->name);
			lv_obj_align(pw->name_label, LV_ALIGN_BOTTOM_MID, 0, -35);
			lv_obj_set_size(pw->name_label, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
			
			pw->val_label = lv_label_create(pw->obj);
			lv_obj_center(pw->val_label);
			break;
	}
	
	parameter_widget_update_value(pw);
	parameter_widget_update_value_label(pw);
	
	return NO_ERROR;
}

void free_parameter_widget(kest_parameter_widget *pw)
{
	if (!pw)
		return;
	
	// Currently the kest_parameter_widget struct contains nothing that
	// it owns itself. This may change !
	kest_free(pw);
}

int kest_parameter_widget_align_nominal_value(kest_parameter_widget *pw)
{
	if (!pw)
		return ERR_NULL_PTR;
	
	if (!pw->param)
		return ERR_BAD_ARGS;
	
	pw->nominal_value = kest_parameter_evaluate(pw->param);
	
	return NO_ERROR;
}

//
//
//
// === setting widget ===
//
//
//


int setting_widget_update_value(kest_setting_widget *sw);

void setting_widget_rep_update(void *representer, void *representee)
{
	kest_setting_widget *sw = representer;
	kest_setting *setting = representee;
	
	if (!sw || !setting)
		return;
	
	setting_widget_update_value(sw);
}

int nullify_setting_widget(kest_setting_widget *sw)
{
	if (!sw)
		return ERR_NULL_PTR;
	
	sw->setting = NULL;
	
	sw->obj = NULL;
	
	sw->type = SETTING_WIDGET_DROPDOWN;
	sw->saved_field_text = NULL;
	
	sw->parent = NULL;
	
	sw->rep.representer = sw;
	sw->rep.representee = NULL;
	sw->rep.update = setting_widget_rep_update;
	
	return NO_ERROR;
}


void sw_field_display_bare_value(kest_setting_widget *sw)
{
	if (!sw) return;
	if (!sw->setting || !sw->obj) return;
	
	char buf[32];
	
	snprintf(buf, 32, "%d", sw->setting->value);
	
	KEST_PRINTF("setting field value to \"%s\"\n", buf);
	lv_textarea_set_text(sw->obj, buf);
}

void sw_field_display_value_with_units(kest_setting_widget *sw)
{
	KEST_PRINTF("\n");
	if (!sw) return;
	KEST_PRINTF("\n");
	if (!sw->setting || !sw->obj) return;
	KEST_PRINTF("\n");
	
	char buf[32];
	KEST_PRINTF("\n");
	
	int l = snprintf(buf, 32, "%d", sw->setting->value);
	KEST_PRINTF("\n");
	if (l < 32 && sw->setting->units)
	{
		KEST_PRINTF("\n");
		snprintf(&buf[l], 32 - l, "%s", sw->setting->units);
		KEST_PRINTF("\n");
	}
	
	KEST_PRINTF("setting field value to \"%s\"\n", buf);
	lv_textarea_set_text(sw->obj, buf);
}

int setting_widget_update_value(kest_setting_widget *sw)
{
	KEST_PRINTF("setting_widget_update_value(sw = %p)\n", sw);
	if (!sw)
		return ERR_NULL_PTR;
	
	KEST_PRINTF(" ... sw->setting = %p, sw->obj = %p\n", sw->setting, sw->obj);
	if (!sw->setting || !sw->obj)
		return ERR_BAD_ARGS;
	
	KEST_PRINTF("sw->type = %d = %s\n", sw->type, (sw->type == SETTING_WIDGET_DROPDOWN)
		? "SETTING_WIDGET_DROPDOWN"
		: ((sw->type == SETTING_WIDGET_SWITCH)
			? "SETTING_WIDGET_SWITCH"
			: ((sw->type == SETTING_WIDGET_FIELD)
				? "SETTING_WIDGET_FIELD"
				: "??")));
	switch (sw->type)
	{
		case SETTING_WIDGET_DROPDOWN:
			KEST_PRINTF("sw->setting->options = %p, sw->setting->n_options = %d\n", sw->setting->options, sw->setting->n_options);
			if (!sw->setting->options)
				return ERR_BAD_ARGS;
			
			for (int i = 0; i < sw->setting->n_options; i++)
			{
				KEST_PRINTF("Option %d: value %d, name \"%s\"\n", i, sw->setting->options[i].value, sw->setting->options[i].name);
				if (sw->setting->options[i].value == sw->setting->value)
				{
					lv_dropdown_set_selected(sw->obj, i);
				}
			}
			break;
			
		case SETTING_WIDGET_SWITCH:
			break;
			
		case SETTING_WIDGET_FIELD:
			KEST_PRINTF("It is a field\n");
			sw_field_display_value_with_units(sw);
			break;
		
		default:
			return ERR_BAD_ARGS;
			break;
	}
	
	return NO_ERROR;
}

int configure_setting_widget(kest_setting_widget *sw, kest_setting *setting, kest_preset *preset, kest_ui_page *parent)
{
	if (!sw || !setting)
		return ERR_NULL_PTR;
	
	sw->setting = setting;
	sw->preset  = preset;
	sw->parent  = parent;
	
	sw->type = sw->setting->widget_type;
	
	//kest_representation_pll_safe_append(&setting->reps, &sw->rep);
	
	//sw->rep.representee = setting;
	
	return NO_ERROR;
}

void setting_widget_refresh_cb(lv_event_t *event)
{
	kest_setting_widget *sw = lv_event_get_user_data(event);
	
	if (!sw)
		return;
	
	sw_field_display_value_with_units(sw);
}

int setting_widget_calc_value(kest_setting_widget *sw, int16_t *target)
{
	if (!sw || !target)
		return ERR_NULL_PTR;
	
	if (!sw->obj)
		return ERR_BAD_ARGS;
	
	kest_setting *setting = sw->setting;
	
	if (!setting)
		return ERR_BAD_ARGS;
	
	char buf[128];
	int found = 0;
	switch (sw->type)
	{
		case SETTING_WIDGET_DROPDOWN:
			lv_dropdown_get_selected_str(sw->obj, buf, 128);
	
			for (int i = 0; i < setting->n_options; i++)
			{
				if (strncmp(buf, setting->options[i].name, 128) == 0)
				{
					found = 1;
					*target = setting->options[i].value;
				}
			}
			
			if (!found)
			{
				return ERR_BAD_ARGS;
			}
			break;
		
		default:
			return ERR_UNIMPLEMENTED;
	}
	
	return NO_ERROR;
}

void sw_field_reject(kest_setting_widget *sw)
{
	if (!sw)
		return;
	
	sw_field_display_value_with_units(sw);
}

void sw_field_save_cb(lv_event_t *e)
{
	kest_setting_widget *sw = lv_event_get_user_data(e);
	
	if (!sw)
		return;
	
	if (!sw->setting)
		return;
	
	// Parse an int!
	kest_preset *preset;
	
	char *content = kest_strndup(lv_textarea_get_text(sw->obj), 32);
	
	if (!content)
	{
		return;
	}
	
	KEST_PRINTF("Field contains the text \"%s\"...\n", content);
	
	int read_int = 0;
	int valid_string = 0;
	
	// ... tbh just delete anything not a number
	for (int i = 0; content[i]; i++)
	{
		if (content[i] < '0' || content[i] > '9')
		{
			if (content[i] == '.')
			{
				content[i] = 0;
				break;
			}
			
			for (int j = i; content[j]; j++)
			{
				content[j] = content[j + 1];
			}
		}
	}
	
	if (!content[0])
	{
		sw_field_reject(sw);
		return;
	}
	
	for (int i = 0; content[i]; i++)
	{
		read_int = read_int * 10 + (int)((uint8_t)content[i] - (uint8_t)'0');
	}
	
	KEST_PRINTF("Read in the int %d\n", read_int);
	
	KEST_PRINTF("read_int = binary_min(binary_max(read_int, sw->setting->min), sw->setting->max) = binary_min(binary_max(%d, %d), %d) = binary_min(%d, %d) = %d\n",
		read_int, sw->setting->min, sw->setting->max, binary_max(read_int, sw->setting->min), sw->setting->max, binary_min(binary_max(read_int, sw->setting->min), sw->setting->max));
	read_int = binary_min(binary_max(read_int, sw->setting->min), sw->setting->max);
	
	if (sw->setting->value != read_int)
	{
		sw->setting->old_value = sw->setting->value;
		
		sw->setting->value = read_int;
		sw->setting->updated = 1;
		
		#ifdef KEST_ENABLE_FPGA
		preset = cxt_get_preset_by_id(&global_cxt, sw->setting->id.preset_id);
		KEST_PRINTF("Setting widget value changed from %d to %d; reprogramming FPGA in light. preset = %p\n",
				sw->setting->old_value, sw->setting->value, preset);
		if (preset)
		{
			kest_preset_if_active_update_fpga(preset);
		}
		#endif
		
		#ifdef USE_TEENSY
		kest_message msg = create_m_message(KEST_MESSAGE_SET_SETTING_VALUE, "ssss", sw->setting->id.preset_id, sw->setting->id.effect_id, sw->setting->id.setting_id, read_int);
		
		queue_msg_to_teensy(msg);
		#endif
	}
	
	kest_free(content);
	
	sw_field_display_value_with_units(sw);
	
	hide_keyboard();
	lv_obj_clear_state(sw->obj, LV_STATE_FOCUSED);
	
	KEST_PRINTF("sw_field_save_cb done\n");
}

void sw_field_cancel_cb(lv_event_t *e)
{
	KEST_PRINTF("sw_field_cancel_cb\n");
	kest_setting_widget *sw = lv_event_get_user_data(e);
	
	if (!sw)
		return;
	
	KEST_PRINTF("%s:%d\n", __func__, __LINE__);
	if (sw->saved_field_text)
	{
		KEST_PRINTF("%s:%d\n", __func__, __LINE__);
		lv_textarea_set_text(sw->obj, sw->saved_field_text);
		KEST_PRINTF("%s:%d\n", __func__, __LINE__);
		kest_free(sw->saved_field_text);
		KEST_PRINTF("%s:%d\n", __func__, __LINE__);
		sw->saved_field_text = NULL;
		KEST_PRINTF("%s:%d\n", __func__, __LINE__);
	}
	hide_keyboard();
	
	sw_field_display_value_with_units(sw);
	KEST_PRINTF("sw_field_cancel_cb done\n");
}

void edit_sw_field_cb(lv_event_t *e)
{
	KEST_PRINTF("edit_sw_field_cb\n");
	kest_setting_widget *sw = lv_event_get_user_data(e);
	
	if (!sw)
		return;
	
	if (!sw->obj || !sw->parent)
		return;
	
	sw->saved_field_text = kest_strndup(lv_textarea_get_text(sw->obj), 32);
	
	KEST_PRINTF("sw->setting->value = %d\n", sw->setting->value);
	sw_field_display_bare_value(sw);
	KEST_PRINTF("edit_sw_field_cb done\n");
	
	spawn_numerical_keyboard(sw->parent->screen, sw->obj, sw_field_save_cb, sw, sw_field_cancel_cb, sw);
	KEST_PRINTF("spawned numerical keyboard\n");
	lv_obj_add_state(sw->obj, LV_STATE_FOCUSED);
}

void setting_widget_change_cb_inner(kest_setting_widget *sw)
{
	if (!sw)
		return;
	
	int16_t value;
	
	int ret_val;
	
	if ((ret_val = setting_widget_calc_value(sw, &value)) != NO_ERROR)
	{
		KEST_PRINTF("Error %s getting setting widget value\n", kest_error_code_to_string(ret_val));
		return;
	}
	
	if (sw->setting)
	{
		sw->setting->value = value;
		sw->setting->updated = 1;
	}
	
	KEST_PRINTF("setting_widget_change_cb_inner. value = %d\n", value);
	#ifdef USE_TEENSY
	kest_message msg = create_m_message(KEST_MESSAGE_SET_SETTING_VALUE, "ssss", sw->setting->id.preset_id, sw->setting->id.effect_id, sw->setting->id.setting_id, value);

	queue_msg_to_teensy(msg);
	#endif
	
	if (sw->preset)
	{
		sw->preset->unsaved_changes = 1;
	}
	else
	{
		// do something
	}
}

void setting_widget_change_cb(lv_event_t *event)
{
	kest_setting_widget *sw = lv_event_get_user_data(event);
	
	if (!sw)
	{
		KEST_PRINTF("NULL virtual sw pointer");
		return;
	}
	
	setting_widget_change_cb_inner(sw);
}

int setting_widget_create_ui(kest_setting_widget *sw, lv_obj_t *parent)
{
	KEST_PRINTF("setting_widget_create_ui(sw = %p, parent = %p)\n", sw, parent);
	if (!sw)
		return ERR_NULL_PTR;
	
	int ret_val;
	if ((ret_val = setting_widget_create_ui_no_callback(sw, parent)) != NO_ERROR)
	{
		KEST_PRINTF("setting_widget_create_ui line %d\n", __LINE__);
		return ret_val;
	}
	
	KEST_PRINTF("setting_widget_create_ui line %d\n", __LINE__);
	switch (sw->type)
	{
		case SETTING_WIDGET_DROPDOWN:
			lv_obj_add_event_cb(sw->obj, setting_widget_change_cb, LV_EVENT_VALUE_CHANGED, sw);
			break;
		
		case SETTING_WIDGET_SWITCH:
			
			break;
		
		case SETTING_WIDGET_FIELD:
			lv_obj_add_event_cb(sw->obj, edit_sw_field_cb, LV_EVENT_CLICKED, sw);
			break;
	}
	
	KEST_PRINTF("setting_widget_create_ui done\n");
	return NO_ERROR;
}

int setting_widget_create_ui_no_callback(kest_setting_widget *sw, lv_obj_t *parent)
{
	KEST_PRINTF("setting_widget_create_ui_no_callback(sw = %p, parent = %p)\n", sw, parent);
	
	if (!sw || !sw->setting || !parent)
	{
		return ERR_NULL_PTR;
	}
	
	sw->container = lv_obj_create(parent);
	lv_obj_remove_style_all(sw->container);
	lv_obj_clear_flag(sw->container, LV_OBJ_FLAG_SCROLLABLE);
	
	KEST_PRINTF("sw->type = %d = %s\n", sw->type, (sw->type == SETTING_WIDGET_DROPDOWN)
		? "SETTING_WIDGET_DROPDOWN"
		: ((sw->type == SETTING_WIDGET_SWITCH)
			? "SETTING_WIDGET_SWITCH"
			: ((sw->type == SETTING_WIDGET_FIELD)
				? "SETTING_WIDGET_FIELD"
				: "??")));
	
	switch (sw->setting->widget_type)
	{	
		case SETTING_WIDGET_DROPDOWN:
			KEST_PRINTF("Creating label for setting %s\n", sw->setting->name);
			
			lv_obj_set_layout(sw->container, LV_LAYOUT_FLEX);
			lv_obj_set_flex_flow(sw->container, LV_FLEX_FLOW_ROW);
			lv_obj_set_flex_align(sw->container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
			
			sw->label = lv_label_create(sw->container);
			lv_label_set_text(sw->label, sw->setting->name);
			lv_obj_set_flex_grow(sw->label, 2);
			
			sw->pad = lv_obj_create(sw->container);
			lv_obj_remove_style_all(sw->pad);
			
			sw->obj = lv_dropdown_create(sw->container);
			lv_dropdown_clear_options(sw->obj);
			lv_obj_set_width(sw->obj, STANDARD_CONTAINER_WIDTH / 3);
			
			lv_obj_set_size(sw->container, STANDARD_CONTAINER_WIDTH - 40, STANDARD_BUTTON_SHORT_HEIGHT);
			
			for (int i = 0; i < sw->setting->n_options; i++)
			{
				lv_dropdown_add_option(sw->obj, sw->setting->options[i].name, i);
			}
			
			lv_dropdown_close(sw->obj);
			break;
			
		case SETTING_WIDGET_SWITCH:
			break;
			
		case SETTING_WIDGET_FIELD:
			lv_obj_set_layout(sw->container, LV_LAYOUT_FLEX);
			lv_obj_set_flex_flow(sw->container, LV_FLEX_FLOW_ROW);
			lv_obj_set_flex_align(sw->container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
			
			KEST_PRINTF("Creating label for setting %s\n", sw->setting->name);
			sw->label = lv_label_create(sw->container);
			lv_label_set_text(sw->label, sw->setting->name);
			lv_obj_set_flex_grow(sw->label, 2);
			
			sw->pad = lv_obj_create(sw->container);
			lv_obj_remove_style_all(sw->pad);
			
			sw->obj = lv_textarea_create(sw->container);
			lv_obj_set_style_text_align(sw->obj, LV_TEXT_ALIGN_RIGHT, 0);
			lv_textarea_set_one_line(sw->obj, true);
			lv_obj_set_width(sw->obj, STANDARD_CONTAINER_WIDTH / 3 - 20);
			
			lv_obj_set_size(sw->container, STANDARD_CONTAINER_WIDTH - 60, STANDARD_BUTTON_SHORT_HEIGHT);
			break;
	}
	
	KEST_PRINTF("setting_widget_create_ui_no_callback nearly finished\n");
	setting_widget_update_value(sw);
	
	KEST_PRINTF("setting_widget_create_ui_no_callback done\n");
	
	return NO_ERROR;
}

void free_setting_widget(kest_setting_widget *sw)
{
	if (!sw)
		return;
	
	// Currently the kest_setting_widget struct contains nothing that
	// it owns itself. This may change !
	kest_free(sw);
}

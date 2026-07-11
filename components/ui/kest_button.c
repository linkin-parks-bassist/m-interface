#include "kest_int.h"

static const char *FNAME = "kest_button.c";
#define PRINTLINES_ALLOWED 0

IMPLEMENT_LINKED_PTR_LIST(kest_active_button);

int init_button(kest_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->flags = 0;
	
	button->obj 				= NULL;
	button->label 				= NULL;
	button->label_text 			= NULL;
	button->clicked_cb 			= NULL;
	button->clicked_cb_arg 		= NULL;
	button->pressing_cb 		= NULL;
	button->pressing_cb_arg 	= NULL;
	button->long_pressed_cb 	= NULL;
	button->long_pressed_cb_arg = NULL;
	button->released_cb 		= NULL;
	button->released_cb_arg 	= NULL;
	
	button->hider = NULL;
	button->clickable = 1;
	
	button->long_pressed = 0;
	
	button->draggable_x = 0;
	button->draggable_y = 0;
	
	button->width  = KEST_BUTTON_WIDTH;
	button->height = KEST_BUTTON_HEIGHT;
	
	button->alignment = LV_ALIGN_CENTER;
	button->align_offs_x = 0;
	button->align_offs_y = 0;
	
	button->n_sub_buttons = 0;
	for (int i = 0; i < KEST_BUTTON_MAX_SUB_BUTTONS; i++)
		button->sub_buttons[i] = NULL;
	
	button->opacity = 255;
	button->label_opacity = 255;
	
	return NO_ERROR;
}

kest_button *new_button(const char *label)
{
	kest_button *button = kest_alloc(sizeof(kest_button));
	
	if (!button)
		return NULL;
	
	init_button(button);
	
	button->label_text = label;
	
	return button;
}

int create_button_ui(kest_button *button, lv_obj_t *parent)
{
	if (!button)
		return ERR_NULL_PTR;
	
	if (button->obj)
	{
		KEST_PRINTF("WARNING: attempt to call create_button_ui on a button for which button->obj is not NULL. button->obj = %p", button->obj);
		return ERR_BAD_ARGS;
	}
	
	button->obj = lv_btn_create(parent);
	
	if (!button->obj)
		return ERR_ALLOC_FAIL;
		
	if (button->flags & KEST_BUTTON_FLAG_DISABLED)
		lv_obj_add_state(button->obj, LV_STATE_DISABLED);
	
	if (button->flags & KEST_BUTTON_FLAG_UNCLICKABLE)
		lv_obj_clear_flag(button->obj, LV_OBJ_FLAG_CLICKABLE);
	
	if (!(button->flags & KEST_BUTTON_FLAG_NO_ALIGN))
		lv_obj_align(button->obj, button->alignment, button->align_offs_x, button->align_offs_y);
	
	//kest_printf("create_button_ui: button->obj = %p\n", button->obj);
	
	button->label = lv_label_create(button->obj);
	
	if (!button->label)
	{
		lv_obj_del(button->obj);
		button->obj = NULL;
		return ERR_ALLOC_FAIL;
	}
	
	if (button->flags & KEST_BUTTON_FLAG_HIDDEN)
	{
		lv_obj_add_flag(button->obj, LV_OBJ_FLAG_HIDDEN);
		
	}
	else
	{
		lv_obj_set_style_opa(button->obj, button->opacity, 0);
		lv_obj_set_style_opa(button->label, button->label_opacity, 0);
	}
	
	int ret_val;
	for (int i = 0; i < button->n_sub_buttons; i++)
	{
		if (button->sub_buttons[i])
		{
			if ((ret_val = create_button_ui(button->sub_buttons[i], button->obj)) != NO_ERROR)
			{
				lv_obj_del(button->label);
				button->label = NULL;
				lv_obj_del(button->obj);
				button->obj = NULL;
				return ret_val;
			}
		}
	}
	
	lv_obj_set_size(button->obj, button->width, button->height);
	lv_label_set_text(button->label, button->label_text);
	lv_obj_center(button->label);
	
	if (button->clicked_cb)
		lv_obj_add_event_cb(button->obj, button->clicked_cb, LV_EVENT_CLICKED, button->clicked_cb_arg);
	
	if (button->pressing_cb)
		lv_obj_add_event_cb(button->obj, button->pressing_cb, LV_EVENT_PRESSING, button->pressing_cb_arg);
	
	if (button->long_pressed_cb)
		lv_obj_add_event_cb(button->obj, button->long_pressed_cb, LV_EVENT_LONG_PRESSED, button->long_pressed_cb_arg);
	
	if (button->released_cb)
		lv_obj_add_event_cb(button->obj, button->released_cb, LV_EVENT_RELEASED, button->released_cb_arg);
	
	return NO_ERROR;
}

int kest_button_create_label_ui(kest_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	if (!button->obj)
		return ERR_BAD_ARGS;
	
	button->label = lv_label_create(button->obj);
	lv_obj_center(button->label);
	
	return NO_ERROR;
}

int button_set_clicked_cb(kest_button *button, lv_event_cb_t cb, void *cb_arg)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->clicked_cb = cb;
	button->clicked_cb_arg = cb_arg;
	
	if (button->obj)
		lv_obj_add_event_cb(button->obj, button->clicked_cb, LV_EVENT_CLICKED, button->clicked_cb_arg);
	
	return NO_ERROR;
}

int button_set_pressing_cb(kest_button *button, lv_event_cb_t cb, void *cb_arg)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->pressing_cb = cb;
	button->pressing_cb_arg = cb_arg;
	
	if (button->obj)
		lv_obj_add_event_cb(button->obj, button->pressing_cb, LV_EVENT_PRESSING, button->pressing_cb_arg);
	
	return NO_ERROR;
}

int button_set_long_pressed_cb(kest_button *button, lv_event_cb_t cb, void *cb_arg)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->long_pressed_cb = cb;
	button->long_pressed_cb_arg = cb_arg;
	
	if (button->obj)
		lv_obj_add_event_cb(button->obj, button->long_pressed_cb, LV_EVENT_LONG_PRESSED, button->long_pressed_cb_arg);
	
	return NO_ERROR;
}

int button_set_released_cb(kest_button *button, lv_event_cb_t cb, void *cb_arg)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->released_cb = cb;
	button->released_cb_arg = cb_arg;
	
	if (button->obj)
		lv_obj_add_event_cb(button->obj, button->released_cb, LV_EVENT_RELEASED, button->released_cb_arg);
		
	return NO_ERROR;
}

int kest_button_set_label(kest_button *button, const char *label)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->label_text = label;
	
	if (button->obj && label)
	{
		if (!button->label)
			kest_button_create_label_ui(button);
		
		lv_label_set_text(button->label, label);
	}
	
	return NO_ERROR;
}


int kest_button_disable_alignment(kest_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->flags |= KEST_BUTTON_FLAG_NO_ALIGN;
	
	return NO_ERROR;
}

int kest_button_set_alignment(kest_button *button, lv_align_t align, int offs_x, int offs_y)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->flags &= ~KEST_BUTTON_FLAG_NO_ALIGN;
	button->alignment = align;
	
	button->align_offs_x = offs_x;
	button->align_offs_y = offs_y;
	
	if (button->obj)
		lv_obj_align(button->obj, button->alignment, offs_x, offs_y);	
	
	return NO_ERROR;
}

int kest_button_set_size(kest_button *button, int width, int height)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->width = width;
	button->height = height;
	
	if (button->obj)
		lv_obj_set_size(button->obj, width, height);	
	
	return NO_ERROR;
}

int kest_button_add_sub_button(kest_button *button, kest_button *sub_button)
{
	if (!button || !sub_button)
		return ERR_NULL_PTR;
	
	if (button->n_sub_buttons >= KEST_BUTTON_MAX_SUB_BUTTONS)
		return ERR_BAD_ARGS;
	
	button->sub_buttons[button->n_sub_buttons++] = sub_button;
	
	return NO_ERROR;
}

int kest_button_hide(kest_button *button)
{
	//kest_printf("kest_button_hide. button = %p\n", button);
	if (!button)
	{
		//kest_printf("bailing\n");
		return ERR_NULL_PTR;
	}
	
	//kest_printf("set hidden flag...\n");
	button->flags |= KEST_BUTTON_FLAG_HIDDEN;
	
	if (button->obj)
	{
		//kest_printf("set hidden lv flag\n");
		lv_obj_add_flag(button->obj, LV_OBJ_FLAG_HIDDEN);
	}
	
	//kest_printf("kest_button_hide done\n");
	return NO_ERROR;
}

int kest_button_unhide(kest_button *button)
{
	//kest_printf("kest_button_unhide. button = %p\n", button);
	if (!button)
	{
		//kest_printf("bailing\n");
		return ERR_NULL_PTR;
	}
	
	//kest_printf("unset hidden flag...\n");
	button->flags &= (~KEST_BUTTON_FLAG_HIDDEN);
	
	if (button->obj)
	{
		//kest_printf("unset hidden lv flag\n");
		lv_obj_clear_flag(button->obj, LV_OBJ_FLAG_HIDDEN);
		//kest_printf("restore opacity %d\n", button->opacity);
		lv_obj_set_style_opa(button->obj, button->opacity, 0);
	}
	
	return NO_ERROR;
}

int kest_button_set_clickable(kest_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->flags &= (~KEST_BUTTON_FLAG_UNCLICKABLE);
	
	if (button->obj)
	{
		lv_obj_add_flag(button->obj, LV_OBJ_FLAG_CLICKABLE);
	}
	
	return NO_ERROR;
}

int kest_button_set_unclickable(kest_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->flags |= KEST_BUTTON_FLAG_UNCLICKABLE;
	
	if (button->obj)
		lv_obj_clear_flag(button->obj, LV_OBJ_FLAG_CLICKABLE);
	
	return NO_ERROR;
}

int kest_button_set_opacity(kest_button *button, int opacity)
{
	//kest_printf("kest_button_set_opacity. button = %p, opacity = %d\n", button, opacity);
	if (!button)
	{
		//kest_printf("bailing\n");
		return ERR_NULL_PTR;
	}
	
	//kest_printf("store new opacity...\n");
	button->opacity = opacity;
	button->label_opacity = opacity;
	
	if (button->flags & KEST_BUTTON_FLAG_HIDDEN)
	{
		//kest_printf("Button is hidden. Exit\n");
		return NO_ERROR;
	}
	
	if (button->obj)
	{
		//kest_printf("button has UI and it is not hidden. apply to lv_obj...\n");
		lv_obj_set_style_opa(button->obj, button->opacity, 0);
		
		if (button->label)
			lv_obj_set_style_opa(button->label, button->label_opacity, 0);
	}
	
	for (int i = 0; i < KEST_BUTTON_MAX_SUB_BUTTONS; i++)
	{
		if (button->sub_buttons[i])
		{
			if (button->sub_buttons[i]->obj)
				lv_obj_set_style_opa(button->sub_buttons[i]->obj, 0, 0);
			if (button->sub_buttons[i]->label)
				lv_obj_set_style_opa(button->sub_buttons[i]->label, opacity, 0);
		}
	}
	
	//kest_printf("kest_button_set_opacity done\n");
	return NO_ERROR;
}

int kest_button_enable(kest_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->flags &= (~KEST_BUTTON_FLAG_DISABLED);
	
	if (button->obj)
		lv_obj_clear_state(button->obj, LV_STATE_DISABLED);
	
	return NO_ERROR;
}

int kest_button_disable(kest_button *button)
{
	//kest_printf("kest_button_disable\n");
	
	if (!button)
		return ERR_NULL_PTR;
	
	button->flags |= KEST_BUTTON_FLAG_DISABLED;
	
	if (button->obj)
	{
		lv_obj_add_state(button->obj, LV_STATE_DISABLED);
	}
	
	//kest_printf("kest_button_disable done\n");
	return NO_ERROR;
}

void kest_button_enable_async_wrapper(void *button_)
{
	kest_button *button = (kest_button*)button_;
	kest_button_enable(button);
}

void kest_button_disable_async_wrapper(void *button_)
{
	kest_button *button = (kest_button*)button_;
	kest_button_disable(button);
}


int kest_button_enable_async(kest_button *button)
{
	kest_ui_async_call(kest_button_enable_async_wrapper, (void*)button);
	return NO_ERROR;
}

int kest_button_disable_async(kest_button *button)
{
	kest_ui_async_call(kest_button_disable_async_wrapper, (void*)button);
	return NO_ERROR;
}

int kest_button_reset_state(kest_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	if (!button->obj)
		return ERR_BAD_ARGS;
	
	lv_obj_clear_state(button->obj, LV_STATE_PRESSED);
	lv_obj_clear_state(button->obj, LV_STATE_CHECKED);
	lv_obj_clear_state(button->obj, LV_STATE_FOCUSED);
	
	return NO_ERROR;
}

int kest_button_delete_ui(kest_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	for (int i = 0; i < button->n_sub_buttons; i++)
		kest_button_delete_ui(button->sub_buttons[i]);
	
	if (button->label)
	{
		lv_obj_del(button->label);
		button->label = NULL;
	}
	
	if (button->obj)
	{
		lv_obj_del(button->obj);
		button->obj = NULL;
	}
	
	return NO_ERROR;
}

//
// Buttons with "are you sure?" popups
//

int init_danger_button(kest_danger_button *button, void (*action_cb)(void *data), void *cb_arg, kest_ui_page *parent)
{
	if (!button)
		return ERR_NULL_PTR;
	
	init_button(&button->button);
	
	button->parent = parent;
	button->popup  = NULL;
	
	button->action_cb = action_cb;
	button->cb_arg = cb_arg;
	
	button_set_clicked_cb(&button->button, kest_danger_button_activate_popup_cb, &button->button);
	
	return NO_ERROR;
}

int kest_danger_button_create_ui(kest_danger_button *button, lv_obj_t *parent)
{
	if (!button)
		return ERR_NULL_PTR;
	
	create_button_ui(&button->button, parent);
	
	return NO_ERROR;
}

void kest_danger_button_confirm_cb(lv_event_t *e)
{
	kest_danger_button *button = lv_event_get_user_data(e);
	
	lv_msgbox_close(button->popup);
	button->popup = NULL;
	
	if (button->action_cb)
		button->action_cb(button->cb_arg);
}

void kest_danger_button_cancel_cb(lv_event_t *e)
{
	kest_danger_button *button = lv_event_get_user_data(e);
	
	if (button->popup)
		lv_msgbox_close(button->popup);
	
	button->popup = NULL;
}

void kest_danger_button_value_changed_cb(lv_event_t *e)
{
	kest_danger_button *button = lv_event_get_user_data(e);
	
	if (!button)
		return;
	
	if (!button->popup)
		return;
	
	#if LVGL_MAJOR_VERSION == 8
	const char *button_text = lv_msgbox_get_active_btn_text(button->popup);
	
	if (strncmp(DANGER_BUTTON_CONFIRM_TEXT, button_text, strlen(DANGER_BUTTON_CONFIRM_TEXT) + 1) == 0)
	{
		if (button->action_cb)
			button->action_cb(button->cb_arg);
	}
	
	lv_msgbox_close(button->popup);
	button->popup = NULL;
	#endif
}

void kest_danger_button_activate_popup_cb(lv_event_t *e)
{
	//kest_printf("kest_danger_button_activate_popup_cb\n");
	kest_danger_button *button = lv_event_get_user_data(e);
	
	if (!button)
	{
		//kest_printf("button is NULL! returning...\n");
		return;
	}
	
	if (!button->parent)
	{
		//kest_printf("Button's parent is NULL! Returning\n");
		return;
	}
	
	button->popup = lv_msgbox_create(button->parent->screen);
	lv_msgbox_add_title(button->popup, button->button.label_text);
	lv_msgbox_add_text(button->popup, "\n\tAre you sure?");
	
	lv_obj_t *btn;
	btn = lv_msgbox_add_footer_button(button->popup, DANGER_BUTTON_CONFIRM_TEXT);
	lv_obj_add_event_cb(btn, kest_danger_button_confirm_cb, LV_EVENT_CLICKED, button);
	lv_obj_set_size(btn, DANGER_BUTTON_POPUP_BUTTON_WIDTH, DANGER_BUTTON_POPUP_BUTTON_HEIGHT);
	lv_obj_align_to(btn, button->popup, LV_ALIGN_BOTTOM_LEFT, 0, -GLOBAL_PAD_WIDTH);
	btn = lv_msgbox_add_footer_button(button->popup, DANGER_BUTTON_CANCEL_TEXT);
	lv_obj_add_event_cb(btn, kest_danger_button_cancel_cb, LV_EVENT_CLICKED, button);
	lv_obj_set_size(btn, DANGER_BUTTON_POPUP_BUTTON_WIDTH, DANGER_BUTTON_POPUP_BUTTON_HEIGHT);
	lv_obj_align_to(btn, button->popup, LV_ALIGN_CENTER, 0, -GLOBAL_PAD_WIDTH);
	
	lv_obj_set_size(button->popup, DANGER_BUTTON_POPUP_WIDTH, DANGER_BUTTON_POPUP_HEIGHT);
	
	//kest_printf("kest_danger_button_activate_popup_cb done\n");
}

void kest_active_button_scale_cb(void *data, int32_t value)
{
	if (!data)
		return;
	
	kest_active_button *button = (kest_active_button*)data;
	
	int32_t new_height = ((float)value / 1000.0) * button->button.height;
	int32_t new_width  = ((float)value / 1000.0) * button->button.width;
	
	//kest_printf("kest_active_button_scale_cb. new_height = %d. new_width = %d\n", new_height, new_width);
	
	lv_obj_set_height(button->button.obj, new_height);
	lv_obj_set_width (button->button.obj, new_width);
	
	button->pos_y = button->pos_y - (new_height - lv_obj_get_height(button->button.obj)) / 2;
	button->button.align_offs_y = button->pos_y;
	
	lv_obj_align(button->button.obj, LV_ALIGN_TOP_MID, 0, button->pos_y);
}

void kest_active_button_glide_cb(void *data, int32_t value)
{
	if (!data)
		return;
	
	kest_active_button *button = (kest_active_button*)data;
	
	lv_obj_set_y(button->button.obj, value);
	button->pos_y = value;
}

void kest_active_button_del_button_fade_cb(void *data, int32_t value)
{
	//kest_printf("kest_active_button_del_button_fade_cb. data = %p\n", data);
	if (!data)
		return;
	
	kest_active_button *button = (kest_active_button*)data;
	
	if (!button->del_button)
		return;
	
	//kest_printf("button->del_button->obj = %p\n", button->del_button->obj);
	
	kest_button_set_opacity(button->del_button, value);
	//kest_printf("kest_active_button_del_button_fade_cb done\n");
}

void kest_active_button_del_button_faded_out_cb(lv_anim_t *anim)
{
	if (!anim)
		return;
	
	kest_active_button *button = (kest_active_button*)anim->user_data;
	
	if (!button)
		return;
	
	kest_button_hide(button->del_button);
}

void kest_active_button_delete_anim_cb(void *data, int32_t value)
{
	if (!data)
		return;
	
	kest_active_button *button = (kest_active_button*)data;
	
	kest_button_set_opacity(&button->button, 255 * ((float)value / 100.0));
}

void kest_active_button_trigger_scale_anim(kest_active_button *button, int direction)
{
	#ifdef ACTIVE_BUTTON_USE_SCALE_ANIM
	if (!button)
		return;
	
	int initial = direction ? 1000 : 1000 * KEST_BUTTON_LP_SCALE;
	int final   = direction ? 1000 * KEST_BUTTON_LP_SCALE : 1000;

	lv_anim_init			(&button->scale_anim);
	lv_anim_set_var			(&button->scale_anim, button);
	lv_anim_set_exec_cb		(&button->scale_anim, kest_active_button_scale_cb);
	lv_anim_set_time		(&button->scale_anim, KEST_BUTTON_SCALE_ANIM_MS);
	lv_anim_set_values		(&button->scale_anim, initial, final);
	lv_anim_path_ease_in_out(&button->scale_anim);
	
	lv_anim_start(&button->scale_anim);
	#endif
}

void kest_active_button_trigger_glide_anim(kest_active_button *button, int32_t new_pos_y)
{
	if (!button)
		return;
	
	lv_anim_init			(&button->glide_anim);
	lv_anim_set_var			(&button->glide_anim, button);
	lv_anim_set_exec_cb		(&button->glide_anim, kest_active_button_glide_cb);
	lv_anim_set_time		(&button->glide_anim, KEST_BUTTON_GLIDE_ANIM_MS);
	lv_anim_set_values		(&button->glide_anim, button->pos_y, new_pos_y);
	lv_anim_path_ease_in_out(&button->glide_anim);
	
	lv_anim_start(&button->glide_anim);
}

void kest_active_button_trigger_del_button_fade_in(kest_active_button *button)
{
	if (!button)
		return;
	
	if (!button->del_button)
	{
		//kest_printf("no del button...\n");
		return;
	}
	
	lv_anim_init			(&button->del_button_fade);
	lv_anim_set_var			(&button->del_button_fade, button);
	lv_anim_set_exec_cb		(&button->del_button_fade, kest_active_button_del_button_fade_cb);
	lv_anim_set_time		(&button->del_button_fade, KEST_BUTTON_DEL_BTN_FADE_IN_MS);
	lv_anim_set_values		(&button->del_button_fade, 0, 255);
	
	button->del_button_fade.user_data = (void*)button;
	
	kest_button_unhide(button->del_button);
	lv_anim_start(&button->del_button_fade);
}

void kest_active_button_trigger_del_button_fade_out(kest_active_button *button)
{
	lv_anim_init			(&button->del_button_fade);
	lv_anim_set_var			(&button->del_button_fade, button);
	lv_anim_set_exec_cb		(&button->del_button_fade, kest_active_button_del_button_fade_cb);
	lv_anim_set_ready_cb	(&button->del_button_fade, kest_active_button_del_button_faded_out_cb);
	lv_anim_set_time		(&button->del_button_fade, KEST_BUTTON_DEL_BTN_FADE_OUT_MS);
	lv_anim_set_values		(&button->del_button_fade, 255, 0);
	
	button->del_button_fade.user_data = (void*)button;
	
	kest_button_unhide(button->del_button);
	lv_anim_start(&button->del_button_fade);
}

void kest_active_button_delete_anim_ready_cb(lv_anim_t *anim)
{
	kest_active_button *button = (kest_active_button*)anim->user_data;
	
	kest_active_button_array_remove(button->array, button->index);
	kest_active_button_free(button);
}

void kest_active_button_trigger_delete_anim(kest_active_button *button)
{
	lv_anim_init			(&button->delete_anim);
	lv_anim_set_var			(&button->delete_anim, button);
	lv_anim_set_exec_cb		(&button->delete_anim, kest_active_button_delete_anim_cb);
	lv_anim_set_ready_cb	(&button->delete_anim, kest_active_button_delete_anim_ready_cb);
	lv_anim_set_time		(&button->delete_anim, KEST_BUTTON_DEL_ANIM_MS);
	lv_anim_set_values		(&button->delete_anim, 100, 0);
	
	button->delete_anim.user_data = (void*)button;
	
	lv_anim_start(&button->delete_anim);
}

int kest_active_button_force_index(kest_active_button *button, int i)
{
	if (!button)
		return ERR_NULL_PTR;
	
	if (!button->array)
		return ERR_BAD_ARGS;
	
	lv_coord_t new_pos_y = kest_active_button_array_index_y_position(button->array, i);
	
	kest_active_button_trigger_glide_anim(button, new_pos_y);
	
	button->index = i;
	
	return NO_ERROR;
}

int kest_active_button_set_index(kest_active_button *button, int i)
{
	if (!button)
		return ERR_NULL_PTR;
	
	if (button->index == i)
		return NO_ERROR;
	
	if (!button->array)
		return ERR_BAD_ARGS;
	
	lv_coord_t new_pos_y = kest_active_button_array_index_y_position(button->array, i);
	
	kest_active_button_trigger_glide_anim(button, new_pos_y);
	
	button->index = i;
	
	return NO_ERROR;
}

void kest_active_button_del_button_remain_timer_cb(lv_timer_t *timer)
{
	kest_active_button *button = lv_timer_get_user_data(timer);
	
	kest_active_button_trigger_del_button_fade_out(button);
	button->del_button_remain_timer = NULL;
}

void kest_active_button_clicked_cb(lv_event_t *e)
{
	KEST_PRINTF("kest_active_button_clicked_cb\n");
	kest_active_button *button = (kest_active_button*)lv_event_get_user_data(e);
	
	KEST_PRINTF("button->long_pressed = %d\n", button->long_pressed);
	
	if (!button)
	{
		KEST_PRINTF("Effect widget long press callback triggered but pointer to struct not passed");
		return;
	}
	
	if (button->array && button->array->clicked_cb && !button->long_pressed)
	{
		button->array->clicked_cb(button);
	}
	else
	{
		button->long_pressed = 0;
	}
}

void kest_active_button_long_pressed_cb(lv_event_t *e)
{
	//kest_printf("kest_active_button_long_pressed_cb\n");
	kest_active_button *button = (kest_active_button*)lv_event_get_user_data(e);
	
	if (!button)
	{
		KEST_PRINTF("Effect widget long press callback triggered but pointer to struct not passed");
		return;
	}
	
	button->long_pressed = 1;
	
	
	//kest_printf("long press detected... button->index = %d, button->prev_index = %d\n", button->index, button->prev_index);
	
	if (button->array)
	{
		if (button->array->flags & KEST_ACTIVE_BUTTON_ARRAY_FLAG_MOVEABLE)
		{
			if (button->array->container)
			{
				button->array->container_scrollable = lv_obj_has_flag(button->array->container, LV_OBJ_FLAG_SCROLLABLE);
				
				if (button->array->container_scrollable)
					lv_obj_remove_flag(button->array->container, LV_OBJ_FLAG_SCROLLABLE);
			}
			
			lv_point_t touch_point;
			lv_indev_t *indev = lv_indev_get_act();
			lv_indev_get_point(indev, &touch_point);
			
			lv_point_t local_point = touch_point;
			lv_obj_t *parent = lv_obj_get_parent(button->button.obj);

			lv_area_t parent_coords;
			lv_obj_get_coords(parent, &parent_coords);
			
			local_point.y -= parent_coords.y1;
			
			button->relative_touch_y = local_point.y - button->pos_y;
			
			button->prev_index = button->index;
			
			kest_active_button_trigger_scale_anim(button, KEST_BUTTON_SCALE_EXPAND);
		
			lv_obj_move_foreground(button->button.obj);
		}
		
		if (button->array->flags & KEST_ACTIVE_BUTTON_ARRAY_FLAG_DELETEABLE && button->del_button_anims)
		{
			kest_active_button_trigger_del_button_fade_in(button);
		}
	}
	
	
	//kest_printf("kest_active_button_long_pressed_cb done\n");
}

void kest_active_button_pressing_cb(lv_event_t *e)
{
	kest_active_button *button = (kest_active_button*)lv_event_get_user_data(e);
	
	if (!button)
	{
		KEST_PRINTF("Effect widget long press callback triggered but pointer to struct not passed");
		return;
	}

	if (!button->long_pressed || !button->array || !(button->array->flags & KEST_ACTIVE_BUTTON_ARRAY_FLAG_MOVEABLE))
		return;

	lv_point_t touch_point;
	lv_indev_t *indev = lv_indev_get_act();
	lv_indev_get_point(indev, &touch_point);
	
	lv_point_t local_point = touch_point;
	lv_obj_t *parent = lv_obj_get_parent(button->button.obj);

	lv_area_t parent_coords;
	lv_obj_get_coords(parent, &parent_coords);
	
	local_point.y -= parent_coords.y1;

	button->pos_y = local_point.y - button->relative_touch_y;
	button->button.align_offs_y = button->pos_y;
	
	lv_obj_set_y(button->button.obj, button->pos_y);
	
	int new_index = 0;
	for (int i = 0; i < button->array->n_buttons; i++)
	{
		if (button->array->buttons[i] == button)
			continue;
		
		if (button->array->buttons[i]->pos_y < button->pos_y)
			new_index++;
		else 
			break;
	}
	
	if (new_index == button->index)
		return;
	
	if (new_index < button->index)
	{
		for (int i = button->index; i > new_index; i--)
		{
			button->array->buttons[i] = button->array->buttons[i - 1];
			kest_active_button_set_index(button->array->buttons[i], i);
		}
		
		button->array->buttons[new_index] = button;
		kest_active_button_set_index(button, new_index);
	}
	else if (new_index > button->index)
	{
		for (int i = button->index; i < new_index; i++)
		{
			button->array->buttons[i] = button->array->buttons[i + 1];
			kest_active_button_set_index(button->array->buttons[i], i);
		}
		
		button->array->buttons[new_index] = button;
		kest_active_button_set_index(button, new_index);
	}
	
	//kest_printf("new index: %d. The current array:\n", new_index);
	
	for (int i = 0; i < button->array->n_buttons; i++)
	{
		//kest_printf("\t buttons[%d] = %p%s\n", i, button->array->buttons[i], (button->array->buttons[i] == button) ? " (this)" : "");
	}
}


void kest_active_button_release_cb(lv_event_t *e)
{
	KEST_PRINTF("kest_active_button_release_cb\n");
	kest_active_button *button = (kest_active_button*)lv_event_get_user_data(e);
	
	if (!button)
	{
		return;
	}
	
	if (!button->array)
	{
		KEST_PRINTF("button has no array :(\n");
		return;
	}
	
	if (button->long_pressed)
	{
		if (button->array->flags & KEST_ACTIVE_BUTTON_ARRAY_FLAG_MOVEABLE)
		{
			if (button->array->container && button->array->container_scrollable)
				lv_obj_add_flag(button->array->container, LV_OBJ_FLAG_SCROLLABLE);
			
			kest_active_button_trigger_scale_anim(button, KEST_BUTTON_SCALE_CONTRACT);
			
			kest_active_button_force_index(button, button->index);
			
			if (button->index != button->prev_index)
			{
				if (button->array && button->array->moved_cb)
				{
					button->array->moved_cb(button);
				}
			}
		}
		
		if (button->array->flags & KEST_ACTIVE_BUTTON_ARRAY_FLAG_DELETEABLE && button->del_button_anims)
		{
			button->del_button_remain_timer = lv_timer_create(kest_active_button_del_button_remain_timer_cb, KEST_BUTTON_DEL_BTN_REMAIN_MS, button);
			lv_timer_set_repeat_count(button->del_button_remain_timer, 1);
			
			lv_timer_resume(button->del_button_remain_timer);
		}
	}
}

void kest_active_button_del_cb(lv_event_t *e)
{
	KEST_PRINTF("kest_active_button_del_cb\n");
	kest_active_button *button = (kest_active_button*)lv_event_get_user_data(e);
	
	kest_active_button_trigger_delete_anim(button);
	
	lv_obj_clear_flag(button->button.obj, LV_OBJ_FLAG_CLICKABLE);
	
	if (button->array && button->array->del_button_cb)
	{
		button->array->del_button_cb(button);
	}
}

int kest_active_button_init(kest_active_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	init_button(&button->button);
	
	button_set_long_pressed_cb	(&button->button, kest_active_button_long_pressed_cb, button);
	button_set_pressing_cb		(&button->button, kest_active_button_pressing_cb, button);
	button_set_released_cb		(&button->button, kest_active_button_release_cb, button);
	button_set_clicked_cb		(&button->button, kest_active_button_clicked_cb, button);
	
	button->del_button 	= NULL;
	
	button->del_button_remain_timer = NULL;
	
	button->index  	   = 0;
	button->prev_index = 0;
	
	button->long_pressed = 0;
	
	button->del_button_anims = 1;
	
	return NO_ERROR;
}

int kest_active_button_add_del_button(kest_active_button *button)
{
	//kest_printf("kest_active_button_add_del_button, button = %p\n", button);
	if (!button)
		return ERR_NULL_PTR;
	
	if (button->del_button)
	{
		KEST_PRINTF("WARNING: kest_active_button_add_del_button called with button for which button->del_button is not NULL: it is %p", button->del_button);
		return ERR_BAD_ARGS;
	}
	
	button->del_button = new_button(LV_SYMBOL_TRASH);
	
	if (!button->del_button)
		return ERR_ALLOC_FAIL;
	
	init_button(button->del_button);
	kest_button_set_label(button->del_button, LV_SYMBOL_TRASH);
	kest_button_hide(button->del_button);
	
	kest_button_add_sub_button(&button->button, button->del_button);
	
	kest_button_set_size(button->del_button, button->button.height, button->button.height);
	kest_button_set_alignment(button->del_button, LV_ALIGN_RIGHT_MID, 0, 0);
	
	button_set_clicked_cb(button->del_button, kest_active_button_del_cb, button);
	
	return NO_ERROR;
}

void kest_active_button_set_representation(kest_active_button *button, void *representer, void *representee, void (*update)(void*, void*))
{
	if (button)
	{
		button->rep.representer = representer;
		button->rep.representee = representee;
		button->rep.update		= update;
	}
}

int kest_active_button_create_ui(kest_active_button *button, lv_obj_t *parent)
{
	if (!button)
		return ERR_NULL_PTR;
	
	create_button_ui(&button->button, parent);
	
	button->base_height = lv_obj_get_height(button->button.obj);
	button->base_width  = lv_obj_get_width (button->button.obj);
	
	/*
	button->button.obj = lv_btn_create(parent);
	
    lv_obj_set_size(button->button.obj, KEST_BUTTON_WIDTH, KEST_BUTTON_HEIGHT);
    
	
	
	lv_obj_align(button->button.obj, LV_ALIGN_TOP_MID, 0, button->pos_y);
	
	button->button.label = lv_label_create(button->button.obj);
	lv_label_set_text(button->button.label, button->button.label_text);
	lv_obj_center(button->button.label);
	
	lv_obj_add_event_cb(button->button.obj, kest_active_button_release_cb, LV_EVENT_RELEASED, button);
	lv_obj_add_event_cb(button->button.obj, kest_active_button_long_pressed_cb, LV_EVENT_LONG_PRESSED, button);
	lv_obj_add_event_cb(button->button.obj, kest_active_button_pressing_cb, LV_EVENT_PRESSING, button);
	
	button->del_button = lv_btn_create(button->button.obj);
	lv_obj_align(button->del_button, LV_ALIGN_RIGHT_MID, 10, 0);
	lv_obj_set_size(button->del_button, 0.75 * KEST_BUTTON_HEIGHT, 0.75 * KEST_BUTTON_HEIGHT);
	lv_obj_add_event_cb(button->del_button, kest_active_button_del_cb, LV_EVENT_CLICKED, button);
	lv_obj_add_flag(button->del_button, LV_OBJ_FLAG_HIDDEN);
	
	button->del_button->label = lv_label_create(button->del_button);
	lv_label_set_text(button->del_button->label, LV_SYMBOL_TRASH);
	lv_obj_center(button->del_button->label);
	*/
	
	return NO_ERROR;
}

void kest_active_button_free(kest_active_button *button)
{
	if (!button)
		return;
	
	if (button->button.obj)
	{
		lv_anim_del(button, NULL);
		
		if (button->del_button_remain_timer)
		{
			lv_timer_del(button->del_button_remain_timer);
			button->del_button_remain_timer = NULL;
		}
		
		if (button->array && button->array->delete_cb)
			button->array->delete_cb(button);
	}
	
	kest_button_delete_ui(&button->button);
	
	kest_free(button);
}

int kest_active_button_change_label(kest_active_button *button, char *text)
{
	if (!button)
		return ERR_NULL_PTR;
	
	kest_button_set_label(&button->button, text);
	
	return NO_ERROR;
}

int kest_active_button_set_dimensions(kest_active_button *button, int w, int h)
{
	if (!button)
		return ERR_NULL_PTR;
	
	button->base_width  = w;
	button->base_height = h;
	
	kest_button_set_size(&button->button, w, h);
	
	return NO_ERROR;
}

int kest_active_button_swap_del_button_for_persistent_unclickable(kest_active_button *button, const char *label)
{
	if (!button)
		return ERR_NULL_PTR;
	
	if (!button->del_button)
		return ERR_BAD_ARGS;
	
	if (!button->del_button->obj)
		return ERR_BAD_ARGS;
	
	if (!button->del_button->label)
	{
		kest_button_create_label_ui(button->del_button);
	}
	
	lv_anim_del(button, kest_active_button_del_button_fade_cb);
	
	kest_button_reset_state(button->del_button);
	kest_button_set_label(button->del_button, label);
	kest_button_set_unclickable(button->del_button);
	kest_button_set_opacity(button->del_button, 255);
	kest_button_unhide(button->del_button);
	
	button->del_button_anims = 0;
	
	return NO_ERROR;
}


int kest_active_button_set_playing(kest_active_button *button)
{
	if (!button)
		return ERR_NULL_PTR;
	
	if (!button->del_button)
		return ERR_BAD_ARGS;
	
	if (!button->del_button->obj)
		return ERR_BAD_ARGS;
	
	if (!button->del_button->label)
	{
		kest_button_create_label_ui(button->del_button);
	}
	
	lv_anim_del(button, kest_active_button_del_button_fade_cb);
	
	kest_button_reset_state(button->del_button);
	kest_button_set_label(button->del_button, LV_SYMBOL_PLAY);
	kest_button_set_unclickable(button->del_button);
	kest_button_set_opacity(button->del_button, 255);
	kest_button_unhide(button->del_button);
	
	button->del_button_anims = 0;
	
	return NO_ERROR;
}

void kest_active_button_set_playing_async_wrapper(void *button_)
{
	kest_active_button_set_playing((kest_active_button*)button_);
}

int kest_active_button_set_playing_async(kest_active_button *button)
{
	kest_ui_async_call(kest_active_button_set_playing_async_wrapper, (void*)button);
	return NO_ERROR;
}

void kest_active_button_reset_del_button_async_wrapper(void *button_)
{
	kest_active_button_reset_del_button((kest_active_button*)button_);
}

int kest_active_button_reset_del_button(kest_active_button *button)
{
	KEST_PRINTF("kest_active_button_reset_del_button. button = %p\n", button);
	if (!button)
		return ERR_NULL_PTR;
	
	if (!button->del_button)
	{
		//kest_printf("del button doesn't exist. bailing\n");
		return ERR_BAD_ARGS;
	}
	
	if (!button->del_button->obj)
	{
		//kest_printf("del button lv_obj not created. not my job. bailing\n");
		return ERR_BAD_ARGS;
	}
	
	//kest_printf("make it hidden...\n");
	kest_button_hide(button->del_button);
	
	if (!button->del_button->label)
	{
		//kest_printf("del button label doesn't exist. creating...\n");
		kest_button_create_label_ui(button->del_button);
	}
	
	//kest_printf("delete buttons's animations...\n");
	lv_anim_del(button, kest_active_button_del_button_fade_cb);
	
	//kest_printf("reset button's state...\n");
	kest_button_reset_state(button->del_button);
	//kest_printf("set label to %s\n", LV_SYMBOL_TRASH);
	kest_button_set_label(button->del_button, LV_SYMBOL_TRASH);
	//kest_printf("make it clickable\n");
	kest_button_set_clickable(button->del_button);
	
	//kest_printf("activate animations...\n");
	button->del_button_anims = 1;
	
	//kest_printf("kest_active_button_reset_del_button done\n");
	return NO_ERROR;
}



int kest_active_button_reset_del_button_async(kest_active_button *button)
{
	kest_ui_async_call(kest_active_button_reset_del_button_async_wrapper, (void*)button);
	return NO_ERROR;
}

int kest_active_button_array_init(kest_active_button_array *array)
{
	if (!array)
		return ERR_NULL_PTR;
	
	array->flags = 0;
	
	array->n_buttons = 0;
	array->length	 = 0;
	array->buttons = NULL;
	
	array->parent = NULL;
	array->container = NULL;
	
	array->delete_cb 		= NULL;
	array->del_button_cb 	= NULL;
	array->clicked_cb 		= NULL;
	array->moved_cb 		= NULL;
	
	array->data = NULL;
	
	array->base_y_pos = KEST_BUTTON_ARRAY_BASE_Y;
	
	array->button_width  = KEST_BUTTON_WIDTH;
	array->button_height = KEST_BUTTON_HEIGHT;
	
	return NO_ERROR;
}

int kest_active_button_array_set_length(kest_active_button_array *array, int n)
{
	if (!array)
		return ERR_NULL_PTR;
	
	array->buttons = kest_alloc(sizeof(kest_active_button*) * n);
	
	if (!array->buttons)
	{
		return ERR_ALLOC_FAIL;
	}
	
	for (int i = 0; i < n; i++)
		array->buttons[i] = NULL;
	
	array->length = n;
	
	return NO_ERROR;
}

kest_active_button_array *kest_active_button_array_new()
{
	kest_active_button_array *array = kest_alloc(sizeof(kest_active_button_array));
	
	if (!array)
		return NULL;
	
	kest_active_button_array_init(array);
	
	return array;
}

kest_active_button *kest_active_button_array_append_new(kest_active_button_array *array, void *data, char *label)
{
	KEST_PRINTF("kest_active_button_array_append_new\n");
	if (!array)
	{
		//kest_printf("Returning NULL cause array = %p\n", array);
		return NULL;
	}
	
	kest_active_button *button = kest_alloc(sizeof(kest_active_button));
	
	if (!button)
	{
		//kest_printf("returning NULL cause button = %p\n", button);
		return NULL;
	}
	
	kest_active_button_init(button);
	
	button->data = data;
	button->button.label_text = label;
	
	int ret_val;
	
	if ((ret_val = kest_active_button_array_append(button, array)) != NO_ERROR)
	{
		kest_free(button);
		//kest_printf("Failed to add button to array: %s\n", kest_error_code_to_string(ret_val));
		return NULL;
	}
	
	if (array->flags & KEST_ACTIVE_BUTTON_ARRAY_FLAG_DELETEABLE)
	{
		kest_active_button_add_del_button(button);
	}
	
	//kest_printf("returning %p\n", button);
	KEST_PRINTF("kest_active_button_array_append_new done\n");
	return button;
}

int kest_active_button_array_append(kest_active_button *button, kest_active_button_array *array)
{
	if (!button || !array)
		return ERR_NULL_PTR;
	
	if (array->n_buttons >= array->length)
	{
		return ERR_BAD_ARGS;
	}
	
	button->index = array->n_buttons;
	button->prev_index = array->n_buttons;
	button->array = array;
	
	button->pos_y = kest_active_button_array_index_y_position(array, button->index);
	kest_button_set_alignment(&button->button, LV_ALIGN_TOP_MID, 0, button->pos_y);
	
	kest_button_set_size(&button->button, array->button_width, array->button_height);
	
	array->buttons[array->n_buttons++] = button;
	
	return NO_ERROR;
}


int kest_active_button_array_remove(kest_active_button_array *array, int index)
{
	if (!array)
		return ERR_NULL_PTR;
	
	if (index < 0 || index > array->n_buttons - 1)
		return ERR_BAD_ARGS;
	
	array->n_buttons--;
	
	array->buttons[index] = NULL;
	
	for (int i = index; i < array->n_buttons; i++)
	{
		array->buttons[i] = array->buttons[i + 1];
		kest_active_button_set_index(array->buttons[i], i);
	}
	
	return NO_ERROR;
}

int kest_active_button_array_index_y_position(kest_active_button_array *array, int index)
{
	return array->base_y_pos + index * KEST_BUTTON_DISTANCE;
}

int kest_active_button_array_create_ui(kest_active_button_array *array, lv_obj_t *parent)
{
	if (!array || !parent)
		return ERR_NULL_PTR;
	
	array->container = parent;
	array->container_scrollable = lv_obj_has_flag(parent, LV_OBJ_FLAG_SCROLLABLE);
	
	for (int i = 0; i < array->n_buttons; i++)
	{
		if (array->buttons[i]) kest_active_button_create_ui(array->buttons[i], parent);
	}
	
	return NO_ERROR;
}

int kest_active_button_array_set_dimensions(kest_active_button_array *array, int w, int h)
{
	if (!array)
		return ERR_NULL_PTR;
	
	array->button_width  = w;
	array->button_height = h;
	
	for (int i = 0; i < array->n_buttons; i++)
	{
		kest_active_button_set_dimensions(array->buttons[i], w, h);
	}
	
	return NO_ERROR;
}


kest_active_button *kest_active_button_array_find_with_data(kest_active_button_array *array, void *data)
{
	if (array)
		return NULL;
	
	
	for (int i = 0; i < array->n_buttons; i++)
	{
		if (array->buttons[i] && array->buttons[i]->data == data)
			return array->buttons[i];
	}
	
	return NULL;
}

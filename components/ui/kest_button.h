#ifndef KEST_INT_M_BUTTON_H_
#define KEST_INT_M_BUTTON_H_

#include "kest_representation.h"

#define STANDARD_DEL_BTN_REMAIN_MS 	1000

#define STANDARD_BUTTON_HEIGHT       ((int)(0.07 * DISPLAY_VRES))
#define STANDARD_BUTTON_SHORT_HEIGHT ((int)(0.06 * DISPLAY_VRES))

#define KEST_BUTTON_HEIGHT STANDARD_BUTTON_HEIGHT
#define KEST_BUTTON_WIDTH  (STANDARD_CONTAINER_WIDTH - (3 * GLOBAL_PAD_WIDTH))

#define KEST_BUTTON_SCALE_EXPAND 	1
#define KEST_BUTTON_SCALE_CONTRACT 0

#define KEST_BUTTON_LP_SCALE 		1.05
#define KEST_BUTTON_SCALE_ANIM_MS 	75
#define KEST_BUTTON_GLIDE_ANIM_MS 	150

#define KEST_BUTTON_DEL_BTN_FADE_IN_MS 	75
#define KEST_BUTTON_DEL_BTN_FADE_OUT_MS 	300
#define KEST_BUTTON_DEL_BTN_REMAIN_MS 	STANDARD_DEL_BTN_REMAIN_MS

#define KEST_BUTTON_DEL_ANIM_MS 			100

#define KEST_BUTTON_ARRAY_BASE_Y 		((int)(0.021f * DISPLAY_VRES))

#define KEST_BUTTON_V_PAD	((int)(0.028f * DISPLAY_VRES))
#define KEST_BUTTON_DISTANCE (KEST_BUTTON_HEIGHT + KEST_BUTTON_V_PAD)


#define KEST_BUTTON_MAX_SUB_BUTTONS 2

#define KEST_BUTTON_DISABLED_OPACITY 128

#define KEST_BUTTON_FLAG_HIDDEN 		0b0001
#define KEST_BUTTON_FLAG_DISABLED	 	0b0010
#define KEST_BUTTON_FLAG_UNCLICKABLE 	0b0100
#define KEST_BUTTON_FLAG_NO_ALIGN	 	0b1000

struct kest_ui_page;

typedef struct kest_button
{
	int flags;
	
	lv_obj_t *obj;
	lv_obj_t *label;
	char *label_text;
	
	lv_event_cb_t clicked_cb;
	void *clicked_cb_arg;
	
	lv_event_cb_t pressing_cb;
	void *pressing_cb_arg;
	
	lv_event_cb_t long_pressed_cb;
	void *long_pressed_cb_arg;
	
	lv_event_cb_t released_cb;
	void *released_cb_arg;
	
	int long_pressed;
	int clickable;
	
	int *hider;
	
	int draggable_x;
	int draggable_y;
	
	int width;
	int height;
	
	int opacity;
	int label_opacity;
	
	lv_align_t alignment;
	int align_offs_x;
	int align_offs_y;
	
	int n_sub_buttons;
	struct kest_button *sub_buttons[KEST_BUTTON_MAX_SUB_BUTTONS];
} kest_button;

int init_button(kest_button *button);
kest_button *new_button(const char *label);

int kest_button_create_label_ui(kest_button *button);
int create_button_ui(kest_button *button, lv_obj_t *parent);

int button_set_clicked_cb	  (kest_button *button, lv_event_cb_t cb, void *cb_arg);
int button_set_pressing_cb	  (kest_button *button, lv_event_cb_t cb, void *cb_arg);
int button_set_long_pressed_cb(kest_button *button, lv_event_cb_t cb, void *cb_arg);
int button_set_released_cb	  (kest_button *button, lv_event_cb_t cb, void *cb_arg);

int kest_button_set_label(kest_button *button, const char *label);
int kest_button_disable_alignment(kest_button *button);
int kest_button_set_alignment(kest_button *button, lv_align_t align, int offs_x, int offs_y);
int kest_button_set_size(kest_button *button, int width, int height);

int kest_button_add_sub_button(kest_button *button, kest_button *sub_button);

int kest_button_hide(kest_button *button);
int kest_button_unhide(kest_button *button);
int kest_button_set_clickable(kest_button *button);
int kest_button_set_unclickable(kest_button *button);

int kest_button_set_opacity(kest_button *button, int opacity);

int kest_button_enable(kest_button *button);
int kest_button_disable(kest_button *button);

int kest_button_enable_async(kest_button *button);
int kest_button_disable_async(kest_button *button);

int kest_button_delete_ui(kest_button *button);

#define DANGER_BUTTON_CONFIRM_TEXT "Yes"
#define DANGER_BUTTON_CANCEL_TEXT  "Cancel"

#define DANGER_BUTTON_POPUP_HEIGHT ((int)(0.25 * DISPLAY_VRES))
#define DANGER_BUTTON_POPUP_WIDTH  ((int)(0.6 * DISPLAY_HRES))

#define DANGER_BUTTON_POPUP_BUTTON_HEIGHT ((int)(0.045 * DISPLAY_VRES))
#define DANGER_BUTTON_POPUP_BUTTON_WIDTH  ((int)(0.2   * DISPLAY_HRES))

typedef struct 
{
	kest_button button;
	struct kest_ui_page *parent;
	lv_obj_t *popup;
	void (*action_cb)(void *data);
	void *cb_arg;
} kest_danger_button;

int init_danger_button(kest_danger_button *button, void (*action_cb)(void *data), void *cb_arg, struct kest_ui_page *parent);
int kest_danger_button_create_ui(kest_danger_button *button, lv_obj_t *parent);
void kest_danger_button_activate_popup_cb(lv_event_t *e);
void kest_danger_button_value_changed_cb(lv_event_t *e);

struct kest_active_button_array;

typedef struct kest_active_button
{
	void *data;
	
	kest_button button;
	kest_button *del_button;
	
	lv_anim_t scale_anim;
	lv_anim_t glide_anim;
	lv_anim_t del_button_fade;
	lv_anim_t delete_anim;
	
	lv_timer_t *del_button_remain_timer;
	
	lv_coord_t pos_y;
	
	int index;
	int prev_index;
	
	int del_button_anims;
	
	uint32_t base_width;
	uint32_t base_height;
	
	int relative_touch_y;
	
	int long_pressed;
	
	kest_representation rep;
	
	struct kest_active_button_array *array;
} kest_active_button;

DECLARE_LINKED_PTR_LIST(kest_active_button);

int kest_active_button_add_del_button(kest_active_button *button);
int kest_active_button_change_label(kest_active_button *button, char *text);
int kest_active_button_swap_del_button_for_persistent_unclickable(kest_active_button *button, const char *label);
int kest_active_button_reset_del_button(kest_active_button *button);


int kest_active_button_reset_del_button_async(kest_active_button *button);
int kest_active_button_set_playing_async(kest_active_button *button);

int kest_active_button_set_dimensions(kest_active_button *button, int w, int h);

int kest_active_button_create_ui(kest_active_button *button, lv_obj_t *parent);

void kest_active_button_free(kest_active_button *button);

void kest_active_button_set_representation(kest_active_button *button,
	void *representer, void *representee, void (*update)(void*, void*));

#define KEST_ACTIVE_BUTTON_ARRAY_FLAG_DELETEABLE 0b0001
#define KEST_ACTIVE_BUTTON_ARRAY_FLAG_MOVEABLE   0b0010

typedef struct kest_active_button_array
{
	int flags;
	
	int n_buttons;
	int length;
	
	int button_width;
	int button_height;
	
	kest_active_button **buttons;
	
	lv_obj_t *container;
	int container_scrollable;
	struct kest_ui_page *parent;
	
	int base_y_pos;
	
	void *data;
	
	int (*delete_cb)(struct kest_active_button *button);
	int (*del_button_cb)(struct kest_active_button *button);
	int (*clicked_cb)(struct kest_active_button *button);
	int (*moved_cb)(struct kest_active_button *button);
} kest_active_button_array;

int kest_active_button_array_index_y_position(kest_active_button_array *array, int index);

kest_active_button_array *kest_active_button_array_new();

int kest_active_button_array_append(kest_active_button *button, kest_active_button_array *array);
kest_active_button *kest_active_button_array_append_new(kest_active_button_array *array, void *data, char *label);
int kest_active_button_array_remove(kest_active_button_array *array, int index);

int kest_active_button_array_set_length(kest_active_button_array *array, int n);
int kest_active_button_array_set_dimensions(kest_active_button_array *array, int w, int h);

int kest_active_button_array_create_ui(kest_active_button_array *array, lv_obj_t *parent);

kest_active_button *kest_active_button_array_find_with_data(kest_active_button_array *array, void *data);

#endif

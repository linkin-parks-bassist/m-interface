#include "kest_int.h"
#ifdef KEST_PLATFORM_P4_NANO
#include "bsp/esp32_p4_nano.h"
#endif

#define PRINTLINES_ALLOWED 0

IMPLEMENT_LINKED_PTR_LIST(lv_obj_t);

static const char *FNAME = "kest_ui.c";

typedef struct
{
	kest_active_button_array *array;
} test_page_str;

kest_ui_page test_page;

lv_obj_t *keyboard;

int create_test_page_ui(kest_ui_page *page)
{
	ui_page_create_base_ui(&test_page);
	test_page_str *str = test_page.data_struct;
	
	kest_active_button_array_create_ui(str->array, test_page.container);
	
	return NO_ERROR;
}

void init_test_page()
{
	init_ui_page(&test_page);
	
	test_page.data_struct = kest_alloc(sizeof(test_page_str));
	
	test_page_str *str = test_page.data_struct;
	
	test_page.configured = 1;
	
	test_page.create_ui = &create_test_page_ui;
	
	test_page.panel = new_panel();
	test_page.panel->text = "Test page";
	
	ui_page_add_bottom_button(&test_page, "Test1", NULL);
	ui_page_add_bottom_button(&test_page, "Test2", NULL);
	
	str->array = kest_active_button_array_new();
	
	kest_active_button_array_set_length(str->array, 7);
	
	str->array->flags |= KEST_ACTIVE_BUTTON_ARRAY_FLAG_DELETEABLE;
	str->array->flags |= KEST_ACTIVE_BUTTON_ARRAY_FLAG_MOVEABLE;
	
	kest_active_button_array_append_new(str->array, NULL, "Test1");
	kest_active_button_array_append_new(str->array, NULL, "Test2");
	kest_active_button_array_append_new(str->array, NULL, "Test3");
	kest_active_button_array_append_new(str->array, NULL, "Test4");
	kest_active_button_array_append_new(str->array, NULL, "Test5");
}

int kest_ui_page_set_background_default(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (!page->screen)
		return ERR_BAD_ARGS;
	
	lv_obj_set_style_bg_color(page->screen, lv_color_hex(GLOBAL_BACKGROUND_COLOUR), 0);
	lv_obj_set_style_bg_opa(page->screen, LV_OPA_COVER, 0);
	
	return NO_ERROR;
}

int kest_init_global_pages(kest_global_pages *pages)
{
	if (!pages)
		return ERR_NULL_PTR;
	
	init_main_menu(&pages->main_menu);
	//init_effect_selector_eff(&pages->effect_selector);
	
	//init_preset_list(&pages->preset_list);
	init_sequence_list(&pages->sequence_list);
	pages->sequence_list.type = KEST_UI_PAGE_SEQ_LIST;
	
	init_sequence_view(&pages->main_sequence_view);
	pages->main_sequence_view.type = KEST_UI_PAGE_MSV;
	
	return NO_ERROR;
}

static lv_style_t bg_style;

static void screen_load_cb(lv_event_t *e)
{
    lv_obj_t *scr = lv_event_get_target(e);
    lv_obj_add_style(scr, &bg_style, LV_PART_MAIN);
}

void init_global_bg(lv_disp_t *disp)
{
    lv_style_init(&bg_style);
    lv_style_set_bg_color(&bg_style, lv_color_black());
    lv_style_set_bg_opa(&bg_style, LV_OPA_COVER);

    lv_obj_add_style(lv_scr_act(), &bg_style, LV_PART_MAIN);
    
    // Attach once: every time a new screen is loaded on this display
    lv_disp_get_scr_act(disp); // make sure disp is valid
    //lv_disp_set_bg_color(disp, lv_palette_darken(LV_PALETTE_GREY, 3)); // initial screen
    lv_obj_add_event_cb(lv_disp_get_scr_act(disp),
                        screen_load_cb,
                        LV_EVENT_SCREEN_LOAD_START,
                        NULL);
}

void kest_create_ui(lv_disp_t *disp)
{
	global_cxt.pages.backstage = lv_obj_create(NULL);
	
	/*init_global_bg(disp);
	lv_theme_default_init(disp,
                      lv_palette_main(LV_PALETTE_GREY),
                      lv_palette_main(LV_PALETTE_GREEN),
                      LV_THEME_DEFAULT_DARK,
                      GLOBAL_MAIN_FONT);
    
	lv_disp_set_theme(disp, lv_theme_default_get());*/
	
	//configure_preset_list(&global_cxt.pages.preset_list, &global_cxt.pages.main_menu);
	
	configure_sequence_list(&global_cxt.pages.sequence_list, &global_cxt.pages.main_menu);
	
	configure_ui_page(&global_cxt.pages.main_menu, NULL);
	
	configure_ui_page(&global_cxt.pages.main_sequence_view, &global_cxt.main_sequence);
	
	//init_test_page();
	//create_test_page_ui();
	//enter_ui_page(&test_page);
	
	if (global_cxt.pages.current_page)
		enter_ui_page(global_cxt.pages.current_page);
	else
		enter_ui_page(&global_cxt.pages.main_sequence_view);
}


void kest_create_ui_async_wrapper(void *arg)
{
	kest_create_ui(NULL);
}

void kest_create_ui_async()
{
	kest_ui_async_call(kest_create_ui_async_wrapper, NULL);
}

int init_ui_page(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	page->type = KEST_UI_PAGE_GENERIC;
	
	page->screen 			= NULL;
	page->panel				= NULL;
	
	page->configure			= NULL;
	page->create_ui 		= NULL;
	page->free_ui 			= NULL;
	page->free_all 			= NULL;
	page->enter_page 		= NULL;
	page->enter_page_from	= NULL;
	page->refresh			= NULL;
	
	page->data_struct 		= NULL;
	page->parent	 		= NULL;
	
	page->configured = 0;
	page->ui_created = 0;
	
	page->container_type = CONTAINER_TYPE_STD;
	page->container = NULL;
	
	for (int i = 0; i < MAX_BOTTOM_BUTTONS; i++)
		page->bottom_buttons[i] = NULL;
	
	return NO_ERROR;
}

kest_ui_page *create_ui_page()
{
	kest_ui_page *res = kest_alloc(sizeof(kest_ui_page));
	
	if (!res)
		return NULL;
	
	init_ui_page(res);
	
	return res;
}

int configure_ui_page(kest_ui_page *page, void *data)
{
	//kest_printf("configure_ui_page\n");
	if (!page)
		return ERR_NULL_PTR;
	
	if (page->configured)
	{
		//kest_printf("page already configured\n");
		return NO_ERROR;
	}
	
	if (page->configure)
	{
		//kest_printf("Function pointer exists\n");
		page->configure(page, data);
	}
	else
	{
		//kest_printf("No configure function pointer!\n");
		return ERR_BAD_ARGS;
	}
	
	page->configured = 1;
	
	//kest_printf("configure_ui_page done\n");
	return NO_ERROR;
}

int create_page_ui(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (page->ui_created)
		return NO_ERROR;
	
	if (page->create_ui)
		page->create_ui(page);
	else
		return ERR_BAD_ARGS;
	
	page->ui_created = 1;
	
	return NO_ERROR;
}

int enter_ui_page(kest_ui_page *page)
{
	if (!page)
	{
		KEST_PRINTF("Error! No page!\n");
		return ERR_NULL_PTR;
	}
	
	if (!page->ui_created)
	{
		if (!page->configured)
		{
			KEST_PRINTF("Error! Page is unconfigured\n");
			return ERR_BAD_ARGS;
		}
		
		if (!page->create_ui)
		{
			KEST_PRINTF("Error! Page has no UI, and no create_ui function pointer!\n");
			return ERR_BAD_ARGS;
		}
	}
	
	if (!page->ui_created)
	{
		KEST_PRINTF("Page has not created its UI yet. Creating now...\n");
		page->create_ui(page);
	}
	
	if (page->refresh)
	{
		KEST_PRINTF("page has refresh; calling\n");
		page->refresh(page);
	}
	
	if (page->enter_page)
	{
		KEST_PRINTF("page has 'enter_page'; calling\n");
		page->enter_page(page);
	}

	if (!page->screen)
	{
		KEST_PRINTF("Error! Page has no screen!\n");
		return ERR_BAD_ARGS;
	}
	lv_scr_load(page->screen);

	global_cxt.pages.current_page = page;
	
	kest_event_log(kest_event_enter_page(page));
	
	return NO_ERROR;
}

void enter_ui_page_async_wrapper(void *ui_page)
{
	enter_ui_page((kest_ui_page*)ui_page);
}


void enter_ui_page_async(kest_ui_page *page)
{
	kest_ui_async_call(enter_ui_page_async_wrapper, (void*)page);
}

void enter_ui_page_forwards_async_wrapper(void *page)
{
	enter_ui_page_forwards((kest_ui_page*)page);
}

void enter_ui_page_backwards_async_wrapper(void *page)
{
	enter_ui_page_backwards((kest_ui_page*)page);
}

void kest_ui_page_enter_forwards_async(kest_ui_page *page)
{
	kest_ui_async_call(enter_ui_page_forwards_async_wrapper, (void*)page);
}

void kest_ui_page_enter_backwards_async(kest_ui_page *page)
{
	kest_ui_async_call(enter_ui_page_backwards_async_wrapper, (void*)page);
}

int enter_ui_page_forwards(kest_ui_page *page)
{
	if (!page)
	{
		KEST_PRINTF("Error! No page!\n");
		return ERR_NULL_PTR;
	}
	
	if (!page->ui_created)
	{
		if (!page->configured)
		{
			KEST_PRINTF("Error! Page is unconfigured\n");
			return ERR_BAD_ARGS;
		}
		
		if (!page->create_ui)
		{
			KEST_PRINTF("Error! Page has no UI, and no create_ui function pointer!\n");
			return ERR_BAD_ARGS;
		}
	}
	
	if (!page->ui_created)
	{
		KEST_PRINTF("Page has not created its UI yet. Creating now...\n");
		page->create_ui(page);
	}
	
	if (page->refresh)
	{
		KEST_PRINTF("page has refresh; calling\n");
		page->refresh(page);
	}
	
	if (page->enter_page)
	{
		KEST_PRINTF("page has 'enter_page'; calling\n");
		page->enter_page(page);
	}

	if (!page->screen)
	{
		KEST_PRINTF("Error! Page has no screen!\n");
		return ERR_BAD_ARGS;
	}
	lv_scr_load_anim(page->screen, LV_SCR_LOAD_ANIM_OUT_LEFT, UI_PAGE_TRANSITION_ANIM_MS, 0, false);
	
	global_cxt.pages.current_page = page;
	
	kest_event_log(kest_event_enter_page(page));
	
	return NO_ERROR;
}

int enter_ui_page_backwards(kest_ui_page *page)
{
	if (!page)
	{
		KEST_PRINTF("Error! No page!\n");
		return ERR_NULL_PTR;
	}
	
	if (!page->ui_created)
	{
		if (!page->configured)
		{
			KEST_PRINTF("Error! Page is unconfigured\n");
			return ERR_BAD_ARGS;
		}
		
		if (!page->create_ui)
		{
			KEST_PRINTF("Error! Page has no UI, and no create_ui function pointer!\n");
			return ERR_BAD_ARGS;
		}
	}
	
	if (!page->ui_created)
	{
		KEST_PRINTF("Page has not created its UI yet. Creating now...\n");
		page->create_ui(page);
	}
	
	if (page->refresh)
	{
		KEST_PRINTF("page has refresh; calling\n");
		page->refresh(page);
	}
	
	if (page->enter_page)
	{
		KEST_PRINTF("page has 'enter_page'; calling\n");
		page->enter_page(page);
	}
	
	if (!page->screen)
	{
		KEST_PRINTF("Error! Page has no screen!\n");
		return ERR_BAD_ARGS;
	}
	KEST_PRINTF("lv_scr_load...\n");
	lv_scr_load_anim(page->screen, LV_SCR_LOAD_ANIM_OUT_RIGHT, UI_PAGE_TRANSITION_ANIM_MS, 0, false);

	global_cxt.pages.current_page = page;
	
	kest_event_log(kest_event_enter_page(page));
	
	return NO_ERROR;
}

int enter_ui_page_indirect(kest_ui_page **_page)
{
	if (!_page)
		return ERR_NULL_PTR;
	
	enter_ui_page(*_page);
	
	return NO_ERROR;
}

int enter_ui_page_indirect_forwards(kest_ui_page **_page)
{
	if (!_page)
		return ERR_NULL_PTR;
	
	enter_ui_page_forwards(*_page);
	
	return NO_ERROR;
}

int enter_ui_page_backwardindirect_s(kest_ui_page **_page)
{
	if (!_page)
		return ERR_NULL_PTR;
	
	enter_ui_page_backwards(*_page);
	
	return NO_ERROR;
}

void enter_ui_page_cb(lv_event_t *e)
{
	//kest_printf("enter ui page callback triggered\n");
	kest_ui_page *page = (kest_ui_page*)lv_event_get_user_data(e);
	//kest_printf("Given page: %p\n", page);
	if (page)
		enter_ui_page(page);
}

void enter_ui_page_forwards_cb(lv_event_t *e)
{
	//kest_printf("enter ui page callback triggered\n");
	kest_ui_page *page = (kest_ui_page*)lv_event_get_user_data(e);
	//kest_printf("Given page: %p\n", page);
	if (page)
		enter_ui_page_forwards(page);
}

void enter_ui_page_backwards_cb(lv_event_t *e)
{
	//kest_printf("enter ui page callback triggered\n");
	kest_ui_page *page = (kest_ui_page*)lv_event_get_user_data(e);
	//kest_printf("Given page: %p\n", page);
	if (page)
		enter_ui_page_backwards(page);
}

void enter_parent_page_cb(lv_event_t *e)
{
	//kest_printf("enter_parent_page_cb\n");
	kest_ui_page *page = lv_event_get_user_data(e);
	
	if (!page)
	{
		//kest_printf("No page !\n");
		return;
	}
	
	if (page->parent)
		enter_ui_page_backwards(page->parent);
	else
		enter_ui_page_backwards(&global_cxt.pages.main_menu);
}

void kest_ui_page_return_to_parent(kest_ui_page *page)
{
	if (!page)
		return;
	
	if (!page->parent)
		return;
	
	if (page->parent->enter_page)
		page->parent->enter_page(page->parent);
	else
		enter_ui_page_backwards(page->parent);
}

int init_ui_page_panel_str(kest_ui_page_panel *panel)
{
	if (!panel)
		return ERR_NULL_PTR;
	
	panel->title 			  	= NULL;
	panel->left_button 		  	= NULL;
	panel->left_button_symbol 	= NULL;
	panel->right_button 		= NULL;
	panel->right_button_symbol 	= NULL;
	panel->text 		  		= NULL;
	
	return NO_ERROR;
}

int create_panel(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	page->panel = kest_alloc(sizeof(kest_ui_page_panel));
	
	if (!page->panel)
		return ERR_ALLOC_FAIL;
	
	init_ui_page_panel_str(page->panel);
	
	page->panel->panel = lv_obj_create(page->screen);
    
    lv_obj_set_size				 (page->panel->panel, LV_PCT(100), TOP_PANEL_HEIGHT);
    lv_obj_set_style_bg_color	 (page->panel->panel, lv_color_hex(TOP_PANEL_COLOUR), 0);
    lv_obj_set_style_pad_all	 (page->panel->panel, 0, 0);
    lv_obj_set_style_border_width(page->panel->panel, 0, 0);
    lv_obj_set_style_radius		 (page->panel->panel, 0, 0);
    lv_obj_set_style_pad_left	 (page->panel->panel, GLOBAL_PAD_WIDTH, 0);
	lv_obj_set_style_pad_right	 (page->panel->panel, GLOBAL_PAD_WIDTH, 0);
    //lv_obj_set_flex_flow		 (page->panel->panel, LV_FLEX_FLOW_ROW);
    //lv_obj_set_flex_align		 (page->panel->panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	
    return NO_ERROR;
}

int set_panel_text(kest_ui_page *page, const char *text)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (!page->panel)
		return ERR_BAD_ARGS;
	
	page->panel->text = text;
    
    if (text && page->panel->panel)
    {
		if (!page->panel->title)
			page->panel->title = lv_label_create(page->panel->panel);
			
		lv_label_set_text(page->panel->title, page->panel->text);
		
		//lv_obj_set_style_text_color(page->panel->title, lv_color_hex(GLOBAL_MAIN_TEXT_COLOUR), 0);
		//lv_obj_set_style_text_font(page->panel->title, GLOBAL_MAIN_FONT, 0);
		
		//lv_obj_set_flex_grow(page->panel->title, 1);
		lv_obj_set_style_text_align(page->panel->title, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_align_to(page->panel->title, page->panel->panel, LV_ALIGN_CENTER, 0, 0);
	}
	else
	{
		page->panel->title = NULL;
	}

    return NO_ERROR;
}

int set_panel_text_rw(kest_ui_page *page, const char *text)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (!page->panel)
		return ERR_BAD_ARGS;
	
	page->panel->text = text;
    
	page->panel->title = lv_textarea_create(page->panel->panel);
	lv_textarea_set_text(page->panel->title, page->panel->text);
	
	lv_obj_set_style_text_color(page->panel->title, lv_color_hex(GLOBAL_MAIN_TEXT_COLOUR), 0);
	lv_obj_set_style_text_font(page->panel->title, GLOBAL_MAIN_FONT, 0);
	
	lv_obj_set_style_border_width(page->panel->title, 0, 0);
    lv_obj_set_style_bg_color	 (page->panel->title, lv_color_hex(TOP_PANEL_COLOUR), 0);
	lv_obj_set_size(page->panel->title, LV_SIZE_CONTENT, TOP_PANEL_HEIGHT * 0.7);
	lv_textarea_set_one_line(page->panel->title, true);
	lv_textarea_set_align(page->panel->title, LV_TEXT_ALIGN_CENTER);
	
	lv_obj_align_to(page->panel->title, page->panel->panel, LV_ALIGN_CENTER, 0, 0);

    return NO_ERROR;
}

int create_panel_rw_title(kest_ui_page *page, const char *text)
{
	if (!page)
		return ERR_NULL_PTR;
	
	create_panel(page);

    set_panel_text_rw(page, text);

    return NO_ERROR;
}

int create_panel_left_button(kest_ui_page *page, const char *button_text, lv_event_cb_t cb, void *cb_arg)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (!page->panel)
		return ERR_BAD_ARGS;
	
	page->panel->left_button = lv_btn_create(page->panel->panel);
    lv_obj_set_size(page->panel->left_button, STANDARD_TOP_PANEL_BUTTON_WIDTH, STANDARD_TOP_PANEL_BUTTON_HEIGHT);
    lv_obj_align_to(page->panel->left_button, page->panel->panel, LV_ALIGN_LEFT_MID, 0, 0);
    
    page->panel->left_button_symbol = lv_label_create(page->panel->left_button);
    lv_label_set_text(page->panel->left_button_symbol, button_text);
    lv_obj_center(page->panel->left_button_symbol);
    
    if (cb)
		lv_obj_add_event_cb(page->panel->left_button, cb, LV_EVENT_CLICKED, cb_arg);
    
    return NO_ERROR;
}

int create_panel_right_button(kest_ui_page *page, const char *button_text, lv_event_cb_t cb, void *cb_arg)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (!page->panel)
		return ERR_BAD_ARGS;
	
	page->panel->right_button = lv_btn_create(page->panel->panel);
    lv_obj_set_size(page->panel->right_button, STANDARD_TOP_PANEL_BUTTON_WIDTH, STANDARD_TOP_PANEL_BUTTON_HEIGHT);
    lv_obj_align_to(page->panel->right_button, page->panel->panel, LV_ALIGN_RIGHT_MID, 0, 0);
    
    page->panel->right_button_symbol = lv_label_create(page->panel->right_button);
    lv_label_set_text(page->panel->right_button_symbol, button_text);
    lv_obj_center(page->panel->right_button_symbol);
    
    if (cb)
		lv_obj_add_event_cb(page->panel->right_button, cb, LV_EVENT_CLICKED, cb_arg);
    
    return NO_ERROR;
}

int create_panel_rw_title_and_left_button(kest_ui_page *page, const char *text, const char *left_button_text, lv_event_cb_t left_cb, void *left_cb_arg)
{
	if (!page)
		return ERR_NULL_PTR;
	
	create_panel(page);

    create_panel_left_button(page, left_button_text, left_cb, left_cb_arg);
    
    set_panel_text_rw(page, text);

    return NO_ERROR;
}

int create_panel_with_back_button(kest_ui_page *page)
{
	return create_panel_with_left_button(page, LV_SYMBOL_LEFT, enter_parent_page_cb, page);
}

int create_panel_with_back_and_settings_buttons(kest_ui_page *page, kest_ui_page *settings_page)
{
	return create_panel_with_back_button_and_page_button(page, LV_SYMBOL_SETTINGS, settings_page);
}

int create_panel_with_back_button_and_right_button(kest_ui_page *page, const char *right_button_text, lv_event_cb_t right_cb, void *cb_arg)
{
	return create_panel_with_left_and_right_buttons(page, LV_SYMBOL_LEFT, enter_parent_page_cb, page, right_button_text, right_cb, cb_arg);
}

int create_panel_with_left_button(kest_ui_page *page, const char *left_button_text, lv_event_cb_t left_cb, void *cb_arg)
{
	if (!page)
		return ERR_NULL_PTR;
	
	create_panel(page);
	
	create_panel_left_button(page, left_button_text, left_cb, cb_arg);

    return NO_ERROR;
}

int create_panel_with_right_button(kest_ui_page *page, const char *right_button_text, lv_event_cb_t right_cb, void *cb_arg)
{
	if (!page)
		return ERR_NULL_PTR;
	
	create_panel_with_back_button(page);
    
    create_panel_right_button(page, right_button_text, right_cb, cb_arg);

    return NO_ERROR;
}

int create_panel_with_back_button_and_page_button(kest_ui_page *page, const char *right_button_text, kest_ui_page *right_button_page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	create_panel_with_back_button(page);
    
    create_panel_right_button(page, right_button_text, enter_ui_page_forwards_cb, right_button_page);

    return NO_ERROR;
}

int create_panel_with_left_and_right_buttons(kest_ui_page *page,
	const char *left_button_text, lv_event_cb_t left_cb, void *left_cb_arg,
	const char *right_button_text, lv_event_cb_t right_cb, void *right_cb_arg)
{
	if (!page)
		return ERR_NULL_PTR;
	
	create_panel(page);
	
	create_panel_left_button(page, left_button_text, left_cb, left_cb_arg);
    
    create_panel_right_button(page, right_button_text, right_cb, right_cb_arg);

    return NO_ERROR;
}

void delete_keyboard()
{
	if (keyboard)
		lv_obj_delete(keyboard);
	keyboard = NULL;
}

void spawn_keyboard(lv_obj_t *parent, lv_obj_t *text_area, void (*ok_cb)(lv_event_t*), void *ok_arg, void (*cancel_cb)(lv_event_t*), void *cancel_arg)
{
	if (keyboard)
	{
		delete_keyboard();
	}

	keyboard = lv_keyboard_create(parent);
	lv_obj_set_size(keyboard, LV_PCT(100), LV_PCT(33));
	lv_keyboard_set_textarea(keyboard, text_area);
	
	lv_obj_add_event_cb(keyboard, ok_cb, 		LV_EVENT_READY, 	ok_arg);
	lv_obj_add_event_cb(keyboard, cancel_cb, 	LV_EVENT_CANCEL, 	cancel_arg);
	lv_obj_add_event_cb(text_area, 					cancel_cb, 	LV_EVENT_DEFOCUSED, cancel_arg);
	lv_obj_add_event_cb(text_area, 					cancel_cb, 	LV_EVENT_LEAVE, 	cancel_arg);
}

void spawn_numerical_keyboard(lv_obj_t *parent, lv_obj_t *text_area, void (*ok_cb)(lv_event_t*), void *ok_arg, void (*cancel_cb)(lv_event_t*), void *cancel_arg)
{
	spawn_keyboard(parent, text_area, ok_cb, ok_arg, cancel_cb, cancel_arg);
	
	lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
}

void hide_keyboard()
{
	delete_keyboard();
}

void hide_keyboard_cb(lv_event_t *e)
{
	hide_keyboard();
}

int create_standard_container(lv_obj_t **cont, lv_obj_t *parent)
{
	if (!cont || !parent)
		return ERR_NULL_PTR;
	
	*cont = lv_obj_create(parent);
	if (!*cont)
		return ERR_ALLOC_FAIL;
	
	lv_obj_set_size(*cont, STANDARD_CONTAINER_WIDTH, STANDARD_CONTAINER_HEIGHT);
	lv_obj_align_to(*cont, parent, LV_ALIGN_CENTER, 0, STANDARD_YPOS);
	
	lv_obj_set_style_anim_time(*cont, 0, 0);
	
	return NO_ERROR;
}

int create_standard_container_tall(lv_obj_t **cont, lv_obj_t *parent)
{
	if (!cont || !parent)
		return ERR_NULL_PTR;
	
	*cont = lv_obj_create(parent);
	
	if (!*cont)
		return ERR_ALLOC_FAIL;
	
	lv_obj_set_size(*cont, STANDARD_CONTAINER_WIDTH, STANDARD_CONTAINER_TALL_HEIGHT);
	lv_obj_align_to(*cont, parent, LV_ALIGN_CENTER, 0, STANDARD_YPOS + (STANDARD_CONTAINER_TALL_HEIGHT - STANDARD_CONTAINER_HEIGHT) / 2);
	
	lv_obj_set_style_anim_time(*cont, 0, 0);
	
	return NO_ERROR;
}

int create_standard_menu_container(lv_obj_t **cont, lv_obj_t *parent)
{
	if (!cont || !parent)
		return ERR_NULL_PTR;
	
	*cont = lv_obj_create(parent);
	if (!*cont)
		return ERR_ALLOC_FAIL;
	
	lv_obj_set_size(*cont, STANDARD_CONTAINER_WIDTH, STANDARD_CONTAINER_HEIGHT);
	lv_obj_align_to(*cont, parent, LV_ALIGN_CENTER, 0, STANDARD_YPOS);
	
	lv_obj_set_style_anim_time(*cont, 0, 0);
	lv_obj_set_flex_align(*cont,
		LV_FLEX_ALIGN_SPACE_EVENLY,
		LV_FLEX_ALIGN_CENTER,
		LV_FLEX_ALIGN_CENTER);
	
	return NO_ERROR;
}

int create_standard_menu_container_tall(lv_obj_t **cont, lv_obj_t *parent)
{
	if (!cont || !parent)
		return ERR_NULL_PTR;
	
	*cont = lv_obj_create(parent);
	
	if (!*cont)
		return ERR_ALLOC_FAIL;
	
	lv_obj_set_size(*cont, STANDARD_CONTAINER_WIDTH, STANDARD_CONTAINER_TALL_HEIGHT);
	lv_obj_align_to(*cont, parent, LV_ALIGN_CENTER, 0, STANDARD_YPOS + (STANDARD_CONTAINER_TALL_HEIGHT - STANDARD_CONTAINER_HEIGHT) / 2);
	
	lv_obj_set_style_anim_time(*cont, 0, 0);
	
	lv_obj_set_flex_flow(*cont, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(*cont,
		LV_FLEX_ALIGN_SPACE_EVENLY,
		LV_FLEX_ALIGN_CENTER,
		LV_FLEX_ALIGN_CENTER);

	
	return NO_ERROR;
}

int create_standard_button_list(lv_obj_t **cont, lv_obj_t *parent)
{
	if (!cont || !parent)
		return ERR_NULL_PTR;
	
	*cont = lv_obj_create(parent);
	if (!*cont)
		return ERR_ALLOC_FAIL;
	
	lv_obj_set_size(*cont, STANDARD_CONTAINER_WIDTH, STANDARD_CONTAINER_HEIGHT);
	lv_obj_align_to(*cont, parent, LV_ALIGN_CENTER, 0, STANDARD_YPOS);
	lv_obj_set_flex_flow(*cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(*cont,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
	
	return NO_ERROR;
}

int create_standard_button_list_tall(lv_obj_t **cont, lv_obj_t *parent)
{
	if (!cont || !parent)
		return ERR_NULL_PTR;
	
	*cont = lv_obj_create(parent);
	if (!*cont)
		return ERR_ALLOC_FAIL;
	
	lv_obj_set_size(*cont, STANDARD_CONTAINER_WIDTH, STANDARD_CONTAINER_TALL_HEIGHT);
	lv_obj_align_to(*cont, parent, LV_ALIGN_CENTER, 0, STANDARD_YPOS + (STANDARD_CONTAINER_TALL_HEIGHT - STANDARD_CONTAINER_HEIGHT) / 2);
	
	lv_obj_set_flex_flow(*cont, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(*cont,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
	
	return NO_ERROR;
}

int create_standard_button_click(lv_obj_t **obj, lv_obj_t **label, lv_obj_t *parent,
	char *text, lv_event_cb_t click_cb, void *click_cb_arg)
{
	if (!obj || !label || !parent)
		return ERR_NULL_PTR;
	
	*obj = lv_btn_create(parent);
    lv_obj_set_size(*obj, LV_PCT(100), STANDARD_BUTTON_HEIGHT);
    
	*label = lv_label_create(*obj);
	
	if (text)
		lv_label_set_text(*label, text);
	
	lv_obj_center(*label);
	
	lv_obj_add_event_cb(*obj, click_cb, LV_EVENT_CLICKED, click_cb_arg);
	
	return NO_ERROR;
}

int create_standard_button_click_short(lv_obj_t **obj, lv_obj_t **label, lv_obj_t *parent,
	char *text, lv_event_cb_t click_cb, void *click_cb_arg)
{
	if (!obj || !label || !parent)
		return ERR_NULL_PTR;
	
	*obj = lv_btn_create(parent);
    lv_obj_set_size(*obj, LV_PCT(100), STANDARD_BUTTON_SHORT_HEIGHT);
    
	*label = lv_label_create(*obj);
	
	if (text)
		lv_label_set_text(*label, text);
	
	lv_obj_center(*label);
	
	lv_obj_add_event_cb(*obj, click_cb, LV_EVENT_CLICKED, click_cb_arg);
	
	return NO_ERROR;
}

int create_standard_button_long_press_release(lv_obj_t **obj, lv_obj_t **label, lv_obj_t *parent,
	char *text, lv_event_cb_t press_cb, void *press_cb_arg, lv_event_cb_t release_cb, void *release_cb_arg)
{
	if (!obj || !label || !parent)
		return ERR_NULL_PTR;
	
	*obj = lv_btn_create(parent);
    lv_obj_set_size(*obj, LV_PCT(100), STANDARD_BUTTON_HEIGHT);
    
	*label = lv_label_create(*obj);
	
	if (text)
		lv_label_set_text(*label, text);
	
	lv_obj_center(*label);
	
	lv_obj_add_event_cb(*obj, press_cb, LV_EVENT_LONG_PRESSED, press_cb_arg);
	lv_obj_add_event_cb(*obj, release_cb, LV_EVENT_RELEASED, release_cb_arg);
	
	return NO_ERROR;
}

int init_panel(kest_ui_page_panel *panel)
{
	if (!panel)
		return ERR_NULL_PTR;
	
	panel->panel = NULL;
	panel->title = NULL;
	panel->text  = NULL;
	
	panel->lb = NULL;
	panel->rb = NULL;
	
	panel->left_button = NULL;
	panel->left_button_symbol = NULL;
	panel->right_button = NULL;
	panel->right_button_symbol = NULL;
	
	return NO_ERROR;
}

kest_ui_page_panel *new_panel()
{
	kest_ui_page_panel *panel = kest_alloc(sizeof(kest_ui_page_panel));
	
	if (!panel)
		return NULL;
	
	init_panel(panel);
	
	return panel;
}

static void edit_rw_text_cb(lv_event_t *e)
{
	kest_ui_page *page = (kest_ui_page*)lv_event_get_user_data(e);
	
	if (!page)
		return;
	
	spawn_keyboard(page->screen, page->panel->title, page->panel->rw_save_cb, page, page->panel->rw_cancel_cb, page);
	lv_obj_add_state(page->panel->title, LV_STATE_FOCUSED);
}

int ui_page_update_title(kest_ui_page *page, const char *text)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (!page->panel)
		return ERR_BAD_ARGS;
	
	if (page->panel->text != text)
		return ui_page_set_title(page, text);
	
	return NO_ERROR;
}

int ui_page_init_create_panel_label(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (!page->panel)
		return ERR_BAD_ARGS;
	
	char *text = page->panel->text ? page->panel->text : "";
	
	if (page->panel->flags & TOP_PANEL_FLAG_RW_TITLE)
	{
		page->panel->title = lv_textarea_create(page->panel->panel);
		lv_textarea_set_text(page->panel->title, text);
		
		lv_obj_set_style_border_width(page->panel->title, 0, 0);
		lv_obj_set_style_bg_color	 (page->panel->title, lv_color_hex(TOP_PANEL_COLOUR), 0);
		lv_textarea_set_one_line(page->panel->title, true);
		lv_textarea_set_align(page->panel->title, LV_TEXT_ALIGN_CENTER);
		
		
		lv_obj_add_event_cb(page->panel->title, edit_rw_text_cb, LV_EVENT_CLICKED, page);
	}
	else
	{
		page->panel->title = lv_label_create(page->panel->panel);
		lv_label_set_text(page->panel->title, text);
		
		lv_obj_set_style_text_align(page->panel->title, LV_TEXT_ALIGN_CENTER, 0);
		lv_obj_align_to(page->panel->title, page->panel->panel, LV_ALIGN_CENTER, 0, 0);
	}
	
	lv_obj_align_to(page->panel->title, page->panel->panel, LV_ALIGN_CENTER, 0, 0);
	//lv_obj_set_size(page->panel->title, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	
	return NO_ERROR;
}

int ui_page_set_title(kest_ui_page *page, const char *text)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (!page->panel)
		return ERR_BAD_ARGS;
	
	page->panel->text = text;
    
    if (text && page->ui_created && page->panel->panel)
    {
		if (!page->panel->title)
			return ui_page_init_create_panel_label(page);
		
		if (page->panel->flags & TOP_PANEL_FLAG_RW_TITLE)
			lv_textarea_set_text(page->panel->title, page->panel->text);
		else	
			lv_label_set_text(page->panel->title, page->panel->text);
	}
	else
	{
		page->panel->title = NULL;
	}
	
	return NO_ERROR;
}

int ui_page_create_panel_ui(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (!page->panel)
		return ERR_BAD_ARGS;
	
	KEST_PRINTF("ui_page_create_panel_ui(page = %p)\n", page);
	
	page->panel->panel = lv_obj_create(page->screen);
    
    lv_obj_set_size				 (page->panel->panel, LV_PCT(100), TOP_PANEL_HEIGHT);
    lv_obj_set_style_bg_color	 (page->panel->panel, lv_color_hex(TOP_PANEL_COLOUR), 0);
    lv_obj_set_style_pad_all	 (page->panel->panel, 0, 0);
    lv_obj_set_style_border_width(page->panel->panel, 0, 0);
    lv_obj_set_style_radius		 (page->panel->panel, 0, 0);
    lv_obj_set_style_pad_left	 (page->panel->panel, GLOBAL_PAD_WIDTH, 0);
	lv_obj_set_style_pad_right	 (page->panel->panel, GLOBAL_PAD_WIDTH, 0);
	
	if (page->panel->lb)
	{
		create_button_ui(page->panel->lb, page->panel->panel);
		lv_obj_align_to(page->panel->lb->obj, page->panel->panel, LV_ALIGN_LEFT_MID, 0, 0);
	}
	
	ui_page_init_create_panel_label(page);
	
	if (page->panel->rb)
	{
		create_button_ui(page->panel->rb, page->panel->panel);
		lv_obj_align_to(page->panel->rb->obj, page->panel->panel, LV_ALIGN_RIGHT_MID, 0, 0);
	}
	
	KEST_PRINTF("ui_page_create_panel_ui done\n");
	
	return NO_ERROR;
}

int ui_page_add_back_button(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (!page->panel)
		return ERR_BAD_ARGS;
	
	KEST_PRINTF("ui_page_add_back_button(page = %p)\n", page);
	
	page->panel->lb = new_button(LV_SYMBOL_LEFT);
	
	if (!page->panel->lb)
		return ERR_ALLOC_FAIL;
	
	page->panel->lb->width  = STANDARD_TOP_PANEL_BUTTON_WIDTH;
	page->panel->lb->height = STANDARD_TOP_PANEL_BUTTON_HEIGHT;
	
	button_set_clicked_cb(page->panel->lb, enter_parent_page_cb, page);
	
	KEST_PRINTF("ui_page_add_back_button done\n");
	return NO_ERROR;
}

int ui_page_add_parent_button(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (!page->panel)
		return ERR_BAD_ARGS;
	
	page->panel->lb = new_button(LV_SYMBOL_LEFT);
	
	if (!page->panel->lb)
		return ERR_ALLOC_FAIL;
	
	page->panel->lb->width  = STANDARD_TOP_PANEL_BUTTON_WIDTH;
	page->panel->lb->height = STANDARD_TOP_PANEL_BUTTON_HEIGHT;
	
	button_set_clicked_cb(page->panel->lb, enter_parent_page_cb, page);
	
	return NO_ERROR;
}

int ui_page_add_left_panel_button(kest_ui_page *page, const char *label, lv_event_cb_t cb)
{
	if (!page)
		return ERR_NULL_PTR;
	
	page->panel->lb = new_button(label);
	
	if (!page->panel->lb)
		return ERR_ALLOC_FAIL;
	
	page->panel->lb->width  = STANDARD_TOP_PANEL_BUTTON_WIDTH;
	page->panel->lb->height = STANDARD_TOP_PANEL_BUTTON_HEIGHT;
	
	button_set_clicked_cb(page->panel->lb, cb, page);
	
	return NO_ERROR;
}

int ui_page_add_right_panel_button(kest_ui_page *page, const char *label, lv_event_cb_t cb)
{
	if (!page)
		return ERR_NULL_PTR;
	
	page->panel->rb = new_button(label);
	
	if (!page->panel->rb)
		return ERR_ALLOC_FAIL;
	
	page->panel->rb->width  = STANDARD_TOP_PANEL_BUTTON_WIDTH;
	page->panel->rb->height = STANDARD_TOP_PANEL_BUTTON_HEIGHT;
	
	button_set_clicked_cb(page->panel->rb, cb, page);
	
	return NO_ERROR;
}

int ui_page_create_base_ui(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	page->screen = lv_obj_create(NULL);
	
	if (!page->screen)
		return ERR_ALLOC_FAIL;
	
	if (page->panel)
	{
		ui_page_create_panel_ui(page);
	}
	
	ui_page_create_container(page);
	
	ui_page_create_bottom_buttons(page);
	
	return NO_ERROR;
}


int ui_page_create_container(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("ui_page_create_container(page = %p)\n", page);
	
	int tall = 1;
	
	for (int i = 0; i < MAX_BOTTOM_BUTTONS; i++)
	{
		if (page->bottom_buttons[i])
			tall = 0;
	}
	
	KEST_PRINTF("page->container_type = %d\n", page->container_type);
	
	switch (page->container_type)
	{
		case CONTAINER_TYPE_STD:
			if (tall)
				return create_standard_container_tall(&page->container, page->screen);
			
			return create_standard_container(&page->container, page->screen);
			
		case CONTAINER_TYPE_STD_BTN_LIST:
			if (tall)
				return create_standard_button_list_tall(&page->container, page->screen);
				
			return create_standard_button_list(&page->container, page->screen);
		case CONTAINER_TYPE_STD_MENU:
			if (tall)
				return create_standard_menu_container_tall(&page->container, page->screen);
				
			return create_standard_menu_container(&page->container, page->screen);
			
		default:
			return ERR_BAD_ARGS;
	}
	
	KEST_PRINTF("ui_page_create_container done\n");
	return NO_ERROR;
}

int ui_page_create_bottom_buttons(kest_ui_page *page)
{
	if (!page)
		return ERR_NULL_PTR;
	
	int n = 0;
	
	for (int i = 0; i < MAX_BOTTOM_BUTTONS; i++)
	{
		if (page->bottom_buttons[i])
			n++;
	}
	
	if (!n)
		return NO_ERROR;
	
	int pad = (n <= 1) ? 0 : (int)((float)BOTTOM_BUTTON_PADDING / n) + BOTTOM_BUTTON_PADDING / 2;
	int width = (int)(float)(STANDARD_CONTAINER_WIDTH - (n - 1) * pad) / n;
	int height = STANDARD_BUTTON_HEIGHT;
	
	int pos;
	
	for (int i = 0; i < MAX_BOTTOM_BUTTONS; i++)
	{
		if (page->bottom_buttons[i])
		{
			create_button_ui(page->bottom_buttons[i], page->screen);
			lv_obj_set_size(page->bottom_buttons[i]->obj, width, height);
			
			pos = (i * (width + pad) + width / 2) - STANDARD_CONTAINER_WIDTH / 2;
			lv_obj_align(page->bottom_buttons[i]->obj, LV_ALIGN_BOTTOM_MID, pos, -50);
		}
	}
	
	return NO_ERROR;
}

kest_button *ui_page_add_bottom_button(kest_ui_page *page, const char *label, lv_event_cb_t cb)
{
	if (!page)
		return NULL;
	
	int index = -1;
	
	for (int i = 0; i < MAX_BOTTOM_BUTTONS; i++)
	{
		if (!page->bottom_buttons[i])
		{
			index = i;
			break;
		}
	}
	
	if (index == -1)
		return NULL;
	
	page->bottom_buttons[index] = new_button(label);
	
	if (cb)
		button_set_clicked_cb(page->bottom_buttons[index], cb, page);
	
	return page->bottom_buttons[index];
}

int ui_page_set_title_rw(kest_ui_page *page, lv_event_cb_t save_cb, lv_event_cb_t cancel_cb)
{
	if (!page)
		return ERR_NULL_PTR;
	
	if (!page->panel)
		return ERR_BAD_ARGS;
	
	page->panel->flags |= TOP_PANEL_FLAG_RW_TITLE;
	
	page->panel->rw_save_cb = save_cb;
	page->panel->rw_cancel_cb = cancel_cb;
	
	return NO_ERROR;
}

int kest_ui_lock()
{
	#ifdef KEST_PLATFORM_P4_NANO
	return bsp_display_lock(0);
	#else
	lv_lock();
	return 1;
	#endif
}

void kest_ui_unlock()
{
	#ifdef KEST_PLATFORM_P4_NANO
	bsp_display_unlock();
	#else
	lv_unlock();
	#endif
}

void kest_ui_async_call(void (*f)(void*), void *arg)
{
#if ASYNC_NEEDS_LOCK
	if (kest_ui_lock())
	{
#endif
		lv_async_call(f, arg);
#if ASYNC_NEEDS_LOCK
		kest_ui_unlock();
	}
#endif
}

void kest_async_call_void_wrapper(void *arg)
{
	void (*f)(void) = arg;
	f();
}

void kest_ui_async_call_void(void (*f)(void))
{
#if ASYNC_NEEDS_LOCK
	if (kest_ui_lock())
	{
#endif
		lv_async_call(kest_async_call_void_wrapper, (void*)f);
#if ASYNC_NEEDS_LOCK
		kest_ui_unlock();
	}
#endif
}

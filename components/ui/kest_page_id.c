#include "kest_int.h"

#define PRINTLINES_ALLOWED 1

static const char *FNAME = "kest_page_id.c";

kest_ui_page *kest_page_id_find_page(kest_context *cxt, kest_page_identifier id)
{
	KEST_PRINTF("kest_page_id_find_page; searching for the page described by {.type = %d, .id = %d, .fname = \"%s\"}\n",
		id.type, id.id, id.fname);
	
	if (!cxt)
		return NULL;
	
	kest_ui_page *page;
	kest_effect_view_str *tv_str;
	kest_effect *effect = NULL;
	kest_preset *preset = NULL;
	kest_sequence *seq = NULL;
	
	switch (id.type)
	{
		case KEST_UI_PAGE_MAIN_MENU: 	KEST_PRINTF("It is simply the main menu !\n"); return &cxt->pages.main_menu;
		case KEST_UI_PAGE_SEQ_LIST: 	KEST_PRINTF("It is simply the sequence list !\n"); return &cxt->pages.sequence_list;
		case KEST_UI_PAGE_MSV:  		KEST_PRINTF("It is simply the main sequence view !\n"); return &cxt->pages.main_sequence_view;
		case KEST_UI_PAGE_SEQ_VIEW:
			KEST_PRINTF("It is a sequence view.\n");
			seq = cxt_get_sequence_by_fname(cxt, id.fname);
			
			if (!seq) return NULL;
			else return seq->view_page;
			
		case KEST_UI_PAGE_PRESET_VIEW: 
			KEST_PRINTF("It is a preset view.\n");
			preset = cxt_get_preset_by_fname(cxt, id.fname);
			
			if (!preset) return NULL;
			else return preset->view_page;
			
		case KEST_UI_PAGE_EFFECT_VIEW: 
			KEST_PRINTF("It is a effect view. First, search for the preset with fname \"%s\"\n", id.fname);
			preset = cxt_get_preset_by_fname(cxt, id.fname);
			KEST_PRINTF("The returned preset: %p\n", preset);
			if (!preset)
			{
				KEST_PRINTF("Unforunately, it is NULL! All is lost!\n");
				return NULL;
			}
			
			KEST_PRINTF("It is non-NULL! Yay!. Now we must try to find among it the desired effect; it has ID %d\n", id.id);
			
			effect = kest_preset_get_effect_by_id(preset, id.id);
			
			if (!effect)
			{
				KEST_PRINTF("The effect was not found. No chance of locating the page. Return NULL\n");
				return NULL;
			}
			else
			{
				KEST_PRINTF("The effect was found! Its view page is %p.\n", effect->view_page);
				KEST_PRINTF("Send it.\n");
				return effect->view_page;
			}
			
		case KEST_UI_PAGE_EFFECT_SETTINGS: 
			KEST_PRINTF("It is a effect settings page.\n");
			preset = cxt_get_preset_by_fname(cxt, id.fname);
			
			if (!preset) return NULL;
			
			effect = kest_preset_get_effect_by_id(preset, id.id);
			
			if (!effect) return NULL;
			page = effect->view_page;
			tv_str = (kest_effect_view_str*)page->data_struct;
			
			if (!tv_str) return NULL;
			else return tv_str->settings_page;
		
		default:
			return NULL;
	}
	
	KEST_PRINTF("Couldn't find it :(\n");
	
	return NULL;
}

int kest_ui_page_create_identifier(kest_ui_page *page, kest_page_identifier *id)
{
	KEST_PRINTF("kest_ui_page_create_identifier(page = %p, id = %p)\n", page, id);
	
	if (!page || !id)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("page->type = %d\n", page->type);
	
	id->type = page->type;
	id->fname[0] = 0;
	id->id = 0;
	
	if (page->type == KEST_UI_PAGE_MAIN_MENU
	 || page->type == KEST_UI_PAGE_SEQ_LIST
	 || page->type == KEST_UI_PAGE_MSV)
	{
		return NO_ERROR;
	}
	
	if (!page->data_struct)
		return ERR_BAD_ARGS;
	
	kest_preset_view_str	*pv_str = (kest_preset_view_str*)		page->data_struct;
	kest_sequence_view_str 	*sv_str = (kest_sequence_view_str*)		page->data_struct;
	kest_effect_view_str 	*tv_str = (kest_effect_view_str*)		page->data_struct;
	effect_settings_page_str *ts_str = (effect_settings_page_str*)	page->data_struct;
	kest_effect *effect = NULL;
	kest_preset *preset = NULL;
	kest_sequence *seq = NULL;
	
	char *fname = NULL;
	
	switch (page->type)
	{
		case KEST_UI_PAGE_SEQ_VIEW:
			seq = sv_str->sequence;
			break;
			
		case KEST_UI_PAGE_PRESET_VIEW: 
			preset = pv_str->preset;
			break;
			
		case KEST_UI_PAGE_EFFECT_VIEW:
			effect = tv_str->effect;
			
			if (!effect) return ERR_BAD_ARGS;
			break;
			
		case KEST_UI_PAGE_EFFECT_SETTINGS:
			effect = ts_str->effect;
			
			if (!effect) return ERR_BAD_ARGS;
			break;
		
		default:
			return ERR_BAD_ARGS;
	}
	
	if (effect)
	{
		id->id = effect->id;
		preset = effect->preset;
	}
	
	if (seq)
		fname = seq->fname;
	else if (preset)
		fname = preset->fname;
	else
		return ERR_BAD_ARGS;
	
	if (fname)
	{
		for (int i = 0; i < 32; i++)
		{
			id->fname[i] = fname[i];
			if (!fname[i])
				break;
			else if (i == 31) 
				fname[i] = 0;
		}
	}
	
	return NO_ERROR;
}

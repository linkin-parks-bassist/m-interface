#include "kest_int.h"

#define PRINTLINES_ALLOWED 1

static const char *FNAME = "kest_state.c";

#ifdef KEST_USE_FREERTOS
SemaphoreHandle_t state_mutex;
#endif

int kest_cxt_clone_state(kest_context *cxt, kest_state *state)
{
	int len;
	
	if (!cxt)
		return ERR_NULL_PTR;
	
	if (!state)
		return ERR_BAD_ARGS;
	
	if (kest_cxt_obtain_mutex(cxt) != NO_ERROR)
		return ERR_MUTEX_UNAVAILABLE;
	
	state->input_gain  = cxt->input_gain.value;
	state->output_gain = cxt->output_gain.value;
	
	kest_ui_page_create_identifier(cxt->pages.current_page, &state->current_page);
	
	if (cxt->active_preset && cxt->active_preset->has_fname)
	{
		len = strlen(cxt->active_preset->fname);
		if (len > 31)
			len = 31;
		
		memcpy(state->active_preset_fname, cxt->active_preset->fname, len);
		state->active_preset_fname[len] = 0;
	}
	else
	{
		state->active_preset_fname[0] = 0;
	}
	
	if (cxt->sequence && cxt->sequence->has_fname)
	{
		len = strlen(cxt->sequence->fname);
		if (len > KEST_FILENAME_LEN - 1)
			len = KEST_FILENAME_LEN - 1;
		
		memcpy(state->active_sequence_fname, cxt->sequence->fname, len);
		state->active_sequence_fname[len] = 0;
	}
	else
	{
		state->active_sequence_fname[0] = 0;
	}
	
	kest_cxt_release_mutex(cxt);
	
	return NO_ERROR;
}

int kest_state_save(kest_state state)
{
	KEST_PRINTF("kest_save_state\n");
	
	int ret_val = safe_file_write((int (*)(void*, const char*))save_state_to_file, &state, SETTINGS_FNAME);
	KEST_PRINTF("kest_save_state return code: %s\n", kest_error_code_to_string(ret_val));
	
	return ret_val;
}

void kest_state_representation_update(void *representer, void *representee)
{
	KEST_PRINTF("kest_state_representation_update\n");
	
	if (!representee)
		return;
	
	kest_context *cxt = (kest_context*)representee;
	kest_state state;
	
	memset(&state, 0, sizeof(kest_state));
	
	int ret_val;
	
	if ((ret_val = kest_cxt_clone_state(cxt, &state)) != NO_ERROR)
	{
		KEST_PRINTF("Failed to clone state; aborting\n");
		return;
	}
	
	KEST_PRINTF("Cloned state\n");
	
	safe_file_write((int (*)(void*, const char*))save_state_to_file, &state, SETTINGS_FNAME);
	KEST_PRINTF("kest_state_representation_update done\n");
	
	return;
}


int kest_cxt_restore_state(kest_context *cxt, kest_state *state)
{
	KEST_PRINTF("kest_cxt_restore_state\n");
	
	if (!cxt || !state)
		return ERR_NULL_PTR;
	
	kest_preset *preset = NULL;
	kest_sequence *sequence = NULL;
	kest_ui_page *page;
	
	int ret_val = NO_ERROR;
	int tries = 0;
	
	preset = cxt_get_preset_by_fname(cxt, state->active_preset_fname);
	
	if (preset)
		set_active_preset_from_sequence(preset);
	
	KEST_PRINTF("Setting input gain to %f...\n", state->input_gain);
	kest_cxt_set_input_gain (cxt, state->input_gain);
	KEST_PRINTF("Setting output gain to %f...\n", state->output_gain);
	kest_cxt_set_output_gain(cxt, state->output_gain);
	
	KEST_PRINTF("kest_cxt_restore_state done\n");
	return NO_ERROR;
}

int kest_cxt_enter_previous_current_page(kest_context *cxt, kest_state *state)
{
	if (!cxt || !state)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("kest_cxt_enter_previous_current_page\n");
		
	kest_ui_page *page = kest_page_id_find_page(cxt, state->current_page);
	
	KEST_PRINTF("page = %p\n", page);
	
	enter_ui_page_async(page);
	
	KEST_PRINTF("kest_cxt_enter_previous_current_page done\n");
	
	return NO_ERROR;
}

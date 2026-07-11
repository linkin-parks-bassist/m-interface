#include "kest_int.h"

#define PRINTLINES_ALLOWED 0

static const char *FNAME = "kest_context.c";

int kest_init_context(kest_context *cxt)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	cxt->n_presets = 0;
	
	cxt->active_preset  = NULL;
	cxt->working_preset = NULL;
	
	cxt->presets  = NULL;
	cxt->sequences = NULL;
	cxt->sequence  = NULL;
	
	kest_context_init_main_sequence(cxt);
	
	cxt->main_sequence.name = "Presets";
	
	cxt->saved_presets_loaded  = 0;
	cxt->saved_sequences_loaded = 0;
	
	cxt->pages.backstage = NULL;
	cxt->pages.current_page = NULL;
	
	cxt->effects = NULL;
	
	init_parameter(&cxt->input_gain, "Input Gain", -100, -24.0, 24.0);
	cxt->input_gain.units = " dB";
	cxt->input_gain.id = (kest_parameter_id){.preset_id = CONTEXT_PRESET_ID, .effect_id = 0, .parameter_id = INPUT_GAIN_PID};
	cxt->input_gain.max_velocity = 0.4;
	cxt->input_gain.min_expr = &kest_expression_standard_gain_min;
	cxt->input_gain.max_expr = &kest_expression_standard_gain_max;
	
	init_parameter(&cxt->output_gain, "Output Gain", -100, -24.0, 24.0);
	cxt->output_gain.units = " dB";
	cxt->output_gain.id = (kest_parameter_id){.preset_id = CONTEXT_PRESET_ID, .effect_id = 0, .parameter_id = OUTPUT_GAIN_PID};
	cxt->output_gain.max_velocity = 0.4;
	cxt->output_gain.min_expr = &kest_expression_standard_gain_min;
	cxt->output_gain.max_expr = &kest_expression_standard_gain_max;
	
	#ifdef KEST_USE_FREERTOS
	cxt->mutex = xSemaphoreCreateMutex();
	#endif
	
	return NO_ERROR;
}

int kest_context_init_main_sequence(kest_context *cxt)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	init_m_sequence(&cxt->main_sequence);
	
	cxt->main_sequence.name = "Presets";
	cxt->main_sequence.view_page = &cxt->pages.main_sequence_view;
	
	for (int k = 0; k < KEST_FILENAME_LEN; k++)
	{
		if (!MAIN_SEQUENCE_FNAME[k])
		{
			cxt->main_sequence.fname[k] = 0;
			break;
		}
		
		cxt->main_sequence.fname[k] = MAIN_SEQUENCE_FNAME[k];
	}
	
	cxt->main_sequence.has_fname = 1;
	cxt->main_sequence.main_sequence = 1;
	
	return NO_ERROR;
}

int kest_context_init_ui(kest_context *cxt)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	cxt->pages.backstage = NULL;
	cxt->pages.current_page = NULL;
	
	return NO_ERROR;
}

int kest_context_add_preset(kest_context *cxt)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	kest_preset *preset = kest_alloc(sizeof(kest_preset));
	
	if (!preset)
		return ERR_ALLOC_FAIL;
	
	init_m_preset(preset);
	
	kest_preset_pll *nl = kest_preset_pll_append(cxt->presets, preset);
	
	if (!nl)
	{
		free_preset(preset);
		return ERR_ALLOC_FAIL;
	}
	
	cxt->presets = nl;
	
	cxt->n_presets++;
	
	return NO_ERROR;
}

kest_preset *kest_context_add_preset_rp(kest_context *cxt)
{
	if (!cxt)
		return NULL;
	
	KEST_PRINTF("kest_context_add_preset_rp\n");
	
	kest_preset *preset = kest_alloc(sizeof(kest_preset));
	
	if (!preset)
		return NULL;
	
	KEST_PRINTF("preset = %p\n", preset);
	
	init_m_preset(preset);
	
	KEST_PRINTF("preset->name = %p\n", preset->name);
	KEST_PRINTF("\t\t\t= %s\n", preset->name ? preset->name : "(NULL)");
	
	kest_preset_pll *nl = kest_preset_pll_append(cxt->presets, preset);
	
	if (!nl)
	{
		free_preset(preset);
		return NULL;
	}
	
	cxt->presets = nl;
	
	cxt->n_presets++;
	
	return preset;
}

kest_sequence *kest_context_add_sequence_rp(kest_context *cxt)
{
	if (!cxt)
		return NULL;
	
	kest_sequence *sequence = kest_alloc(sizeof(kest_sequence));
	
	if (!sequence)
		return NULL;
	
	init_m_sequence(sequence);
	
	sequence_ll *nl = kest_sequence_pll_append(cxt->sequences, sequence);
	
	if (!nl)
	{
		free_sequence(sequence);
		return NULL;
	}
	
	cxt->sequences = nl;
	
	return sequence;
}

kest_preset *cxt_get_preset_by_id(kest_context *cxt, uint16_t preset_id)
{
	KEST_PRINTF("cxt_get_preset_by_id(cxt = %p, preset_id = %d)\n", cxt, preset_id);
	
	if (!cxt)
		return NULL;
	
	kest_preset_pll *current = cxt->presets;
	
	KEST_PRINTF("cxt->presets = %p\n", current);
	
	while (current)
	{
		KEST_PRINTF("Consider preset at %p.\n", current->data);
		
		if (!current->data)
		{
			KEST_PRINTF("It is NULL; ignoring\n");
		}
		else 
		{
			KEST_PRINTF("It has ID %d; we are looking for ID %d\n", current->data->id, preset_id);
		}
			
		if (current->data && current->data->id == preset_id)
		{
			return current->data;
		}
		
		current = current->next;
	}
	
	return NULL;
}

kest_effect *cxt_get_effect_by_id(kest_context *cxt, uint16_t preset_id, uint16_t effect_id)
{
	KEST_PRINTF("cxt_get_effect_by_id(cxt = %p, preset_id = %d, effect_id = %d)\n", cxt, preset_id, effect_id);
	if (!cxt)
		return NULL;
	
	kest_preset *preset = cxt_get_preset_by_id(cxt, preset_id);
	
	if (!preset)
	{
		KEST_PRINTF("preset = NULL !\n");
		return NULL;
	}
	
	
	kest_effect_pll *current = preset->pipeline.effects;
	
	while (current)
	{
		if (current->data && current->data->id == effect_id)
		{
			return current->data;
		}
		
		current = current->next;
	}
	
	return NULL;
}

kest_parameter *cxt_get_parameter_by_id(kest_context *cxt, uint16_t preset_id, uint16_t effect_id, uint16_t parameter_id)
{
	if (!cxt)
		return NULL;
	
	kest_effect *effect = cxt_get_effect_by_id(cxt, preset_id, effect_id);
	
	if (!effect)
		return NULL;
	
	return effect_get_parameter(effect, parameter_id);
}	

int cxt_get_parameter_and_effect_by_id(kest_context *cxt, kest_parameter_id id, kest_parameter **pp, kest_effect **tp)
{
	KEST_PRINTF("cxt_get_parameter_and_effect_by_id(cxt = %p, id = %d.%d.%d, pp = %p, tp = %p)\n",
		cxt, id.preset_id, id.effect_id, id.parameter_id, pp, tp);
	if (!cxt || !pp || !tp)
		return ERR_NULL_PTR;
	
	*pp = NULL;
	*tp = NULL;
	
	if (id.preset_id == CONTEXT_PRESET_ID)
	{
		KEST_PRINTF("id.preset_id = CONTEXT_PRESET_ID (%d = %d)\n", id.preset_id, CONTEXT_PRESET_ID);
		if (id.effect_id == 0)
		{
			switch (id.parameter_id)
			{
				case INPUT_GAIN_PID:
					*pp = &global_cxt.input_gain;
					return NO_ERROR;
					
				case OUTPUT_GAIN_PID:
					*pp = &global_cxt.output_gain;
					return NO_ERROR;
				
				default:
					return ERR_BAD_ARGS;
			}
		}
		
		return ERR_BAD_ARGS;
	}
	
	kest_effect *effect = cxt_get_effect_by_id(cxt, id.preset_id, id.effect_id);
	
	
	if (!effect)
	{
		KEST_PRINTF("effect = NULL !\n");
		return ERR_BAD_ARGS;
	}

	kest_parameter *param = effect_get_parameter(effect, id.parameter_id);
	
	if (!param)
	{
		KEST_PRINTF("param = NULL !\n");
		return ERR_BAD_ARGS;
	}
	
	*pp = param;
	*tp = effect;
	
	return NO_ERROR;
}

kest_setting *cxt_get_setting_by_id(kest_context *cxt, uint16_t preset_id, uint16_t effect_id, uint16_t parameter_id)
{
	if (!cxt)
		return NULL;
	
	kest_effect *effect = cxt_get_effect_by_id(cxt, preset_id, effect_id);
	
	if (!effect)
		return NULL;
	
	return effect_get_setting(effect, parameter_id);
}

int cxt_effect_id_to_position(kest_context *cxt, uint16_t preset_id, uint16_t effect_id)
{
	if (!cxt)
		return -ERR_NULL_PTR;
	
	kest_preset *preset = cxt_get_preset_by_id(cxt, preset_id);
	
	if (!preset)
		return -ERR_INVALID_PRESET_ID;
	
	kest_effect_pll *current = preset->pipeline.effects;
	
	int i = 0;
	while (current)
	{
		if (current->data && current->data->id == effect_id)
			return i;
		
		current = current->next;
		i++;
	}
	
	return -ERR_INVALID_EFFECT_ID;
}

int cxt_effect_position_to_id(kest_context *cxt, uint16_t preset_id, uint16_t effect_pos)
{
	if (!cxt)
		return -ERR_NULL_PTR;
	
	kest_preset *preset = cxt_get_preset_by_id(cxt, preset_id);
	
	if (!preset)
		return -ERR_INVALID_PRESET_ID;
	
	kest_effect_pll *current = preset->pipeline.effects;
	
	int i = 0;
	while (current)
	{
		if (i == effect_pos)
		{
			if (!current->data)
				return ERR_NULL_PTR;
			
			return current->data->id;
		}
		
		current = current->next;
		i++;
	}
	
	return -ERR_INVALID_EFFECT_ID;
}

int cxt_remove_preset(kest_context *cxt, kest_preset *preset)
{
	if (!cxt || !preset)
		return ERR_NULL_PTR;
	
	kest_preset_pll *current = cxt->presets;
	kest_preset_pll *prev = NULL;
	
	if (preset && preset->has_fname)
		remove(preset->fname);
	
	while (current)
	{
		if (current->data == preset)
		{
			#ifdef USE_TEENSY
			queue_msg_to_teensy(create_m_message(KEST_MESSAGE_DELETE_PRESET, "s", preset->id));
			#endif
			
			if (!prev)
			{
				cxt->presets = current->next;
			}
			else
			{
				prev->next = current->next;
			}
			
			free_preset(preset);
			kest_free(current);
			
			return NO_ERROR;
		}
		
		prev = current;
		current = current->next;
	}
	
	return ERR_INVALID_PRESET_ID;
}

int cxt_remove_sequence(kest_context *cxt, kest_sequence *sequence)
{
	if (!cxt || !sequence)
		return ERR_NULL_PTR;
	
	sequence_ll *current = cxt->sequences;
	sequence_ll *prev = NULL;
	
	if (sequence && sequence->has_fname)
		remove(sequence->fname);
	
	while (current)
	{
		if (current->data == sequence)
		{
			if (!prev)
			{
				cxt->sequences = current->next;
			}
			else
			{
				prev->next = current->next;
			}
			
			free_sequence(sequence);
			kest_free(current);
			
			return NO_ERROR;
		}
		
		prev = current;
		current = current->next;
	}
	
	return ERR_INVALID_PRESET_ID;
}

int cxt_remove_effect(kest_context *cxt, uint16_t pid, uint16_t tid)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	int ret_val = kest_preset_remove_effect(cxt_get_preset_by_id(cxt, pid), tid);
	
	if (ret_val == NO_ERROR)
	{
		#ifdef USE_TEENSY
		queue_msg_to_teensy(create_m_message(KEST_MESSAGE_REMOVE_EFFECT, "ss", pid, tid));
		#endif
	}
	
	return ret_val;
}

int set_active_preset(kest_preset *preset)
{
	if (preset)
		kest_preset_set_active(preset);
	
	if (preset == global_cxt.active_preset)
		return NO_ERROR;
	
	if (preset && preset->sequence)
	{
		KEST_PRINTF("preset has a sequence. call kest_sequence_activate_at\n");
		kest_sequence_activate_at(preset->sequence, preset);
	}
	
	kest_preset_set_inactive(global_cxt.active_preset);
	
	global_cxt.active_preset = preset;
	
	uint16_t id = preset ? preset->id : 0;
	
	return NO_ERROR;
}

int activate_active_preset_dma()
{
	KEST_PRINTF("activate_active_preset_dma\n");
	return kest_preset_activate_dma(global_cxt.active_preset);
}

int activate_active_preset_lfos()
{
	KEST_PRINTF("activate_active_preset_lfos\n");
	return kest_preset_activate_lfos(global_cxt.active_preset);
}

// This version is called from a sequence-related-cb, so there is no need to
// tell the sequence about it; it is handled from the caller
int set_active_preset_from_sequence(kest_preset *preset)
{
	KEST_PRINTF("set_active_preset_from_sequence, preset = %p\n", preset);
	if (preset)
		kest_preset_set_active(preset);
	
	if (preset == global_cxt.active_preset)
		return NO_ERROR;
	
	kest_preset_set_inactive(global_cxt.active_preset);
	
	global_cxt.active_preset = preset;
	
	int ret_val = NO_ERROR;
	
	kest_queue_state_save();
	
	return ret_val;
}

int set_working_preset(kest_preset *preset)
{
	if (!preset)
		return ERR_NULL_PTR;
	
	if (preset == global_cxt.working_preset)
		return NO_ERROR;
	
	global_cxt.working_preset = preset;
	
	return NO_ERROR;
}

void context_print_presets(kest_context *cxt)
{
	if (!cxt)
		return;
		
	KEST_PRINTF("Printing presets...\n");
	
	kest_preset_pll *current = global_cxt.presets;
	
	
	int i = 0;
	while (current)
	{
		KEST_PRINTF("Preset %d, stored at %p, ", i, current->data);
		
		if (current->data)
		{
			int j = 0;
			kest_effect_pll *ct = current->data->pipeline.effects;
			
			while (ct)
			{
				ct = ct->next;
				j++;
			}
			KEST_PRINTF("has name %s, and has %d effects%s", current->data->name ? current->data->name : "(NULL)", j, (j > 0) ? ", which are\n" : "\n\n");
			
			ct = current->data->pipeline.effects;
			
			while (ct)
			{
				KEST_PRINTF("\t%s,\n", (ct->data && kest_effect_name(ct->data)) ? kest_effect_name(ct->data) : "UNKNOWN");
				ct = ct->next;
			}
		}
		current = current->next;
		i++;
	}
	
	if (i == 0)
	{
		KEST_PRINTF("There are none!\n");
	}
}

int cxt_set_all_presets_left_button_to_main_menu(kest_context *cxt)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	kest_preset_pll *current = cxt->presets;
	
	while (current)
	{
		if (current->data && current->data->view_page)
		{
			//preset_view_set_left_button_mode(current->data->view_page, LEFT_BUTTON_MENU);
		}
		
		current = current->next;
	}
	
	return NO_ERROR;
}

int cxt_handle_hw_switch(kest_context *cxt, int sw)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("cxt_handle_hw_switch, sw = %d\n", sw);
	
	if (cxt->sequence)
	{
		if (sw == 0)
			kest_sequence_regress(cxt->sequence);
		else if (sw == 1)
			kest_sequence_advance(cxt->sequence);
	}
	
	return NO_ERROR;
}

kest_preset *cxt_find_preset(kest_context *cxt, const char *fname)
{
	if (!cxt)
		return NULL;
	
	kest_preset_pll *current = cxt->presets;
	
	KEST_PRINTF("Searching for preset with fname %s...\n", fname);
	while (current)
	{
		if (current->data && current->data->has_fname)
		{
			KEST_PRINTF("Check %s\n", current->data->fname);
			if (strncmp(current->data->fname, fname, PRESET_NAME_MAX_LEN) == 0)
			{
				KEST_PRINTF("Match!\n");
				return current->data;
			}
			KEST_PRINTF("No match\n");
		}
		
		current = current->next;
	}
	
	return NULL;
}

int cxt_save_all_presets(kest_context *cxt)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	kest_preset_pll *current = cxt->presets;
	
	while (current)
	{
		if (current->data)
		{
			kest_preset_save(current->data);
		}
		
		current = current->next;
	}
	
	return NO_ERROR;
}

int kest_cxt_obtain_mutex(kest_context *cxt)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	#ifdef KEST_USE_FREERTOS
	if (xSemaphoreTake(cxt->mutex, 0) != pdTRUE) return ERR_MUTEX_UNAVAILABLE;
	#else
	return ERR_FEATURE_DISABLED;
	#endif
	
	return NO_ERROR;
}

int kest_cxt_obtain_mutex_wait_forever(kest_context *cxt)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	#ifdef KEST_USE_FREERTOS
	if (xSemaphoreTake(cxt->mutex, portMAX_DELAY) != pdTRUE)
		return ERR_MUTEX_UNAVAILABLE;
	#else
	return ERR_FEATURE_DISABLED;
	#endif
	
	return NO_ERROR;
}


int kest_cxt_release_mutex(kest_context *cxt)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	#ifdef KEST_USE_FREERTOS
	xSemaphoreGive(cxt->mutex);
	#else
	return ERR_FEATURE_DISABLED;
	#endif
	
	return NO_ERROR;
}

int kest_cxt_queue_save_state(kest_context *cxt)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	#ifdef KEST_ENABLE_REPRESENTATIONS
	queue_representation_list_update(&cxt->state_rep_lstub);
	#endif
	
	return NO_ERROR;
}

kest_preset *cxt_get_preset_by_fname(kest_context *cxt, const char *fname)
{
	KEST_PRINTF("cxt_get_preset_by_fname(cxt = %p, fname = %s)\n", cxt, fname ? fname : "(NULL)");
	
	if (!cxt || !fname)
		return NULL;
	
	kest_preset_pll *current = cxt->presets;
	
	KEST_PRINTF("Searching the list. %s\n", !current ? "... but it is empty!" : "");
	int i = 0;
	while (current)
	{
		KEST_PRINTF("Preset %d ", i);
		if (current->data)
		{
			KEST_PRINTF("has fname \"%s\".\n", current->data->has_fname ? current->data->fname : "(NULL)");
		}
		else
		{
			KEST_PRINTF("... doesn't exist???\n");
		}
		if (current->data && current->data->has_fname && fnames_agree(current->data->fname, fname))
		{
			KEST_PRINTF("That's the one :) returning it\n");
			return current->data;
		}
		
		current = current->next;
		i++;
	}
	
	return NULL;
}

kest_sequence *cxt_get_sequence_by_fname(kest_context *cxt, const char *fname)
{
	if (!cxt || !fname)
		return NULL;
	
	kest_sequence_pll *current = cxt->sequences;
	
	while (current)
	{
		if (current->data && current->data->has_fname && fnames_agree(current->data->fname, fname))
			return current->data;
		
		current = current->next;
	}
	
	return NULL;
}

int kest_cxt_set_input_gain(kest_context *cxt, float gain)
{
	KEST_PRINTF("kest_cxt_set_input_gain\n");
	
	if (!cxt) return ERR_NULL_PTR;
	
	int ret_val = kest_parameter_trigger_update(&cxt->input_gain, gain);
	
	KEST_PRINTF("kest_cxt_set_input_gain done\n");
	return ret_val;
}

int kest_cxt_set_output_gain(kest_context *cxt, float gain)
{
	KEST_PRINTF("kest_cxt_set_output_gain\n");
	if (!cxt) return ERR_NULL_PTR;
	
	int ret_val = kest_parameter_trigger_update(&cxt->output_gain, gain);
	
	KEST_PRINTF("kest_cxt_set_input_gain done\n");
	return ret_val;
}

kest_effect_desc *kest_cxt_get_effect_desc_from_cname(kest_context *cxt, const char *cname)
{
	if (!cxt || !cname)
		return NULL;
	
	kest_effect_desc_pll *current = cxt->effects;
	
	while (current)
	{
		if (current->data && current->data->cname && strcmp(current->data->cname, cname) == 0)
			return current->data;
		
		current = current->next;
	}
	
	return NULL;
}

int kest_cxt_get_sequence_count(kest_context *cxt)
{
	if (!cxt)
		return 0;
	
	kest_sequence_pll *cs = cxt->sequences;
	int i = 0;
	
	while (cs)
	{
		cs = cs->next;
		i++;
	}
	
	return i;
}

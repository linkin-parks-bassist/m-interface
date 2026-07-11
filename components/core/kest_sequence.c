#include "kest_int.h"

#define PRINTLINES_ALLOWED 0

static const char *FNAME = "kest_sequence.c";

IMPLEMENT_LINKED_PTR_LIST(kest_sequence);

IMPLEMENT_POOL(kest_sequence);
kest_allocator kest_sequence_allocator;
kest_sequence_pool kest_sequence_mem_pool;

int init_m_sequence(kest_sequence *sequence)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	sequence->name 	  = NULL;
	sequence->presets = NULL;
	
	sequence->active = 0;
	sequence->unsaved_changes = 1;
	sequence->position = NULL;
	
	#ifdef KEST_ENABLE_UI
	sequence->view_page = NULL;
	#endif
	
	sequence->fname[0] = 0;
	sequence->has_fname = 0;
	
	sequence->listings = NULL;
	
	sequence->main_sequence = 0;
	
	return NO_ERROR;
}

int kest_sequence_set_default_name(kest_sequence *sequence, uint32_t n)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	// 24 bytes is safe here.
	// 10 for "Sequence", plus space, plus null-terminator.
	// + 10 for maximum number of digits in a uint32_t
	// + another 4 bc paranoid, but keep it aligned to 32 bits
	sequence->name = kest_alloc(24);
	
	if (!sequence->name)
		return ERR_ALLOC_FAIL;
	
	sprintf(sequence->name, "Sequence %lu", n);
	
	return NO_ERROR;
}

int sequence_append_preset(kest_sequence *sequence, kest_preset *preset)
{
	if (!sequence || !preset)
		return ERR_NULL_PTR;
	
	seq_kest_preset_pll *new_node = kest_alloc(sizeof(seq_kest_preset_pll));
	
	if (!new_node)
		return ERR_ALLOC_FAIL;
	
	preset->sequence = sequence;
	
	new_node->data = preset;
	new_node->next = NULL;
	new_node->prev = NULL;
	
	if (!sequence->presets)
	{
		sequence->presets = new_node;
		return NO_ERROR;
	}
	
	seq_kest_preset_pll *current = sequence->presets;
	
	while (current)
	{
		if (!current->next)
			break;
		current = current->next;
	}
	
	current->next = new_node;
	new_node->prev = current;
	
	preset->sequence = sequence;
	
	kest_queue_sequence_save(sequence);
	
	return NO_ERROR;
}


seq_kest_preset_pll *sequence_append_preset_rp(kest_sequence *sequence, kest_preset *preset)
{
	KEST_PRINTF("sequence_append_preset_rp, line %d\n", __LINE__);
	
	if (!sequence || !preset)
		return NULL;
	
	
	KEST_PRINTF("sequence_append_preset_rp, line %d\n", __LINE__);
	seq_kest_preset_pll *new_node = kest_alloc(sizeof(seq_kest_preset_pll));
	
	if (!new_node)
		return NULL;
	
	KEST_PRINTF("sequence_append_preset_rp, line %d\n", __LINE__);
	new_node->data = preset;
	new_node->next = NULL;
	new_node->prev = NULL;
	
	
	KEST_PRINTF("sequence_append_preset_rp, line %d\n", __LINE__);
	if (!sequence->presets)
	{
		sequence->presets = new_node;
	}
	else
	{
	
		KEST_PRINTF("sequence_append_preset_rp, line %d\n", __LINE__);
		
		seq_kest_preset_pll *current = sequence->presets;
		
		
		KEST_PRINTF("sequence_append_preset_rp, line %d\n", __LINE__);
		while (current)
		{
			if (!current->next)
				break;
			current = current->next;
		}
		
		
		KEST_PRINTF("sequence_append_preset_rp, line %d\n", __LINE__);
		current->next = new_node;
		new_node->prev = current;
	}
	
	
	
	KEST_PRINTF("sequence_append_preset_rp, line %d\n", __LINE__);
	kest_queue_sequence_save(sequence);
	
	KEST_PRINTF("sequence_append_preset_rp, line %d\n", __LINE__);
	
	return new_node;
}

int kest_sequence_move_preset(kest_sequence *sequence, int pos, int new_pos)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	if (pos < 0 || new_pos < 0)
		return ERR_BAD_ARGS;
	
	seq_kest_preset_pll *target = sequence->presets;
	seq_kest_preset_pll *prev = NULL;
	
	for (int i = 0; i < pos; i++)
	{
		prev = target;
		if (target)
		{
			target = target->next;
		}
	}
	
	if (!target)
		return ERR_BAD_ARGS;
	
	if (prev)
	{
		prev->next = target->next;
	}
	
	if (new_pos == 0)
	{
		target->next = sequence->presets;
		sequence->presets = target;
		return NO_ERROR;
	}
	
	seq_kest_preset_pll *current = sequence->presets;
	
	for (int i = 0; i < new_pos; i++)
	{
		prev = current;
		if (current)
			current = current->next;
	}
	
	if (prev)
	{
		target->next = prev->next;
		prev->next = target;
	}
	else
	{
		return ERR_BAD_ARGS;
	}
	
	kest_queue_sequence_save(sequence);
	
	return NO_ERROR;
}

int kest_sequence_remove_preset(kest_sequence *sequence, kest_preset *preset)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	seq_kest_preset_pll *current = sequence->presets;
	
	while (current)
	{
		if (current->data == preset)
			break;
		
		current = current->next;
	}
	
	if (current)
	{
		if (current->prev)
			current->prev->next = current->next;
		else
			sequence->presets = current->next;
		
		kest_free(current);
	}
	else
	{
		return ERR_BAD_ARGS;
	}
	
	kest_queue_sequence_save(sequence);
	
	return NO_ERROR;
}

int kest_sequence_delete_preset(kest_sequence *sequence, kest_preset *preset)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	int ret_val = kest_sequence_remove_preset(sequence, preset);
	
	if (preset && ret_val == NO_ERROR)
	{
		kest_free_preset(preset);
	}
	
	kest_queue_sequence_save(sequence);
	
	return NO_ERROR;
}

void free_sequence(kest_sequence *sequence)
{
	if (!sequence)
		return;
	kest_allocator_free(&kest_sequence_allocator, sequence);
}

int kest_sequence_add_menu_listing(kest_sequence *sequence, kest_menu_item *listing)
{
	#ifdef KEST_ENABLE_UI
	if (!sequence || !listing)
		return ERR_NULL_PTR;
	
	kest_menu_item_pll *nl = kest_menu_item_pll_append(sequence->listings, listing);
	
	if (nl)
		sequence->listings = nl;
	else
		return ERR_ALLOC_FAIL;
	
	return NO_ERROR;
	#else
	return ERR_FEATURE_DISABLED;
	#endif
}

int kest_sequence_begin(kest_sequence *sequence)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	if (!sequence->presets)
	{
		KEST_PRINTF("Sequence is empty !\n");
		return NO_ERROR;
	}
	
	global_cxt.sequence = sequence;
	sequence->active = 1;
	
	set_active_preset_from_sequence(sequence->presets->data);
	
	sequence->position = sequence->presets;
	
	return NO_ERROR;
}

int kest_sequence_begin_at(kest_sequence *sequence, kest_preset *preset)
{
	if (!sequence || !preset)
		return ERR_NULL_PTR;
	
	if (!sequence->presets)
	{
		KEST_PRINTF("Sequence is empty !\n");
		return NO_ERROR;
	}
	
	global_cxt.sequence = sequence;
	sequence->active = 1;
	
	seq_kest_preset_pll *current = sequence->presets;
	int found = 0;
	
	while (current && !found)
	{
		if (current->data == preset)
			found = 1;
		else
			current = current->next;
	}
	
	if (!found)
		return ERR_BAD_ARGS;
	
	set_active_preset_from_sequence(current->data);
	
	sequence->position = current;
	
	return NO_ERROR;
}


int kest_sequence_regress(kest_sequence *sequence)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("regressing sequence\n");
	
	if (!sequence->presets)
	{
		KEST_PRINTF("Error: empty sequence\n");
		return ERR_BAD_ARGS;
	}
	
	if (!sequence->active || !sequence->position)
	{
		KEST_PRINTF("Error: sequence not active\n");
		return ERR_BAD_ARGS;
	}
	
	if (!sequence->position->prev)
	{
		KEST_PRINTF("Can't regress sequence; sequence at start already\n");
		return NO_ERROR;
	}
	
	sequence->position = sequence->position->prev;
	
	set_active_preset_from_sequence(sequence->position->data);
	
	return NO_ERROR;
}

int kest_sequence_advance(kest_sequence *sequence)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("advancing sequence. sequence = %p\n", sequence);
	
	if (!sequence->presets)
	{
		KEST_PRINTF("Error: empty sequence\n");
		return ERR_BAD_ARGS;
	}
	
	KEST_PRINTF("Sequence nonempty\n");
	
	if (!sequence->active || !sequence->position)
	{
		KEST_PRINTF("Error: sequence not active\n");
		return ERR_BAD_ARGS;
	}
	
	KEST_PRINTF("Sequence active. Position: %p\n", sequence->position);
	
	if (!sequence->position->next)
	{
		KEST_PRINTF("Can't regress sequence; sequence at end already\n");
		return NO_ERROR;
	}
	
	KEST_PRINTF("sequence->position->next = %p\n", sequence->position->next);
	
	sequence->position = sequence->position->next;
	
	KEST_PRINTF("New sequence->position: %p. sequence->position->data: %p\n", 
		sequence->position, (sequence->position) ? sequence->position->data : NULL);
	
	return set_active_preset_from_sequence(sequence->position->data);
}

int kest_sequence_stop(kest_sequence *sequence)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	global_cxt.sequence = NULL;
	sequence->active = 0;
	
	set_active_preset_from_sequence(NULL);
	
	sequence->position = NULL;
	
	return NO_ERROR;
}


int kest_sequence_stop_from_preset(kest_sequence *sequence)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	global_cxt.sequence = NULL;
	sequence->active = 0;
	
	sequence->position = NULL;
	
	return NO_ERROR;
}

int kest_sequence_activate_at(kest_sequence *sequence, kest_preset *preset)
{
	KEST_PRINTF("kest_sequence_activate_at\n");
	if (!sequence)
		return ERR_NULL_PTR;
	
	seq_kest_preset_pll *current = sequence->presets;
	
	while (current)
	{
		if (current->data && current->data == preset)
		{
			sequence->position = current;
			sequence->active = 1;
			
			return NO_ERROR;
		}
		
		current = current->next;
	}
	
	KEST_PRINTF("kest_sequence_activate_at done\n");
	return ERR_BAD_ARGS;
}

#ifdef KEST_ENABLE_UI
int kest_sequence_handle_name_change_in_ui(kest_sequence *sequence)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	char *new_name = sequence->name;
	
	if (!new_name)
		new_name = "Sequence";
	
	hide_keyboard();
	
	kest_ui_page *sv_page = sequence->view_page;
	kest_sequence_view_str *sv_str = sv_page ? (kest_sequence_view_str*)sv_page->data_struct : NULL;
	
	if (sv_page)
	{
		if (sv_page->panel && sv_page->panel->title)
			lv_obj_clear_state(sv_page->panel->title, LV_STATE_FOCUSED);
		if (sv_page->container)
			lv_obj_add_state(sv_page->container, LV_STATE_FOCUSED);
	}
	
	if (sv_str && sv_str->menu_item)
		sequence_listing_menu_item_change_name(sv_str->menu_item, sequence->name);
	
	return NO_ERROR;
}

void kest_sequence_handle_name_change_in_ui_async_wrapper(void *sequence)
{
	kest_sequence_handle_name_change_in_ui(sequence);
}
#endif

int kest_sequence_handle_name_change(kest_sequence *sequence)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	#ifdef KEST_ENABLE_UI
	kest_ui_async_call(kest_sequence_handle_name_change_in_ui_async_wrapper, (void*)sequence);
	#endif
	
	kest_queue_sequence_save(sequence);
	
	return NO_ERROR;
}

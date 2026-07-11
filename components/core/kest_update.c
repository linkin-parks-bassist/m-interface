#include "kest_int.h"

#define PRINTLINES_ALLOWED 0

static const char *FNAME = "kest_update.c";

#define UPDATE_RATE_HZ 100
#define UPDATE_PERIOD_MS (1000.0f / (float)UPDATE_RATE_HZ)

#ifdef KEST_USE_FREERTOS
QueueHandle_t update_queue_ = NULL;
static const int update_period_ticks = (pdMS_TO_TICKS((int)UPDATE_PERIOD_MS) == 0) ? 1 : pdMS_TO_TICKS((int)UPDATE_PERIOD_MS);
#endif

IMPLEMENT_LIST(kest_update);
IMPLEMENT_LIST(kest_fpga_write);
IMPLEMENT_LIST(kest_fpga_alloc);
IMPLEMENT_LIST(kest_fpga_mem_read);

int kest_updater_state_init(kest_updater_state *state)
{
	if (!state)
		return ERR_NULL_PTR;
	
	state->state = KEST_UPDATER_STATE_READY;
	
	#ifdef KEST_LIBRARY
	state->active_preset = NULL;
	#else
	state->active_preset = global_cxt.active_preset;
	#endif
	
	kest_update_list_init(&state->updates);
	
	kest_fpga_command_list_init(&state->cmds);
	
	kest_fpga_alloc_list_init(&state->allocs);
	
	kest_fpga_write_list_init(&state->instr_writes);
	kest_fpga_write_list_init(&state->reg_writes);
	kest_fpga_write_list_init(&state->filter_writes);
	
	kest_fpga_mem_read_list_init(&state->reads);
	
	kest_dsp_resource_ptr_list_init(&state->resources);
	
	kest_fpga_transfer_batch_init(&state->batch);
	state->batch.buffer_owned = 1;
	
	state->tick_ctr = 0;
	
	return NO_ERROR;
}

int kest_updater_drain_lists(kest_updater_state *state)
{
	if (!state)
		return ERR_NULL_PTR;
	
	kest_update_list_drain(&state->updates);
	
	kest_fpga_batch_drain(&state->batch);
	kest_fpga_command_list_drain(&state->cmds);
	
	kest_fpga_alloc_list_drain(&state->allocs);
	kest_fpga_write_list_drain(&state->instr_writes);
	kest_fpga_write_list_drain(&state->reg_writes);
	kest_fpga_write_list_drain(&state->filter_writes);
	
	return NO_ERROR;
}

int kest_updater_clear(kest_updater_state *state)
{
	if (!state)
		return ERR_NULL_PTR;
	
	kest_updater_drain_lists(state);
	kest_fpga_mem_read_list_drain(&state->reads);
	
	kest_dsp_resource_ptr_list_drain(&state->resources);
	
	return NO_ERROR;
}

void kest_updater_state_destroy(kest_updater_state *state)
{
	if (!state)
		return;
	
	kest_update_list_destroy(&state->updates);
	kest_fpga_command_list_destroy(&state->cmds);
	
	kest_fpga_alloc_list_destroy(&state->allocs);
	
	kest_fpga_write_list_destroy(&state->instr_writes);
	kest_fpga_write_list_destroy(&state->reg_writes);
	kest_fpga_write_list_destroy(&state->filter_writes);
}

int kest_update_task_start()
{
	#ifdef KEST_USE_FREERTOS
	KEST_PRINTF("kest_update_task_start\n");
	update_queue_ = xQueueCreate(16, sizeof(kest_update));
	
	if (!update_queue_)
		return ERR_UNKNOWN_ERR;
	
	xTaskCreate(kest_update_task, "kest_update_task", 4096, NULL, 8, NULL);
	
	return NO_ERROR;
	#else
	return ERR_FEATURE_DISABLED;
	#endif
}

void kest_update_print(kest_update update)
{
	switch (update.type)
	{
		case KEST_UPDATE_PARAM:
			if (update.data.param)
			{
				KEST_PRINTF_FORCE_("KEST_UPDATE_PARAM(param = \"%s\" (\"%s\") = %p)\n", update.data.param->name, update.data.param->name_internal, update.data.param);
			}
			else
			{
				KEST_PRINTF_FORCE_("KEST_UPDATE_PARAM(param = NULL)\n");
			}
			break;
			
		case KEST_UPDATE_PRESET:
			KEST_PRINTF_FORCE_("KEST_UPDATE_PRESET(preset = %p)\n", update.data.preset);
			break;
		
		case KEST_UPDATE_MEM:
			KEST_PRINTF_FORCE_("KEST_UPDATE_MEM\n");
			break;
		
		case KEST_UPDATE_SCOPE_ENTRY:
			KEST_PRINTF_FORCE_("KEST_UPDATE_SCOPE_ENTRY \"%s\" in %p\n", update.data.scope_entry.key, update.data.scope_entry.effect->scope);
			break;
	}
}

void kest_update_task(void *arg)
{
	kest_updater_state state;
	kest_updater_state_init(&state);
	
	kest_update update;
	
	#ifdef KEST_USE_FREERTOS
	TickType_t last_wake = xTaskGetTickCount();
	#endif
	
	while (1)
	{
		#ifdef KEST_USE_FREERTOS
		while (xQueueReceive(update_queue_, &update, 0) == pdPASS)
		{
			#ifdef PRINT_UPDATES
			KEST_PRINTF_FORCE_("Update: ");
			kest_update_print(update);
			#endif
			if (update.type == KEST_UPDATE_PRESET)
				kest_updater_drain_lists(&state);
			
			kest_update_list_append(&state.updates, update);
		}
		#endif
		
		for (size_t i = 0; i < state.updates.count; i++)
			kest_updater_handle_update(&state, state.updates.entries[i]);
		
		kest_updater_handle_resource_updates(&state);
		
		#ifdef PRINT_ALLOCS
		kest_updater_print_allocs(&state);
		#endif
		#ifdef PRINT_INSTR_WRITES
		kest_updater_print_instr_writes(&state);
		#endif
		#ifdef PRINT_REG_WRITES
		kest_updater_print_reg_writes(&state);
		#endif
		#ifdef PRINT_FILTER_WRITES
		kest_updater_print_filter_writes(&state);
		#endif
		
		kest_updater_generate_command_list(&state);
		
		#ifdef PRINT_COMMAND_LIST
		kest_updater_print_command_list(&state);
		#endif
		
		kest_updater_generate_tx_batch(&state);
		kest_updater_send(&state);
		
		kest_updater_drain_lists(&state);
		
		state.tick_ctr++;
		
		#ifdef KEST_USE_FREERTOS
		xTaskDelayUntil(&last_wake, update_period_ticks);
		#endif
	}
	
	#ifdef KEST_USE_FREERTOS
	vTaskDelete(NULL);
	#endif
}

int kest_fpga_write_generate_update(kest_fpga_write *write, kest_dependent *dep, kest_effect *effect)
{
	KEST_PRINTF("kest_fpga_write_generate_update(write = %p, dep = %p, effect = %p)\n",
		write, dep, effect);
	
	if (!write || !dep || !effect)
		return ERR_NULL_PTR;
	
	kest_scope *scope = effect->scope;
	kest_expression *expr = NULL;
	kest_block_reg_val *reg_val = NULL;
	kest_block *block = NULL;
	kest_dsp_resource *res = NULL;
	kest_filter *filter = NULL;
	int res_found = 0;
	
	int addr_1_local = 0;
	
	if (dep->type == KEST_DEPENDENT_BLOCK_REG)
	{
		KEST_PRINTF("dep->type == KEST_DEPENDENT_BLOCK_REG\n");
		write->type = KEST_BLOCK_REG_UPDATE;
		
		addr_1_local = dep->data.block_reg.block;
		
		if (addr_1_local < 0 || addr_1_local >= effect->blocks.count || !(dep->data.block_reg.reg == 0 || dep->data.block_reg.reg == 1))
			return ERR_BAD_ARGS;
		
		if (dep->data.block_reg.reg == 0)
			reg_val = &effect->blocks.entries[addr_1_local].reg_0;
		else if (dep->data.block_reg.reg == 1)
			reg_val = &effect->blocks.entries[addr_1_local].reg_1;
		
		if (!reg_val || !reg_val->expr)
			return ERR_BAD_ARGS;
		
		write->addr_1 = kest_effect_fpga_position_resolve_block(&effect->position_, addr_1_local);
		write->addr_2 = dep->data.block_reg.reg;
		write->format = reg_val->format;
		write->expr = reg_val->expr;
		write->scope = scope;
		
	}
	else if (dep->type == KEST_DEPENDENT_FILTER_COEF)
	{
		KEST_PRINTF("dep->type == KEST_DEPENDENT_FILTER_COEF\n");
		
		write->type = KEST_FILTER_COEF_UPDATE;
		
		addr_1_local = dep->data.filter_coef.filter;
		
		KEST_PRINTF("Searching for filter with handle %d. Effect has %d resources\n", addr_1_local, effect->resources.count);
		
		res_found = 0;
		for (int i = 0; !res_found && i < effect->resources.count; i++)
		{
			res = effect->resources.entries[i];
			
			KEST_PRINTF("effect->resources.entries[%d] = %p\n", i, effect->resources.entries[i]);
			
			if (!res)
			{
				continue;
			}
			
			KEST_PRINTF("Resource %d has type %s, handle %d\n", i, kest_dsp_resource_type_to_string(res->type), res->handle);
			
			if (res->type == KEST_DSP_RESOURCE_FILTER && res->handle == addr_1_local)
				res_found = 1;
		}
		
		if (!res || !res_found)
		{
			KEST_PRINTF("res = %p, res_found = %d\n", res, res_found);
			return ERR_BAD_ARGS;
		}
		
		filter = res->data;
		
		if (!filter)
			return ERR_BAD_ARGS;
		
		if (dep->data.filter_coef.coef < 0 || dep->data.filter_coef.coef >= filter->coefs.count)
			return ERR_BAD_ARGS;
		
		expr = filter->coefs.entries[dep->data.filter_coef.coef];
		
		if (!expr)
			return ERR_BAD_ARGS;
		
		write->addr_1 = kest_effect_fpga_position_resolve_filter(&effect->position_, addr_1_local);
		write->addr_2 = dep->data.filter_coef.coef;
		
		write->format = filter->format;
		write->expr   = expr;
		write->scope  = scope;
	}
	else
	{
		return ERR_BAD_ARGS;
	}
	
	KEST_PRINTF("kest_fpga_write_generate_update done with no error\n");
	return NO_ERROR;
}

int kest_updater_add_reg_update(kest_updater_state *state, kest_dependent *dep, kest_effect *effect)
{
	if (!state || !dep || !effect)
		return ERR_NULL_PTR;
	
	kest_fpga_write write;
	
	int ret_val = kest_fpga_write_generate_update(&write, dep, effect);
	
	if (ret_val != NO_ERROR)
	{
		KEST_PRINTF_FORCE("Error: %s\n", kest_error_code_to_string(ret_val));
		return ret_val;
	}
	
	for (int i = 0; i < state->reg_writes.count; i++)
	{
		if (state->reg_writes.entries[i].addr_1 == write.addr_1 && state->reg_writes.entries[i].addr_2 == write.addr_2)
			return NO_ERROR;
	}
	
	ret_val = kest_fpga_write_list_append(&state->reg_writes, write);
	
	return ret_val;
}

int kest_updater_add_filter_coef_update(kest_updater_state *state, kest_dependent *dep, kest_effect *effect)
{
	if (!state || !dep || !effect)
		return ERR_NULL_PTR;
	
	kest_fpga_write write;
	
	int ret_val = kest_fpga_write_generate_update(&write, dep, effect);
	
	if (ret_val != NO_ERROR)
	{
		KEST_PRINTF_FORCE("Error: %s\n", kest_error_code_to_string(ret_val));
		return ret_val;
	}
	
	for (int i = 0; i < state->filter_writes.count; i++)
	{
		if (state->filter_writes.entries[i].addr_1 == write.addr_1 && state->filter_writes.entries[i].addr_2 == write.addr_2)
			return NO_ERROR;
	}
	
	ret_val = kest_fpga_write_list_append(&state->filter_writes, write);
	
	return ret_val;
}

void kest_updater_print_reg_writes(kest_updater_state *state)
{
	if (!state)
		return;
	
	if (state->reg_writes.count == 0)
		return;
	
	kest_fpga_write_list *writes = &state->reg_writes;
	kest_fpga_write write;
	
	KEST_PRINTF_FORCE("Updater tick %d register writes (n = %d):\n", state->tick_ctr, state->reg_writes.count);
	
	float val;
	
	for (size_t i = 0; i < writes->count; i++)
	{
		write = writes->entries[i];
		
		val = kest_expression_evaluate(write.expr, write.scope);
		KEST_PRINTF_FORCE_("\tBlock %d reg %d, format %d, scope %p, value %s%.04f = %s\n", write.addr_1, write.addr_2, write.format,
			write.scope, val < 0 ? "" : " ", val, kest_expression_to_string(write.expr));
	}
}

void kest_updater_print_filter_writes(kest_updater_state *state)
{
	if (!state)
		return;
		
	if (state->filter_writes.count == 0)
		return;
	
	kest_fpga_write_list *writes = &state->filter_writes;
	kest_fpga_write write;
	
	KEST_PRINTF_FORCE("Updater tick %d filter writes (n = %d):\n", state->tick_ctr, state->filter_writes.count);
	
	float val;
	
	for (size_t i = 0; i < writes->count; i++)
	{
		write = writes->entries[i];
		
		val = kest_expression_evaluate(write.expr, write.scope);
		KEST_PRINTF_FORCE_("\tFilter %d coef %d, format %d, scope %p, value %s%.04f = %s\n", write.addr_1, write.addr_2, write.format,
			write.scope, val < 0 ? "" : " ", val, kest_expression_to_string(write.expr));
	}
}

void kest_updater_print_command_list(kest_updater_state *state)
{
	if (!state)
		return;
	
	if (state->cmds.count == 0)
		return;
	
	kest_fpga_command cmd;
	
	kest_string str;
	kest_string_init(&str);
	
	KEST_PRINTF_FORCE("Updater tick %d commands (n = %d):\n", state->tick_ctr, state->cmds.count);
	
	for (size_t i = 0; i < state->cmds.count; i++)
	{
		cmd = state->cmds.entries[i];
		
		KEST_PRINTF_FORCE_("\tCommand %d: ", i);
		kest_fpga_command_to_string_(cmd, &str);
		KEST_PUTS_FORCE(str);
		kest_string_drain(&str);
		KEST_PRINTF_FORCE_("\n");
	}
}

void kest_updater_print_instr_writes(kest_updater_state *state)
{
	
}

void kest_updater_print_allocs(kest_updater_state *state)
{
	if (!state)
		return;
	
	if (state->allocs.count == 0)
		return;
	
	kest_fpga_alloc alloc;
	
	KEST_PRINTF("Updater tick %d commands (n = %d):\n", state->tick_ctr, state->allocs.count);
	
	for (size_t i = 0; i < state->allocs.count; i++)
	{
		alloc = state->allocs.entries[i];
		
		KEST_PRINTF_("\tAlloc %d: ", i);
		
		switch (alloc.type)
		{
			case KEST_ALLOC_TYPE_DELAY:
				KEST_PRINTF_("Delay (size: %d samples (%.01fms), delay: %d samples (%.01fms))\n",
					alloc.size_1, ((float)alloc.size_1 / (float)KEST_FPGA_SAMPLE_RATE) * 1000.0f,
					alloc.size_2, ((float)alloc.size_2 / (float)KEST_FPGA_SAMPLE_RATE) * 1000.0f);
				break;
			case KEST_ALLOC_TYPE_FILTER:
				KEST_PRINTF_("Filter (order: %d, feed_forward: %d, feed_back: %d, format %d)\n",
					alloc.size_1 + alloc.size_2, alloc.size_1, alloc.size_2, alloc.format);
				break;
		}
	}
}

#define PRINTLINES_ALLOWED 0

void mem_read_callback(kest_fpga_sample_t result, void *arg)
{
	kest_dsp_resource *res = (kest_dsp_resource*)arg;
	if (!res)
	{
		KEST_PRINTF_FORCE("=/\n");
		return;
	}
	
	kest_mem_slot *mem = (kest_mem_slot*)res->data;
	if (!mem)
	{
		KEST_PRINTF_FORCE(":0\n");
		return;
	}
	
	mem->value = result;
	
	kest_effect *effect = res->effect;
	if (!effect)
	{
		KEST_PRINTF_FORCE(":(\n");
		return;
	}

	kest_updater_notify_scope_entry(effect, res->name);
}

int kest_updater_dispatch_mem_read(kest_updater_state *state, kest_dsp_resource *res)
{
	KEST_PRINTF("kest_updater_dispatch_mem_read\n");
	if (!res)
		return ERR_NULL_PTR;
	
	kest_effect *effect = res->effect;
	
	if (!effect)
	{
		KEST_PRINTF("Error: resource has no effect\n");
		return ERR_BAD_ARGS;
	}
	
	#ifndef KEST_LIBRARY
	kest_fpga_queue_mem_read(effect->position_.mem_start + res->handle, mem_read_callback, res);
	#endif
	
	return NO_ERROR;
}

#define PRINTLINES_ALLOWED 0

int kest_updater_handle_resource_update(kest_updater_state *state, kest_dsp_resource *res)
{
	KEST_PRINTF("kest_updater_handle_resource_update\n");
	if (!state || !res)
		return ERR_NULL_PTR;
	
	kest_scope_entry *entry = NULL;
	kest_effect *effect = res->effect;
	
	switch (res->type)
	{
		case KEST_DSP_RESOURCE_LFO:
			KEST_PRINTF("It is an LFO\n");
			entry = kest_scope_lookup(effect->scope, res->name);
			kest_updater_handle_scope_entry_update(state, entry, effect);
			break;
			
		case KEST_DSP_RESOURCE_MEM:
			kest_updater_dispatch_mem_read(state, res);
			break;
	}
	
	return NO_ERROR;
}

int kest_updater_handle_resource_updates(kest_updater_state *state)
{
	if (!state)
		return ERR_NULL_PTR;
	
	for (size_t i = 0; i < state->resources.count; i++)
	{
		kest_updater_handle_resource_update(state, state->resources.entries[i]);
	}
	
	return NO_ERROR;
}

int kest_update_queue(kest_update update)
{
	KEST_PRINTF("kest_update_queue\n");
	#ifdef KEST_USE_FREERTOS
	if (xQueueSend(update_queue_, &update, pdMS_TO_TICKS(1)) != pdPASS)
		return ERR_CURRENTLY_EXHAUSTED;
	
	return NO_ERROR;
	#else
	return ERR_FEATURE_DISABLED;
	#endif
}

int kest_updater_notify_param(kest_parameter *param)
{
	if (!param)
		return ERR_BAD_ARGS;
	
	kest_update update;
	
	update.type = KEST_UPDATE_PARAM;
	update.data.param = param;
	
	return kest_update_queue(update);
}

int kest_updater_notify_mem(kest_parameter *param)
{
	if (!param)
		return ERR_BAD_ARGS;
	
	kest_update update;
	
	update.type = KEST_UPDATE_PARAM;
	update.data.param = param;
	
	return kest_update_queue(update);
}

int kest_updater_notify_preset(kest_preset *preset)
{
	if (!preset)
		return ERR_BAD_ARGS;
	
	kest_update update;
	
	update.type = KEST_UPDATE_PRESET;
	update.data.preset = preset;
	
	return kest_update_queue(update);
}

int kest_updater_notify_scope_entry(kest_effect *effect, const char *key)
{
	if (!effect || !key)
		return ERR_BAD_ARGS;
	
	kest_update update;
	
	update.type = KEST_UPDATE_SCOPE_ENTRY;
	update.data.scope_entry.effect = effect;
	update.data.scope_entry.key = key;
	
	return kest_update_queue(update);
}

#define PRINTLINES_ALLOWED 0

int kest_updater_handle_scope_entry_update(kest_updater_state *state, kest_scope_entry *entry, kest_effect *effect)
{
	KEST_PRINTF("kest_updater_handle_scope_entry_update\n");
	
	if (!state || !entry || !effect)
		return ERR_NULL_PTR;
	
	kest_dependent *dep = NULL;
	
	#ifdef PRINTLINES_ALLOWED
	kest_string str;
	kest_string_init(&str);
	#endif
	
	KEST_PRINTF("Entry %p has %d dependents\n", entry, entry->dependents.count);
	
	if (entry->dependents.count && PRINTLINES_ALLOWED)
	{
		KEST_PRINTF("Dependents:\n");
	}
	
	for (size_t i = 0; i < entry->dependents.count; i++)
	{
		dep = &entry->dependents.entries[i];
		
		#ifdef PRINTLINES_ALLOWED
		KEST_PRINTF_("\t");
		kest_string_append_dependent(&str, entry->dependents.entries[i]);
		#endif
		
		switch (dep->type)
		{
			case KEST_DEPENDENT_BLOCK_REG:
				kest_updater_add_reg_update(state, dep, effect);
				break;
			case KEST_DEPENDENT_FILTER_COEF:
				kest_updater_add_filter_coef_update(state, dep, effect);
				break;
			case KEST_DEPENDENT_BOUND_PARAMETER:
				#ifdef KEST_ENABLE_UI
				kest_parameter_refresh_pw_async(dep->data.param);
				#endif
				break;
			case KEST_DEPENDENT_DRIVEN_PARAMETER:
				#ifdef KEST_ENABLE_UI
				kest_parameter_refresh_pw_async(dep->data.param);
				KEST_PRINTF("dep->data.param = %p, dep->data.param->effect = %p, dep->data.param->pw = %p\n", dep->data.param, dep->data.param->effect, dep->data.param->pw);
				#endif
				break;
		}
		
		#ifdef PRINTLINES_ALLOWED
		KEST_PUTS_(str);
		KEST_PRINTF_("\n");
		kest_string_drain(&str);
		#endif
	}
	
	return NO_ERROR;
}

#define PRINTLINES_ALLOWED 0

int kest_updater_handle_update(kest_updater_state *state, kest_update update)
{
	KEST_PRINTF("kest_updater_handle_update:\n");
	#ifdef PRINT_UPDATES
	kest_update_print(update);
	#endif
	if (!state)
		return ERR_NULL_PTR;
	
	kest_parameter *param = NULL;
	kest_scope_entry *entry = NULL;
	kest_dependent *dep = NULL;
	kest_effect *effect = NULL;
	
	#ifdef PRINTLINES_ALLOWED
	kest_string str;
	kest_string_init(&str);
	#endif
	
	if (state->state == KEST_UPDATER_STATE_REPROGRAM)
	{
		KEST_PRINTF("Aborting; we are in reprogram state\n");
		return NO_ERROR;
	}
	
	switch (update.type)
	{
		case KEST_UPDATE_PARAM:
			param = update.data.param;
			if (!param) break;
			
			KEST_PRINTF("Handling update of parameter \"%s\"...\n", param->name);
			
			effect = param->effect;
			
			if (!effect || !effect->scope) break;
			entry = kest_scope_lookup(effect->scope, param->name_internal);
			
			if (!entry) break;
			kest_updater_handle_scope_entry_update(state, entry, effect);
			break;
		
		case KEST_UPDATE_SCOPE_ENTRY:
			effect = update.data.scope_entry.effect;
			if (!effect || !effect->scope) break;
			entry = kest_scope_lookup(effect->scope, update.data.scope_entry.key);
			if (!entry) break;
			kest_updater_handle_scope_entry_update(state, entry, effect);
			break;
		
		case KEST_UPDATE_PRESET:
			KEST_PRINTF("oooooogh\n");
			kest_updater_handle_preset_update(state, update.data.preset);
			break;
	}
	
	return NO_ERROR;
}

#define PRINTLINES_ALLOWED 0

int kest_alloc_for_filter(kest_fpga_alloc *alloc, kest_dsp_resource *res)
{
	if (!alloc || !res)
		return ERR_NULL_PTR;
	
	kest_filter *filter = (kest_filter*)res->data;
	
	if (!filter)
		return ERR_BAD_ARGS;
	
	alloc->type = KEST_ALLOC_TYPE_FILTER;
	alloc->size_1 = filter->feed_forward;
	alloc->size_2 = filter->feed_back;
	alloc->format = filter->format;
	
	return NO_ERROR;
}

int kest_alloc_for_delay(kest_fpga_alloc *alloc, kest_dsp_resource *res, kest_scope *scope)
{
	if (!alloc || !res || !scope)
		return ERR_NULL_PTR;
	
	kest_delay *delay = (kest_delay*)res->data;
	
	if (!delay)
		return ERR_BAD_ARGS;
	
	float unit_c = 0.001 * KEST_FPGA_SAMPLE_RATE;
	uint32_t delay_samples, delay_size;
	
	switch (delay->units)
	{
		case KEST_DELAY_UNITS_MS:
			KEST_PRINTF("Units: ms\n");
			unit_c = 0.001 * KEST_FPGA_SAMPLE_RATE;
			break;
		case KEST_DELAY_UNITS_SECONDS:
			KEST_PRINTF("Units: s\n");
			unit_c = KEST_FPGA_SAMPLE_RATE;
			break;
		case KEST_DELAY_UNITS_SAMPLES:
			KEST_PRINTF("Units: samples\n");
			unit_c = 1.0;
			break;
	}
	
	delay_samples = (uint32_t)(ceilf(kest_expression_evaluate(delay->delay, scope)) * unit_c);
	
	if (delay->size)
	{
		delay_size = (uint32_t)(ceilf(kest_expression_evaluate(delay->size, scope)) * unit_c);
		
		KEST_PRINTF("delay->size = %s\n", kest_expression_to_string(delay->size));
		
		KEST_PRINTF("delay_size = (uint32_t)(ceilf(kest_expression_evaluate(delay->size, scope)) * unit_c) = ceilf(%f) * %f = %d\n",
			kest_expression_evaluate(delay->size, scope), unit_c, delay_size);
	}
	else
	{
		delay_size = delay_samples + 4;
	}


	if (delay_size < delay_samples + 4)
		delay_size = delay_samples + 4;

	delay_size += 4 - (delay_size % 4);
	
	alloc->type = KEST_ALLOC_TYPE_DELAY;
	alloc->size_1 = delay_size;
	alloc->size_2 = delay_samples;
	
	return NO_ERROR;
}

int kest_alloc_to_command(kest_fpga_command *cmd, kest_fpga_alloc *alloc)
{
	if (!cmd || !alloc)
		return ERR_NULL_PTR;
	
	switch (alloc->type)
	{
		case KEST_ALLOC_TYPE_DELAY:
			*cmd = kest_fpga_command_alloc_delay(alloc->size_1, alloc->size_2);
			break;
		
		case KEST_ALLOC_TYPE_FILTER:
			*cmd = kest_fpga_command_alloc_filter(alloc->format, alloc->size_1, alloc->size_2);
			break;
	}
	
	return NO_ERROR;
}

int kest_updater_handle_preset_update_add_filter_writes(kest_updater_state *state, int handle, kest_filter *filter, kest_scope *scope)
{
	if (!state || !filter)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	kest_fpga_write write;
	
	write.type = KEST_FILTER_COEF_WRITE;
	write.addr_1 = handle;
	write.format = filter->format;
	write.scope = scope;
	
	for (size_t i = 0; i < filter->coefs.count; i++)
	{
		write.addr_2 = i;
		write.expr = filter->coefs.entries[i];
		kest_fpga_write_list_append(&state->filter_writes, write);
	}
	
	return ret_val;
}

#define PRINTLINES_ALLOWED 0

int kest_updater_handle_preset_update_add_effect(kest_updater_state *state, kest_effect *effect)
{
	KEST_PRINTF("kest_updater_handle_preset_update_add_effect\n");
	if (!state || !effect)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	kest_dsp_resource *res = NULL;
	
	kest_fpga_alloc alloc;
	kest_fpga_write write;
	kest_fpga_mem_read read;
	kest_block *block;
	
	kest_mem_slot *mem = NULL;
	
	int block_pos = effect->position_.block_start;
	
	for (size_t i = 0; i < effect->resources.count; i++)
	{
		res = effect->resources.entries[i];
		
		if (!res)
			continue;
		
		switch (res->type)
		{
			case KEST_DSP_RESOURCE_DELAY:
				ret_val = kest_alloc_for_delay(&alloc, res, effect->scope);
				
				if (ret_val == NO_ERROR)
					kest_fpga_alloc_list_append(&state->allocs, alloc);
				else
					return ret_val;
				break;

			case KEST_DSP_RESOURCE_FILTER:
				ret_val = kest_alloc_for_filter(&alloc, res);
				
				if (ret_val == NO_ERROR)
					kest_fpga_alloc_list_append(&state->allocs, alloc);
				else
					return ret_val;
				
				kest_updater_handle_preset_update_add_filter_writes(state,
					effect->position_.filter_start + res->handle, (kest_filter*)res->data, effect->scope);
				break;
				
			case KEST_DSP_RESOURCE_MEM:
			case KEST_DSP_RESOURCE_LFO:
				kest_dsp_resource_ptr_list_append(&state->resources, res);
				break;
		}
	}
	
	write.scope = effect->scope;
	
	for (size_t i = 0; i < effect->blocks.count; i++)
	{
		block = &effect->blocks.entries[i];
		write.addr_1 = block_pos + i;
		
		write.type = KEST_BLOCK_INSTR_WRITE;
		
		write.instr = kest_block_instr_encode_positional(block, &effect->position_);
		kest_fpga_write_list_append(&state->instr_writes, write);
		
		write.type = KEST_BLOCK_REG_WRITE;
		if (block->reg_0.active)
		{
			write.addr_2 = 0;
			
			write.expr = block->reg_0.expr;
			write.format = block->reg_0.format;
			kest_fpga_write_list_append(&state->reg_writes, write);
		}
		
		if (block->reg_1.active)
		{
			write.addr_2 = 1;
			
			write.expr = block->reg_1.expr;
			write.format = block->reg_1.format;
			kest_fpga_write_list_append(&state->reg_writes, write);
		}
	}
	
	KEST_PRINTF("kest_updater_handle_preset_update_add_effect done (ret_val = %s)\n", kest_error_code_to_string(ret_val));
	return ret_val;
}

int kest_updater_handle_preset_update(kest_updater_state *state, kest_preset *preset)
{
	KEST_PRINTF("kest_updater_handle_preset_update\n");
	if (!state)
		return ERR_NULL_PTR;
	
	if (!preset)
		return ERR_BAD_ARGS;
	
	kest_updater_clear(state);
	
	state->state = KEST_UPDATER_STATE_REPROGRAM;
	
	state->active_preset = preset;
	
	kest_preset_update_positions(preset);
	
	kest_effect_pll *current = preset->pipeline.effects;
	
	while (current)
	{
		kest_updater_handle_preset_update_add_effect(state, current->data);
		current = current->next;
	}
	
	KEST_PRINTF("kest_updater_handle_preset_update done\n");
	return NO_ERROR;
}

#define PRINTLINES_ALLOWED 0

int kest_fpga_write_to_command(kest_fpga_command *dest, kest_fpga_write *src)
{
	if (!dest || !src)
		return ERR_NULL_PTR;
	
	int eval_expr = 0;
	float val = 0.0f;
	
	switch (src->type)
	{
		case KEST_BLOCK_REG_WRITE:
		case KEST_BLOCK_REG_UPDATE:
		case KEST_FILTER_COEF_WRITE:
		case KEST_FILTER_COEF_UPDATE:
			eval_expr = 1;
			break;
	}
	
	if (eval_expr)
		val = kest_expression_evaluate(src->expr, src->scope);
	
	switch (src->type)
	{
		case KEST_BLOCK_INSTR_WRITE:
			*dest = kest_fpga_command_write_block_instr(src->addr_1, src->instr);
			break;
			
		case KEST_BLOCK_REG_WRITE:
			if (src->addr_2 == 0)
				*dest = kest_fpga_command_write_block_reg_0(src->addr_1, val, src->format);
			else if (src->addr_2 == 1)
				*dest = kest_fpga_command_write_block_reg_1(src->addr_1, val, src->format);
			else
				return ERR_BAD_ARGS;
			break;

		case KEST_BLOCK_REG_UPDATE:
			if (src->addr_2 == 0)
				*dest = kest_fpga_command_update_block_reg_0(src->addr_1, val, src->format);
			else if (src->addr_2 == 1)
				*dest = kest_fpga_command_update_block_reg_1(src->addr_1, val, src->format);
			else
				return ERR_BAD_ARGS;
			break;

		case KEST_FILTER_COEF_WRITE:
			*dest = kest_fpga_command_write_filter_coef(src->addr_1, src->addr_2, val, src->format);
			break;

		case KEST_FILTER_COEF_UPDATE:
			*dest = kest_fpga_command_update_filter_coef(src->addr_1, src->addr_2, val, src->format);
			break;
		
		default:
			return ERR_BAD_ARGS;
	}
	
	return NO_ERROR;
}

int kest_updater_generate_command_list(kest_updater_state *state)
{
	if (!state)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	int any_reg_updates = (state->reg_writes.count != 0);
	int current_filter_found;
	
	int_list filter_updates;
	int_list_init(&filter_updates);
	
	kest_fpga_alloc *alloc;
	kest_fpga_write *write;
	kest_fpga_command cmd;
	
	int total_delay_alloc = 0;
	
	for (size_t i = 0; i < state->allocs.count; i++)
	{
		alloc = &state->allocs.entries[i];
		ret_val = kest_alloc_to_command(&cmd, alloc);
		if (ret_val != NO_ERROR)
		{
			KEST_PRINTF_FORCE("Error: %s\n", kest_error_code_to_string(ret_val));
			return ret_val;
		}
		kest_fpga_command_list_append(&state->cmds, cmd);
		
		if (alloc->type == KEST_ALLOC_TYPE_DELAY)
		{
			total_delay_alloc += alloc->size_1;
		}
	}
	
	for (size_t i = 0; i < state->instr_writes.count; i++)
	{
		write = &state->instr_writes.entries[i];
		ret_val = kest_fpga_write_to_command(&cmd, write);
		if (ret_val != NO_ERROR)
		{
			KEST_PRINTF_FORCE("Error: %s\n", kest_error_code_to_string(ret_val));
			return ret_val;
		}
		kest_fpga_command_list_append(&state->cmds, cmd);
	}
	
	for (size_t i = 0; i < state->reg_writes.count; i++)
	{
		write = &state->reg_writes.entries[i];
		ret_val = kest_fpga_write_to_command(&cmd, write);
		if (ret_val != NO_ERROR)
		{
			KEST_PRINTF_FORCE("Error: %s\n", kest_error_code_to_string(ret_val));
			return ret_val;
		}
		kest_fpga_command_list_append(&state->cmds, cmd);
	}
	
	for (size_t i = 0; i < state->filter_writes.count; i++)
	{
		write = &state->filter_writes.entries[i];
		ret_val = kest_fpga_write_to_command(&cmd, write);
		if (ret_val != NO_ERROR)
		{
			KEST_PRINTF_FORCE("Error: %s\n", kest_error_code_to_string(ret_val));
			return ret_val;
		}
		kest_fpga_command_list_append(&state->cmds, cmd);
		
		if (write->type == KEST_FILTER_COEF_UPDATE)
		{
			current_filter_found = 0;
			for (size_t j = 0; !current_filter_found && j < filter_updates.count; j++)
			{
				if (filter_updates.entries[j] == write->addr_1)
					current_filter_found = 1;
			}
			if (!current_filter_found)
				int_list_append(&filter_updates, write->addr_1);
		}
	}
	
	for (size_t i = 0; i < filter_updates.count; i++)
		kest_fpga_command_list_append(&state->cmds, kest_fpga_command_commit_filter_coefs(filter_updates.entries[i]));
	
	if (any_reg_updates)
		kest_fpga_command_list_append(&state->cmds, kest_fpga_command_commit_reg_updates());
	
	if (total_delay_alloc)
	{
		KEST_PRINTF_FORCE("total_delay_alloc = %d\n", total_delay_alloc);
	}
	if (total_delay_alloc > KEST_TAIL_CUTOFF)
	{
		KEST_PRINTF_FORCE("which is greater than cutoff %d. activating tail...\n", KEST_TAIL_CUTOFF);
		kest_fpga_command_list_append(&state->cmds, kest_fpga_command_enable_tail());
	}
	
	return ret_val;
}

int kest_updater_generate_tx_batch(kest_updater_state *state)
{
	if (!state)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	kest_fpga_command_list_append_encoded(&state->cmds, &state->batch);
	
	return ret_val;
}

int kest_updater_send(kest_updater_state *state)
{
	KEST_PRINTF("kest_updater_send(state = %p)\n", state);
	#ifdef KEST_LIBRARY
	return ERR_FEATURE_DISABLED;
	#else
	
	if (!state)
		return ERR_NULL_PTR;
	
	int ret_val = NO_ERROR;
	
	kest_fpga_transfer_batch send_batch;
	
	if (state->batch.len == 0 && state->state == KEST_UPDATER_STATE_READY)
	{
		return NO_ERROR;
	}
	
	int len = state->batch.len;
	int buf_len = (len < 64) ? 64 : len;
	
	send_batch.buf = kest_alloc(buf_len);
	
	if (!send_batch.buf)
	{
		KEST_PRINTF("Send buffer failed to alloc :(\n");
		return ERR_ALLOC_FAIL;
	}
	
	send_batch.buf_len = buf_len;
	send_batch.len = len;
	send_batch.buffer_owned = 0;
	
	memcpy(send_batch.buf, state->batch.buf, len);
	
	switch (state->state)
	{
		case KEST_UPDATER_STATE_READY:
			KEST_PRINTF("Sending batch...\n");
			kest_fpga_queue_transfer_batch(send_batch);
			break;
		
		case KEST_UPDATER_STATE_REPROGRAM:
			KEST_PRINTF("Programming...\n");
			kest_fpga_queue_program_batch(send_batch);
			state->state = KEST_UPDATER_STATE_READY;
			break;
	}
	
	return ret_val;
	#endif
}

#define PRINTLINES_ALLOWED 0

kest_fpga_transfer_batch kest_standalone_generate_program_batch(kest_effect_ptr_list *effects)
{
	KEST_PRINTF("kest_standalone_generate_program_batch\n");
	kest_fpga_transfer_batch batch;
	
	kest_updater_state state;
	kest_updater_state_init(&state);
	
	if (!effects)
		goto standalone_generate_finish;
	
	kest_effect_ptr_list_update_positions(effects);
	
	state.state = KEST_UPDATER_STATE_REPROGRAM;
	
	for (size_t i = 0; i < effects->count; i++)
		kest_updater_handle_preset_update_add_effect(&state, effects->entries[i]);
		
	kest_updater_generate_command_list(&state);
	
	kest_fpga_batch_append(&state.batch, COMMAND_BEGIN_PROGRAM);
	kest_updater_generate_tx_batch(&state);
	kest_fpga_batch_append(&state.batch, COMMAND_END_PROGRAM);
	
standalone_generate_finish:
	
	batch = state.batch;
	kest_updater_state_destroy(&state);
	batch.buffer_owned = 0;
	return batch;
}

#include "kest_int.h"

static const char *FNAME = "kest_fpga_dma.c";

#define PRINTLINES_ALLOWED 0

IMPLEMENT_LIST(kest_fpga_read_spec);

#ifdef KEST_ENABLE_UI
void kest_fpga_periodic_read_timer_cb(lv_timer_t *timer)
{
	kest_fpga_periodic_read *read = lv_timer_get_user_data(timer);
	
	if (!read)
		return;
	
	kest_fpga_queue_read(&read->spec);
}
#endif

int kest_fpga_periodic_read_init(kest_fpga_periodic_read *read)
{
	if (!read)
		return ERR_NULL_PTR;
	
	memset(read, 0, sizeof(kest_fpga_periodic_read));
	
	return NO_ERROR;
}

int kest_fpga_periodic_read_init_mem(kest_fpga_periodic_read *read)
{
	if (!read)
		return ERR_NULL_PTR;
	
	memset(read, 0, sizeof(kest_fpga_periodic_read));
	
	read->spec.type 		= DATA_REQ_MEM;
	read->spec.addr_size 	= KEST_FPGA_MEM_ADDR_BYTES;
	read->spec.ret_size 	= KEST_FPGA_DATA_BYTES;
	read->spec.callback 	= kest_periodic_mem_read_cb;
	
	read->period_ms 		= 10;
	
	return NO_ERROR;
}

int kest_periodic_read_dispatch(kest_fpga_periodic_read *read)
{
	#ifdef KEST_LIBRARY
	return NO_ERROR;
	#else
	if (!read)
		return ERR_NULL_PTR;
	else
		return kest_fpga_queue_read(&read->spec);
	#endif
}

int kest_begin_periodic_read(kest_fpga_periodic_read *read)
{
	return NO_ERROR;
	
	KEST_PRINTF("kest_begin_periodic_read\n");
	if (!read)
		return ERR_NULL_PTR;
	
#ifndef KEST_ENABLE_UI
	return ERR_FEATURE_DISABLED;
#else
	//read->timer = lv_timer_create(kest_fpga_periodic_read_timer_cb, read->period_ms, read);
	
	//if (!read->timer)
	//	return ERR_UNKNOWN_ERR;
	
	KEST_PRINTF("kest_begin_periodic_read done\n");
	return NO_ERROR;
#endif
}

int kest_periodic_mem_read_cb(kest_fpga_read_spec *read)
{
	#ifdef KEST_LIBRARY
	return NO_ERROR;
	#else
	KEST_PRINTF("kest_periodic_mem_read_cb(read = %p)\n", read);
	
	if (!read)
		return ERR_NULL_PTR;
	
	kest_scope_entry *entry = read->data;
	
	if (!entry)
		return ERR_BAD_ARGS;
	
	kest_mem_slot *mem_slot = entry->val.mem;
	
	if (!mem_slot)
		return ERR_BAD_ARGS;
	
	#ifdef KEST_FPGA_SIMULATED
	kest_fpga_sample_t s = 1;
	#else
	kest_fpga_sample_t s = read->result & 0xFFFF;
	#endif
	float f = powf(2, -15) * (float)s;
	
	KEST_PRINTF("Result: %s%.07f\n", f > 0 ? " " : "", f);
	
	mem_slot->value = s;
	entry->updated = 1;
	
	//if (mem_slot->effect && mem_slot->effect->preset)
	//	kest_active_preset_updater_notify_effect_by_ptr(mem_slot->effect);
	#endif
	
	return NO_ERROR;
}

int kest_fpga_periodic_read_activate(kest_fpga_periodic_read *read)
{
	return ERR_FEATURE_DISABLED;
	
	KEST_PRINTF("kest_fpga_periodic_read_activate(read = %p)\n", read);
	if (!read)
		return ERR_NULL_PTR;
	
	if (read->active)
		return NO_ERROR;
	
#ifndef KEST_ENABLE_UI
	return ERR_FEATURE_DISABLED;
#else
	//read->timer = lv_timer_create(kest_fpga_periodic_read_timer_cb, read->period_ms, read);
	read->active = 1;
	
	return NO_ERROR;
#endif
}

int kest_fpga_periodic_read_deactivate(kest_fpga_periodic_read *read)
{
	return ERR_FEATURE_DISABLED;
	
	if (!read)
		return ERR_NULL_PTR;
	
	read->active = 0;
	
#ifdef KEST_ENABLE_UI
	if (read->timer)
	{
		//lv_timer_del(read->timer);
		read->timer = NULL;
	}
#endif
	
	return NO_ERROR;
}

void kest_fpga_periodic_read_activate_async_wrapper(void *read)
{
	return;
	
	KEST_PRINTF("kest_fpga_periodic_read_activate_async_wrapper\n");
	kest_fpga_periodic_read_activate((kest_fpga_periodic_read*)read);
}

void kest_fpga_periodic_read_deactivate_async_wrapper(void *read)
{
	kest_fpga_periodic_read_deactivate((kest_fpga_periodic_read*)read);
}

int kest_fpga_periodic_read_activate_async(kest_fpga_periodic_read *read)
{
	return NO_ERROR;
	
	KEST_PRINTF("kest_fpga_periodic_read_activate_async(read = %p)\n", read);
	if (!read)
		return ERR_NULL_PTR;
	
#ifndef KEST_ENABLE_UI
	return ERR_FEATURE_DISABLED;
#else
	kest_ui_async_call(kest_fpga_periodic_read_activate_async_wrapper, read);
	
	return NO_ERROR;
#endif
}

int kest_fpga_periodic_read_deactivate_async(kest_fpga_periodic_read *read)
{
	return NO_ERROR;
	
	if (!read)
		return ERR_NULL_PTR;
	
	read->active = 0;
	
#ifndef KEST_ENABLE_UI
	return ERR_FEATURE_DISABLED;
#else
	kest_ui_async_call(kest_fpga_periodic_read_deactivate_async_wrapper, read);
	
	return NO_ERROR;
#endif
}

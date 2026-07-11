#include "kest_int.h"

#define PRINTLINES_ALLOWED 0

static const char *FNAME = "kest_param_update.c";

#ifdef KEST_USE_FREERTOS
#define UPDATE_QUEUE_LENGTH 64

static kest_parameter_update update_queue[UPDATE_QUEUE_LENGTH];
static int n_waiting = 0;
static int update_queue_head = 0;
static int update_queue_tail = 0;

static kest_parameter_update update_array[MAX_CONCURRENT_PARAM_UPDATES];
static int n_updates = 0;

#define UPDATE_RATE_HZ 100
#define UPDATE_PERIOD_MS (1000.0f / (float)UPDATE_RATE_HZ)

static const int update_period_ticks = (pdMS_TO_TICKS((int)UPDATE_PERIOD_MS) == 0) ? 1 : pdMS_TO_TICKS((int)UPDATE_PERIOD_MS);

int queue_initd = 0;
QueueHandle_t update_rtos_queue;

int kest_init_parameter_updater()
{
	xTaskCreate(kest_param_update_task, "kest_param_update_task", 4096, NULL, 8, NULL);
	return NO_ERROR;
}

void remove_param_update(int index)
{
	for (int i = index; i + 1 < n_updates; i++)
		update_array[i] = update_array[i+1];
	
	n_updates--;
}

int add_param_update(kest_parameter_update up)
{
	for (int i = 0; i < n_updates; i++)
	{
		if (update_queue[i].id.preset_id 		== up.id.preset_id
		 && update_queue[i].id.effect_id 	== up.id.effect_id
		 && update_queue[i].id.parameter_id 	== up.id.parameter_id)
		{
			update_array[i].target = up.target;
		}
	}
	
	if (n_updates + 1 >= MAX_CONCURRENT_PARAM_UPDATES)
		return ERR_CURRENTLY_EXHAUSTED;
	
	update_array[n_updates++] = up;
	
	return NO_ERROR;
}

void print_parameter_update(kest_parameter_update up)
{
	KEST_PRINTF("%d.%d.%d -> %s%.03f\n", up.id.preset_id, up.id.effect_id, up.id.parameter_id, (up.target >= 0) ? " " : "", up.target);
}

void kest_param_update_task(void *arg)
{
	update_rtos_queue = xQueueCreate(16, sizeof(kest_parameter_update));
	queue_initd = 1;
	
	vTaskDelay(pdMS_TO_TICKS(FPGA_BOOT_MS));
	
	TickType_t last_wake = xTaskGetTickCount();
	
	kest_parameter_update current;
	kest_event event;
	kest_update update;
	kest_effect *effect;
	kest_parameter *param;
	
	float val;
	float diff;
	int enqueue;
	int send;
	int commit;
	int k;
	
	int ret_val;
	int wake_fpga_updater;
	
	while (1)
	{
		wake_fpga_updater = 0;
		while ((update_queue_tail + 1) % UPDATE_QUEUE_LENGTH != update_queue_head && xQueueReceive(update_rtos_queue, &current, 0) == pdPASS)
		{
			enqueue = 1;
			for (int i = 0; i < n_updates; i++)
			{
				print_parameter_update(current);
				if (update_array[i].id.preset_id 		== current.id.preset_id
				 && update_array[i].id.effect_id 	== current.id.effect_id
				 && update_array[i].id.parameter_id 	== current.id.parameter_id)
				{
					update_array[i].target = current.target;
					enqueue = 0;
					break;
				}
			}
			
			if (!enqueue)
				continue;
			
			for (int j = update_queue_head; j != update_queue_tail; j = (j + 1) % UPDATE_QUEUE_LENGTH)
			{
				print_parameter_update(update_queue[j]);
				
				if (update_queue[j].id.preset_id 		== current.id.preset_id
				 && update_queue[j].id.effect_id 	== current.id.effect_id
				 && update_queue[j].id.parameter_id 	== current.id.parameter_id)
				{
					update_queue[j].target = current.target;
					enqueue = 0;
					break;
				}
			}
			
			if (enqueue)
			{
				update_queue[update_queue_tail] = current;
				update_queue_tail = (update_queue_tail + 1) % UPDATE_QUEUE_LENGTH;
			}
		}
		
		while (update_queue_tail != update_queue_head && n_updates < MAX_CONCURRENT_PARAM_UPDATES)
		{
			update_array[n_updates++] = update_queue[update_queue_head];
			update_queue_head = (update_queue_head + 1) % UPDATE_QUEUE_LENGTH;
			print_parameter_update(update_array[n_updates - 1]);
		}
		
		commit = 0;
		
		for (int i = 0; i < n_updates; i++)
		{
			current = update_array[i];
			print_parameter_update(current);
			
			if ((ret_val = cxt_get_parameter_and_effect_by_id(&global_cxt, update_array[i].id, &update_array[i].p, &update_array[i].t)) != NO_ERROR)
			{
				remove_param_update(i);
				KEST_PRINTF("Removing update %d from queue for reason: cxt_get_parameter_and_effect_by_id returned %s!\n", i, kest_error_code_to_string(ret_val));
				i--;
				continue;
			}
			
			param = update_array[i].p;
			
			if (!param)
			{
				remove_param_update(i);
				i--;
				continue;
			}
			
			if (update_array[i].id.preset_id != CONTEXT_PRESET_ID)
			{
				update_array[i].send = (update_array[i].t != NULL);
			
				if (update_array[i].send)
				{
					for (int j = 0; j < i && update_array[i].send; j++)
					{
						if (update_array[j].t == update_array[i].t)
							update_array[i].send = 0;
					}
					
					if (!(update_array[i].send && update_array[i].t/* && xSemaphoreTake(update_array[i].t->mutex, 0) == pdTRUE*/))
						update_array[i].send = 0;
				}
			}
			
			val = kest_parameter_evaluate(param);
			diff = update_array[i].target - val;
			
			if (param->scale == PARAMETER_SCALE_LINEAR)
			{
				if (diff > UPDATE_PERIOD_MS * param->max_velocity)
					diff = UPDATE_PERIOD_MS * param->max_velocity;
				
				if (diff < -UPDATE_PERIOD_MS * param->max_velocity)
					diff = -UPDATE_PERIOD_MS * param->max_velocity;
			}
			else if (param->scale == PARAMETER_SCALE_LOGARITHMIC)
			{
				if (diff > UPDATE_PERIOD_MS * param->max_velocity * param->value)
					diff = UPDATE_PERIOD_MS * param->max_velocity * param->value;
				
				if (diff < -UPDATE_PERIOD_MS * param->max_velocity * param->value)
					diff = -UPDATE_PERIOD_MS * param->max_velocity * param->value;
			}
			
			KEST_PRINTF("Set %s {%d.%d.%d} from %s%.05f to %s%.05f\n", param->name ? param->name : "(NULL)",
				update_array[i].id.preset_id, update_array[i].id.effect_id, update_array[i].id.parameter_id, val < 0 ? "" : " ", val, (val + diff) < 0 ? "" : " ", val + diff);
			kest_parameter_set(param, val + diff);
			
			if (update_array[i].id.preset_id == CONTEXT_PRESET_ID)
			{
				KEST_PRINTF("%d = update_array[%d].id.preset_id == CONTEXT_PRESET_ID = %d\n", update_array[i].id.preset_id, i, CONTEXT_PRESET_ID);
				update_array[i].send = 0;
				if (param == &global_cxt.input_gain)
				{
					kest_fpga_queue_input_gain_set(param->value);
					kest_ui_async_call(kest_parameter_widget_refresh_async_wrapper, input_gain_widget);
				}
				else if (param == &global_cxt.output_gain)
				{
					kest_fpga_queue_output_gain_set(param->value);
					kest_ui_async_call(kest_parameter_widget_refresh_async_wrapper, output_gain_widget);
				}
			}
			else
			{
				commit = 1;
			}
		}
		
		if (n_updates)
		{
			KEST_PRINTF("Processed %d updates. Send array:\n", n_updates);
			
			for (int i = 0; i < n_updates; i++)
			{
				KEST_PRINTF("update_array[%d].send = %d (update_array[%d].t = %p)\n", i, update_array[i].send, i, update_array[i].t);
			}
		}
		
		for (int i = 0; i < n_updates; i++)
		{			
			if (update_array[i].t && update_array[i].send)
			{
				if (global_cxt.active_preset && global_cxt.active_preset->id == update_array[i].id.preset_id)
				{
					update.type = KEST_UPDATE_PARAM;
					update.data.param = update_array[i].p;
					kest_update_queue(update);
				}
			}
		}
		
		for (int i = 0; i < n_updates; i++)
		{
			if (!update_array[i].p)
			{
				KEST_PRINTF("Removing update %d from queue for reason: no parameter!\n", i);
				remove_param_update(i);
				i--;
				continue;
			}
			
			//kest_representation_ptr_list_queue_updates_(&update_array[i].p->reps);
			
			if (update_array[i].p->value == update_array[i].target)
			{
				KEST_PRINTF("Removing update %d from queue for reason: value %.03f equals target %.03f\n", i, update_array[i].p->value, update_array[i].target);
				if (update_array[i].p->id.preset_id == CONTEXT_PRESET_ID)
					kest_queue_state_save();
				else if (update_array[i].t && update_array[i].t->preset)
					kest_queue_preset_save(update_array[i].t->preset);
				
				remove_param_update(i);
				i--;
				continue;
			}
		}
		
		if (commit)
		{
			//kest_fpga_queue_register_commit();
		}
		
		if (wake_fpga_updater)
		{
			//kest_fpga_updater_wake();
		}
		
		xTaskDelayUntil(&last_wake, update_period_ticks);
	}
}

int kest_parameter_trigger_update(kest_parameter *param, float target)
{
	KEST_PRINTF("kest_parameter_trigger_update, param = %p, target = %f\n", param, target);
	if (!param)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("Parameter %s, ID %d.%d.%d. Current value: %f. Update target: %f. Max velocity: %f\n",
		param->name, param->id.preset_id, param->id.effect_id, param->id.parameter_id,
		param->value, target, param->max_velocity);
	
	kest_parameter_update up;
	up.id = param->id;
	up.target = target;
	
	while (!queue_initd);
	
	if (xQueueSend(update_rtos_queue, &up, pdMS_TO_TICKS(1)) != pdPASS)
		return ERR_CURRENTLY_EXHAUSTED;
	
	return NO_ERROR;
}
#else
int kest_parameter_trigger_update(kest_parameter *param, float target)
{
	KEST_PRINTF("kest_parameter_trigger_update, param = %p, target = %f\n", param, target);
	if (!param)
		return ERR_NULL_PTR;
	
	param->value = target;
	
	return NO_ERROR;
}
#endif

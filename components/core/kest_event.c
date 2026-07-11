#include "kest_int.h"

#define PRINTLINES_ALLOWED 0

static const char *FNAME = "kest_event.c";

kest_event kest_event_preset_name_change(kest_preset *preset)
{
	kest_event ret = {.type = KEST_EVENT_PRESET_NAME_CHANGE,
		.val_ptr = (void*)preset
	};
	
	return ret;
}

kest_event kest_event_setting_change(kest_preset *preset)
{
	kest_event ret = {.type = KEST_EVENT_SETTING_CHANGE,
		.val_ptr = (void*)preset
	};
	
	return ret;
}

kest_event kest_event_sequence_name_change(kest_sequence *sequence)
{
	kest_event ret = {.type = KEST_EVENT_SEQUENCE_NAME_CHANGE,
		.val_ptr = (void*)sequence
	};
	
	return ret;
}

#ifdef KEST_ENABLE_UI
kest_event kest_event_enter_page(kest_ui_page *page)
{
	kest_event ret = {.type = KEST_EVENT_ENTER_PAGE,
		.val_ptr = (void*)page
	};
	
	return ret;
}
#endif

QueueHandle_t event_queue = NULL;

void kest_event_task(void *arg);

int kest_event_task_start()
{
	event_queue = xQueueCreate(16, sizeof(kest_event));
	
	if (!event_queue)
		return ERR_UNKNOWN_ERR;
	
	xTaskCreate(kest_event_task, "kest_event_task", 8192, NULL, 8, NULL);
	
	return NO_ERROR;
}

void kest_event_handle(kest_event event);

void kest_event_task(void *arg)
{
	kest_event event;
	
	while (xQueueReceive(event_queue, &event, portMAX_DELAY) == pdTRUE)
	{
		kest_event_handle(event);
	}
	
	vTaskDelete(NULL);
}

int kest_event_log(kest_event event)
{
	if (xQueueSend(event_queue, &event, pdMS_TO_TICKS(1)) != pdPASS)
		return ERR_CURRENTLY_EXHAUSTED;
	
	return NO_ERROR;
}

void kest_event_log_from_ISR(kest_event event, BaseType_t *xHigherPriorityTaskWoken)
{
    xQueueSendFromISR(event_queue, &event, xHigherPriorityTaskWoken);
}

void kest_event_handle(kest_event event)
{
	kest_event new_event;
	kest_update update;
	
	kest_ui_page *current_page = global_cxt.pages.current_page;
	kest_ui_page *new_page = current_page;
	int update_view = 0;
	
	switch (event.type)
	{
		case KEST_EVENT_STARTUP:
			kest_init();
			break;
		
		case KEST_EVENT_PARAM_CHANGE:
			update.type = KEST_UPDATE_PARAM;
			update.data.param = event.val_ptr;
			kest_update_queue(update);
			break;
		
		case KEST_EVENT_SETTING_CHANGE:
			update.type = KEST_UPDATE_PRESET;
			update.data.preset = event.val_ptr;
			kest_update_queue(update);
			break;
		
		case KEST_EVENT_FOOTSWITCH:
			switch (event.val_i)
			{
				case 0:
					new_event.type = KEST_EVENT_SEQUENCE_REGRESS;
					kest_event_log(new_event);
					KEST_PRINTF_FORCE("Left footswitch\n");
					break;
					
				case 1:
					new_event.type = KEST_EVENT_SEQUENCE_ADVANCE;
					kest_event_log(new_event);
					KEST_PRINTF_FORCE("Right footswitch\n");
					break;
				
				default:
					break;
			}
			break;
		
		case KEST_EVENT_SEQUENCE_START:
			
			break;
			
		case KEST_EVENT_SEQUENCE_ADVANCE:
			KEST_PRINTF_FORCE("Advance sequence %p\n", global_cxt.sequence);
			if (global_cxt.sequence)
			{
				update_view = global_cxt.active_preset && (global_cxt.pages.current_page == global_cxt.active_preset->view_page);
				
				kest_sequence_advance(global_cxt.sequence);
				
				new_page = (update_view && global_cxt.active_preset && global_cxt.active_preset->view_page) ? global_cxt.active_preset->view_page : current_page;
				
				if (new_page != current_page)
					kest_ui_page_enter_forwards_async(new_page);
			}
			break;
			
		case KEST_EVENT_SEQUENCE_REGRESS:
			KEST_PRINTF_FORCE("Regress sequence %p\n", global_cxt.sequence);
			
			update_view = global_cxt.active_preset && (global_cxt.pages.current_page == global_cxt.active_preset->view_page);
			
			kest_sequence_regress(global_cxt.sequence);
			
			new_page = (update_view && global_cxt.active_preset && global_cxt.active_preset->view_page) ? global_cxt.active_preset->view_page : current_page;
				
			if (new_page != current_page)
				kest_ui_page_enter_backwards_async(new_page);
			break;
			
		case KEST_EVENT_PRESET_NAME_CHANGE:
			kest_preset_handle_name_change((kest_preset*)event.val_ptr);
			break;
			
		case KEST_EVENT_SEQUENCE_NAME_CHANGE:
			kest_sequence_handle_name_change((kest_sequence*)event.val_ptr);
			break;
			
		case KEST_EVENT_ENTER_PAGE:
			kest_queue_state_save();
			break;
	}
}

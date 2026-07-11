#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>

#include "kest_int.h"

static const char *FNAME = "kest_footswitch.c";
#define PRINTLINES_ALLOWED 1

#define HW_SWITCH_GPIO_0   20   // n = 0
#define HW_SWITCH_GPIO_1   21   // n = 1

#define HW_SWITCH_TASK_STACK  	4096
#define HW_SWITCH_TASK_PRIO   	6

uint8_t  left_switch_state = 0;
uint8_t right_switch_state = 0;

static QueueHandle_t hw_switch_queue = NULL;

static int64_t footswitch_times[N_FOOTSWITCHES] = {0, 0};

static void IRAM_ATTR hw_switch_isr(void *arg)
{
    int n = (int)arg;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    int64_t time = kest_system_time_ms();
    
    if (time < footswitch_times[n] + HW_SWITCH_DEBOUNCE_MS)
		return;
	
	footswitch_times[n] = time;
    
    kest_event event;
    event.type = KEST_EVENT_FOOTSWITCH;
    event.val_i = n;
    
    kest_event_log_from_ISR(event, &xHigherPriorityTaskWoken);

    if (xHigherPriorityTaskWoken)
    {
        portYIELD_FROM_ISR();
    }
}

void footswitch_task(void *arg)
{
	int n;
    TickType_t last_event_tick[2] = {0, 0};
    const TickType_t debounce_ticks = pdMS_TO_TICKS(HW_SWITCH_DEBOUNCE_MS);

    while (true)
    {
        if (xQueueReceive(hw_switch_queue, &n, portMAX_DELAY) == pdTRUE)
        {
			TickType_t now = xTaskGetTickCount();

            if ((now - last_event_tick[n]) >= debounce_ticks)
            {
                last_event_tick[n] = now;
				KEST_PRINTF("SW %d detectde\n", n);
							
				#ifdef USE_DISPLAY
                cxt_handle_hw_switch(&global_cxt, n);
                #endif
            }
        }
    }
}

void kest_init_footswitches()
{
    gpio_config_t cfg = {
        .pin_bit_mask =
            (1ULL << HW_SWITCH_GPIO_0) |
            (1ULL << HW_SWITCH_GPIO_1),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE
    };

    gpio_config(&cfg);

    gpio_install_isr_service(0);

    gpio_isr_handler_add(HW_SWITCH_GPIO_0, hw_switch_isr, (void *)0);
    gpio_isr_handler_add(HW_SWITCH_GPIO_1, hw_switch_isr, (void *)1);
    
    memset(footswitch_times, 0, N_FOOTSWITCHES * sizeof(int64_t));
}

int init_footswitch_task()
{
    hw_switch_queue = xQueueCreate(8, sizeof(int));

	kest_init_footswitches();

    xTaskCreate(
        footswitch_task,
        "hw_switch_task",
        HW_SWITCH_TASK_STACK,
        NULL,
        HW_SWITCH_TASK_PRIO,
        NULL
    );
	
	return NO_ERROR;
}

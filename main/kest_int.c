#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>

static const char *FNAME = "kest_int.c";
#define PRINTLINES_ALLOWED 1

#include "bsp/esp32_p4_nano.h"
#include "esp_task_wdt.h"

#include "kest_int.h"

kest_context global_cxt;


void app_main()
{
	int ret_val;
	
	kest_printf_init();
	
	srand(time(0));
	
	esp_task_wdt_deinit();
	
	#ifdef USE_DISPLAY
	#ifndef SGTL_TEST
	KEST_PRINTF("Initialise display...\n");
	lv_disp_t *disp;
	waveshare_dsi_touch_5_a_init(&disp);
	KEST_PRINTF("Done\n");
	#endif
	#endif
	
	kest_event_task_start();
	
	kest_event startup_event;
	startup_event.type = KEST_EVENT_STARTUP;
	
	kest_event_log(startup_event);
	
	#ifdef KEST_SIMULATED
	#ifdef USE_DISPLAY
	#ifndef SGTL_TEST
    while (1)
    {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    #endif
    #endif
	#endif
}

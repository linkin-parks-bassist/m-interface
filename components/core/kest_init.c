#include "kest_int.h"

#define PRINTLINES_ALLOWED 0

static const char *FNAME = "kest_init.c";

int kest_init()
{
	KEST_PRINTF("Initialising...\n");
	#ifdef SGTL_TEST
	xTaskCreate(kest_sgtl5000_init, "kest_sgtl5000_init_task", 8192, NULL, 8, NULL);
	return NO_ERROR;
	#endif
	
	#ifndef USE_DISPLAY
	xTaskCreate(kest_sgtl5000_init, "kest_sgtl5000_init_task", 8192, NULL, 8, NULL);
	init_footswitch_task();
	
	return NO_ERROR;
	#endif
	
	int ret_val = NO_ERROR;
	
	kest_mem_init();
	
	kest_update_task_start();
	
	#ifdef USE_DISPLAY
	kest_ui_lock();
	kest_init_context(&global_cxt);
	kest_init_global_pages(&global_cxt.pages);
	kest_ui_unlock();
	#endif
	
	#ifdef USE_FPGA
	kest_init_fpga_comms();
	kest_init_parameter_updater();
	#endif
	
	#ifdef USE_SDCARD
	init_sd_card();
	#endif
	
	#ifdef USE_SGTL5000
	xTaskCreate(kest_sgtl5000_init, "kest_sgtl5000_init_task", 8192, NULL, 8, NULL);
	#endif
	
	kest_init_directories();
	
	kest_ui_lock();
	load_effects(&global_cxt);
	init_effect_selector_eff(&global_cxt.pages.effect_selector);
	load_saved_presets(&global_cxt);
	load_saved_sequences(&global_cxt);
	kest_ui_unlock();
	
	kest_create_ui_async();
	
	kest_state state;
	ret_val = load_state_from_file(&state, SETTINGS_FNAME);
	
	if (ret_val == NO_ERROR)
	{
		ret_val = kest_cxt_restore_state(&global_cxt, &state);
		kest_cxt_enter_previous_current_page(&global_cxt, &state);
		
		KEST_PRINTF("Restored state from disk with error code \"%s\"\n", kest_error_code_to_string(ret_val));
	}
	else
	{
		KEST_PRINTF("Unable to restore state from disk: \"%s\"\n", kest_error_code_to_string(ret_val));
	}
	
	kest_init_file_task();
	
	#ifndef KEST_DESKTOP
	kest_init_footswitches();
	#endif
	
	return ret_val;
}

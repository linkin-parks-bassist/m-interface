#ifndef KEST_LIBRARY
#ifdef KEST_DESKTOP
#include "kest_desktop.h"
#else
#ifndef KEST_INTERFACE_MAIN_H_
#define KEST_INTERFACE_MAIN_H_

#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define KEST_ENABLE_LV_LOGGING

#define KEST_ENABLE_UI


//#define SGTL_TEST


#define USE_DISPLAY
#define KEST_ENABLE_SDCARD
#define USE_SDCARD

//#define USE_COMMS

#define KEST_ENABLE_FPGA
#define USE_FPGA

#define KEST_PLATFORM_P4_NANO

//#define USE_TEENSY
//#define PRINT_MEMORY_USAGE

//#define KEST_ENABLE_REPRESENTATIONS
#define KEST_ENABLE_GLOBAL_CONTEXT
#define KEST_ENABLE_SEQUENCES

#define USE_5A
#define USE_SGTL5000
#define KEST_USE_FREERTOS

#include "driver/i2c_master.h"
#include <esp_log.h>

#ifdef KEST_USE_FREERTOS
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>
#endif




#ifdef USE_5A
#include "waveshare_dsi_touch_5_a.h"
#endif

#include <lvgl.h>

#define LL_MALLOC kest_alloc
#define LL_FREE   kest_free

#include "kest_error_codes.h"

#include "kest_alloc.h"
#include "kest_bump_arena.h"
#include "kest_pool.h"

#include "kest_list.h"
#include "kest_dict.h"
#include "kest_string.h"
#include "kest_linked_list.h"
#include "kest_global.h"

#define KEST_FILENAME_LEN 32

#include "kest_fpga_defs.h"
#include "kest_fpga_instr.h"
#include "kest_fpga_io.h"
#include "kest_fpga_cmd.h"
#include "kest_fpga_dma.h"
#include "kest_fpga_position.h"
#include "kest_fpga_update.h"
#include "kest_representation.h"
#include "kest_driver.h"
#include "kest_dependent.h"
#include "kest_expr_scope.h"
#include "kest_expression.h"
#include "kest_resource.h"
#include "kest_block.h"
#include "kest_parameter.h"
#include "kest_eff_desc.h"
#include "kest_effect.h"
#include "kest_pipeline.h"
#include "kest_preset.h"
#include "kest_param_update.h"
#include "kest_status.h"
#include "kest_event.h"
#include "kest_init.h"
#include "kest_helper_fn.h"
#include "kest_i2c.h"
#include "kest_sgtl5000.h"
#include "kest_sd.h"
#include "kest_footswitch.h"
#include "kest_fpga_comms.h"
#include "kest_button.h"
#include "kest_ui.h"
#include "kest_parameter_widget.h"
#include "kest_effect_view.h"
#include "kest_effect_settings.h"
#include "kest_effect_select.h"
#include "kest_sequence.h"
#include "kest_preset_settings.h"
#include "kest_preset_view.h"
#include "kest_sequence_view.h"
#include "kest_page_id.h"
#include "kest_context.h"
#include "kest_files.h"
#include "kest_file_task.h"
#include "kest_sequence_list.h"
#include "kest_menu.h"
#include "kest_lv_log.h"
#include "kest_tokenizer.h"
#include "kest_dictionary.h"
#include "kest_eff_parser.h"
#include "kest_expr_parser.h"
#include "kest_eff_section.h"
#include "kest_asm_parser.h"
#include "kest_reg_format.h"
#include "kest_fixed_point.h"
#include "kest_fpga_encoding.h"
#include "kest_dict_extract.h"
#include "kest_printf.h"
#include "kest_state.h"
#include "kest_update.h"

#endif
#endif
#else
#include "kest_lib.h"
#endif

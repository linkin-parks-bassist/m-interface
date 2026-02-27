#ifndef M_LIB_MAIN_H_
#define M_LIB_MAIN_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "m_linked_list.h"

#include "m_lib_cmph.h"

#include "m_error_codes.h"
#include "m_bump_arena.h"
#include "m_representation.h"
#include "m_parameter.h"
#include "m_resource.h"
#include "m_expr_scope.h"
#include "m_expression.h"
#include "m_block.h"
#include "m_eff_desc.h"
#include "m_fpga_io.h"
#include "m_transformer.h"
#include "m_pipeline.h"
#include "m_profile.h"
//#include "m_param_update.h"
//#include "m_status.h"
//#include "m_transformer_enum.h"
//#include "m_alloc.h"
#include "m_hfunc.h"
//#include "m_i2c.h"
//#include "m_sgtl5000.h"
//#include "m_sd.h"
//#include "m_footswitch.h"
//#include "m_fpga_comms.h"
//#include "m_button.h"
//#include "m_ui.h"
//#include "m_parameter_widget.h"
//#include "m_transformer_init.h"
//#include "m_transformer_table.h"
//#include "m_transformer_view.h"
//#include "m_transformer_settings.h"
//#include "m_transformer_select.h"
//#include "m_sequence.h"
//#include "m_profile_settings.h"
//#include "m_profile_view.h"
//#include "m_sequence_view.h"
//#include "m_settings.h"
//#include "m_context.h"
//#include "m_files.h"
//#include "m_sequence_list.h"
//#include "m_menu.h"
//#include "m_lv_log.h"
#include "m_tokenizer.h"
#include "m_dictionary.h"
#include "m_eff_parser.h"
#include "m_expr_parser.h"
#include "m_eff_section.h"
#include "m_asm_parser.h"
#include "m_reg_format.h"
#include "m_fpga_encoding.h"
#include "m_dict_extract.h"

#ifdef __cplusplus
}
#endif

#endif

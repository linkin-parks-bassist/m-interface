idf_component_register(SRCS "m_alloc.c"
							"m_block.c"
							"m_context.c"
							"m_eff_desc.c"
							"m_error_codes.c"
							"m_files.c"
							"m_hfunc.c"
							"m_lv_log.c"
							"m_parameter.c"
							"m_param_update.c"
							"m_pipeline.c"
							"m_profile.c"
							"m_representation.c"
							"m_sequence.c"
							"m_settings.c"
							"m_transformer.c"
							"m_transformer_enum.c"
							"m_transformer_init.c"
							"m_transformer_table.c"
					REQUIRES
							driver
							drivers
							ui
							fpga_comms
					INCLUDE_DIRS "." "../../main" "../" "../ui" "../drivers" "../parser")

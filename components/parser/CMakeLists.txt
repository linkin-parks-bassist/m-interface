idf_component_register(SRCS "m_asm_parser.c"
							"m_dictionary.c"
							"m_expr_parser.c"
							"m_eff_parser.c"
							"m_eff_section.c"
							"m_tokenizer.c"
					REQUIRES
							driver
							drivers
							ui
					INCLUDE_DIRS "." "../../main" "../" "../ui" "../drivers")

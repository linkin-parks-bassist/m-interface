idf_component_register(SRCS "m_fpga_comms.c"
							"m_fpga_encoding.c"
							"m_fpga_io.c"
							"m_reg_format.c"
					REQUIRES
							driver
							drivers
							ui
					INCLUDE_DIRS "." "../../main" "../" "../ui" "../drivers" "../parser")

							

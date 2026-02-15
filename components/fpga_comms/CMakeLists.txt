idf_component_register(SRCS "m_fpga_comms.c"
					REQUIRES
							driver
							drivers
							ui
					INCLUDE_DIRS "." "../../main" "../" "../ui" "../drivers")

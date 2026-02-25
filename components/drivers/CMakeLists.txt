idf_component_register(SRCS "m_sgtl5000.c"
							"m_sd.c"
							"m_footswitch.c"
							"waveshare_dsi_touch_5_a.c"
							"m_touch_irq.c"
							"m_i2c.c"
					REQUIRES
							driver
							esp_lcd_touch_gt911
							esp32_p4_nano
							fatfs
							core
							parser
							fpga
							tinyusb
							esp_tinyusb
					INCLUDE_DIRS "." "../../main" "../" "../ui" "../core" "../parser")

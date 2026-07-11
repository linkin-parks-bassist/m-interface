#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_check.h"
#include "esp_log.h"

#include "kest_int.h"

#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"


static const char *FNAME = "kest_sgtl5000.c";
#define PRINTLINES_ALLOWED 1

#define I2C_PORT 0
#define SGTL5000_SDA 2
#define SGTL5000_SCL 3
#define I2C_HZ 50000

static const char *TAG = "sgtl5000";

int sgtl5000_status = 0;

static uint16_t ana_ctrl;
static uint8_t  i2c_addr = 0x0A;

static bool muted;
static bool semi_automated;

static i2c_master_bus_handle_t bus;
static i2c_master_dev_handle_t dev;


typedef struct {
    int i2c_port;
    int sda_gpio;
    int scl_gpio;
    uint32_t i2c_hz;
} sgtl5000_i2c_cfg_t;

int sgtl5000_set_address_level(int cs_level_high)
{
    uint8_t new_addr = cs_level_high ? SGTL5000_I2C_ADDR_CS_HIGH : SGTL5000_I2C_ADDR_CS_LOW;
    i2c_addr = new_addr;

    if (!bus) return ERR_BAD_ARGS;

    if (dev) {
        i2c_master_bus_rm_device(dev);
        dev = NULL;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = i2c_addr,
        .scl_speed_hz    = I2C_HZ,
    };
    
    esp_err_t err = i2c_master_bus_add_device(bus, &dev_cfg, &dev);
    
    return (err == ESP_OK) ? NO_ERROR : ERR_I2C_FAIL;
}

int sgtl5000_read_reg(uint16_t reg, uint16_t *out_val)
{
    if (!dev || !out_val) return ESP_ERR_INVALID_STATE;

    uint8_t addr_be[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };
    uint8_t data[2]    = {0, 0};

    esp_err_t err = i2c_master_transmit_receive(dev, addr_be, sizeof(addr_be), data, sizeof(data), -1);
    if (err != ESP_OK) return ERR_I2C_FAIL;
    
	vTaskDelay(pdMS_TO_TICKS(2));

    *out_val = ((uint16_t)data[0] << 8) | (uint16_t)data[1];
    return NO_ERROR;
}

int sgtl5000_write_reg(uint16_t reg, uint16_t val)
{
    if (!dev) return ESP_ERR_INVALID_STATE;

    if (reg == CHIP_ANA_CTRL) ana_ctrl = val;

    uint8_t pkt[4] = {
        (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF),
        (uint8_t)(val >> 8), (uint8_t)(val & 0xFF)
    };

	esp_err_t err = i2c_master_transmit(dev, pkt, sizeof(pkt), -1);

    if (err != ESP_OK)
		return ERR_I2C_FAIL;
	
	return NO_ERROR;
}

int sgtl5000_modify_reg(uint16_t reg, uint16_t val, uint16_t mask, uint16_t *out_newval)
{
    uint16_t cur;
    int ret_val = sgtl5000_read_reg(reg, &cur);
    if (ret_val != NO_ERROR) return ret_val;

    uint16_t newv = (cur & ~mask) | (val & mask);
    ret_val = sgtl5000_write_reg(reg, newv);
    if (ret_val != NO_ERROR) return ret_val;

    if (out_newval) *out_newval = newv;
    return NO_ERROR;
}

int sgtl5000_enable(void)
{
    if (!dev) return ESP_ERR_INVALID_STATE;

    muted = true;
	
	KEST_PRINTF("Send power sequence...\n");

	int ret_val;

    if ((ret_val = sgtl5000_write_reg(CHIP_LINREG_CTRL, 	0x006C)) != NO_ERROR) return ret_val;
    
    if ((ret_val = sgtl5000_write_reg(CHIP_REF_CTRL, 		0x01F2)) != NO_ERROR) return ret_val;
    if ((ret_val = sgtl5000_write_reg(CHIP_LINE_OUT_CTRL, 	0x0F22)) != NO_ERROR) return ret_val;
    if ((ret_val = sgtl5000_write_reg(CHIP_SHORT_CTRL, 		0x4446)) != NO_ERROR) return ret_val;
    if ((ret_val = sgtl5000_write_reg(CHIP_ANA_CTRL, 		0x0137)) != NO_ERROR) return ret_val;

    if ((ret_val = sgtl5000_write_reg(CHIP_ANA_POWER, 		0x40FF)) != NO_ERROR) return ret_val;
    if ((ret_val = sgtl5000_write_reg(CHIP_DIG_POWER, 		0x0073)) != NO_ERROR) return ret_val;
    
    // delay(400);
    vTaskDelay(pdMS_TO_TICKS(400));

    if ((ret_val = sgtl5000_write_reg(CHIP_LINE_OUT_VOL, 	0x1D1D)) != NO_ERROR) return ret_val;

    if ((ret_val = sgtl5000_write_reg(CHIP_CLK_CTRL, 		0x0004)) != NO_ERROR) return ret_val;
    if ((ret_val = sgtl5000_write_reg(CHIP_I2S_CTRL, 		0x0030)) != NO_ERROR) return ret_val;
    
    if ((ret_val = sgtl5000_write_reg(CHIP_SSS_CTRL, 		0x0010)) != NO_ERROR) return ret_val;    
    if ((ret_val = sgtl5000_write_reg(CHIP_ADCDAC_CTRL, 	0x0000)) != NO_ERROR) return ret_val;    
    if ((ret_val = sgtl5000_write_reg(CHIP_DAC_VOL, 		0x3C3C)) != NO_ERROR) return ret_val;    
    if ((ret_val = sgtl5000_write_reg(CHIP_ANA_HP_CTRL, 	0x7F7F)) != NO_ERROR) return ret_val;    
    if ((ret_val = sgtl5000_write_reg(CHIP_ANA_CTRL, 		0x0036)) != NO_ERROR) return ret_val;

    semi_automated = true;

    // Set input as line in
    if ((ret_val = sgtl5000_write_reg(0x0020, 0x0055)) != NO_ERROR) return ret_val;
	if ((ret_val = sgtl5000_write_reg(0x0024, ana_ctrl | (1u<<2))) != NO_ERROR) return ret_val;
    
    if ((ret_val = sgtl5000_modify_reg(CHIP_ADCDAC_CTRL, 0, 0x0300, NULL)) != NO_ERROR)
		return ret_val;
	
	KEST_PRINTF("Done.\n");

    return NO_ERROR;
}

int sgtl5000_line_in_level(uint8_t n)
{
	if (n > 15) n = 15;
	return sgtl5000_write_reg(CHIP_ANA_ADC_CTRL, (n << 4) | n);
}

int sgtl5000_line_out_level(uint8_t n)
{
	if (n > 31) n = 31;
	else if (n < 13) n = 13;
	return sgtl5000_modify_reg(CHIP_LINE_OUT_VOL,(n<<8)|n, (31<<8)|31, NULL);
}

void sgtl_power_gpio_init(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << 2,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    gpio_config(&cfg);
}

static void read_and_print(uint16_t reg, const char *name)
{
    uint16_t v = 0xFFFF;
    int ret_val = sgtl5000_read_reg(reg, &v);
    if (ret_val != NO_ERROR)
    {
        KEST_PRINTF("RD  %-18s (0x%04X): ERROR\n", name, reg);
        return;
    }
    KEST_PRINTF("RD  %-18s (0x%04X): 0x%04X\n", name, reg, v);
}

typedef struct {
    uint16_t reg;
    const char *name;
} sgtl_reg_desc_t;

void sgtl5000_readout_registers()
{
    static const sgtl_reg_desc_t reads[] = {
        { CHIP_ID,        "CHIP_ID"        },
        { CHIP_DIG_POWER,   "CHIP_DIG_POWER"   },
        { CHIP_ANA_POWER,   "CHIP_ANA_POWER"   },
        { CHIP_CLK_CTRL,    "CHIP_CLK_CTRL"    },
        { CHIP_I2S_CTRL,    "CHIP_I2S_CTRL"    },
        { CHIP_ADCDAC_CTRL, "CHIP_ADCDAC_CTRL" },
        { CHIP_DAC_VOL, 	 "CHIP_DAC_VOL"	    },
        { CHIP_PAD_STRENGTH, 	 "CHIP_PAD_STRENGTH"	    },
        { CHIP_ANA_ADC_CTRL, 	 "CHIP_ANA_ADC_CTRL"	    },
        { CHIP_ANA_HP_CTRL, 	 "CHIP_ANA_HP_CTRL"	    },
        { CHIP_ANA_CTRL, 	 "CHIP_ANA_CTRL"	    },
        { CHIP_LINREG_CTRL, 	 "CHIP_LINREG_CTRL"	    },
        { CHIP_REF_CTRL, 	 "CHIP_REF_CTRL"	    },
        { CHIP_MIC_CTRL, 	 "CHIP_MIC_CTRL"	    },
        { CHIP_LINE_OUT_CTRL, 	 "CHIP_LINE_OUT_CTRL"	    },
        { CHIP_LINE_OUT_VOL, 	 "CHIP_LINE_OUT_VOL"	    },
        { CHIP_ANA_POWER, 	 "CHIP_ANA_POWER"	    },
        { CHIP_PLL_CTRL, 	 "CHIP_PLL_CTRL"	    },
        { CHIP_CLK_TOP_CTRL, 	 "CHIP_CLK_TOP_CTRL"	    },
        { CHIP_ANA_STATUS, 	 "CHIP_ANA_STATUS"	    },
        { CHIP_ANA_TEST1, 	 "CHIP_ANA_TEST1"	    },
        { CHIP_ANA_TEST2, 	 "CHIP_ANA_TEST2"	    },
        { CHIP_SHORT_CTRL, 	 "CHIP_SHORT_CTRL"	    },
        { CHIP_SSS_CTRL, 	 "CHIP_SSS_CTRL"	    },
    };

    KEST_PRINTF_FORCE("\n=== SGTL5000 READ DUMP ===\n");
    for (size_t i = 0; i < sizeof(reads)/sizeof(reads[0]); i++) {
        (void)read_and_print(reads[i].reg, reads[i].name);
    }
}

void sgtl_i2c_scan(void)
{
    KEST_PRINTF("Scanning I2C...\n");

    for (uint8_t addr = 0; addr < 128; addr++)
    {
        i2c_master_dev_handle_t scan_dev = NULL;

        i2c_device_config_t cfg = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address  = addr,
            .scl_speed_hz    = I2C_HZ,
        };

        if (i2c_master_bus_add_device(bus, &cfg, &scan_dev) != ESP_OK)
            continue;

        esp_err_t err = i2c_master_probe(bus, addr, 50);

        if (err == ESP_OK)
            KEST_PRINTF("Found device at 0x%02X\n", addr);

        i2c_master_bus_rm_device(scan_dev);
    }

    KEST_PRINTF("Scan complete.\n");
}

void kest_sgtl5000_init(void *param)
{
	//sgtl_power_gpio_init();
	
	gpio_reset_pin(2);
	gpio_reset_pin(3);
	gpio_set_pull_mode(2, GPIO_FLOATING);
	gpio_set_pull_mode(3, GPIO_FLOATING);
	
	sgtl5000_i2c_cfg_t sgtl_i2c_cfg = {
        .i2c_port = I2C_PORT,
        .sda_gpio = SGTL5000_SDA,
        .scl_gpio = SGTL5000_SCL,
        .i2c_hz = I2C_HZ,
    };
    
    #ifdef SGTL_TEST
	vTaskDelay(pdMS_TO_TICKS(100));
    #else
	vTaskDelay(pdMS_TO_TICKS(3000));
    #endif

    int ret_val;
	esp_err_t err;
    
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_PORT,
        .sda_io_num = sgtl_i2c_cfg.sda_gpio,
        .scl_io_num = sgtl_i2c_cfg.scl_gpio,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags.enable_internal_pullup = false,
    };

    err = i2c_new_master_bus(&bus_cfg, &bus);
    if (err != ESP_OK) goto sgtl_init_exit;
    
    
    #ifdef SGTL_TEST
    while (1)
		sgtl_i2c_scan();
    #endif
	
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = i2c_addr,
        .scl_speed_hz    = sgtl_i2c_cfg.i2c_hz,
    };

    err = i2c_master_bus_add_device(bus, &dev_cfg, &dev);
    
    if ((ret_val = sgtl5000_set_address_level(0)) != NO_ERROR)
		goto sgtl_init_exit;
    
    KEST_PRINTF("Initialising SGTL5000...\n");
    if ((ret_val = sgtl5000_enable()) != NO_ERROR)
	{
		KEST_PRINTF("Error initialising SGTL5000L %s\n", kest_error_code_to_string(ret_val));
		goto sgtl_init_exit;
	}
	
	sgtl5000_line_in_level(7);
	sgtl5000_line_out_level(31);
	
	#if PRINTLINES_ALLOWED == 1
	sgtl5000_readout_registers();
	#endif
	
	sgtl5000_status = 1;
	
	KEST_PRINTF("SGTL5000 Initialised\n");
	
sgtl_init_exit:
	vTaskDelete(NULL);
}

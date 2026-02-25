#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/i2c.h"
#include "sd_pwr_ctrl_by_on_chip_ldo.h"

#include "tinyusb.h"
#include "tinyusb_msc.h"

#include <string.h>
#include "bsp/esp32_p4_nano.h"

static const char *TAG = "m_sd.c";

#include "m_int.h"

#define MAX_PARALLEL_FILES 5

const int mount_point_strlen = strlen(MOUNT_POINT);

const char mount_point[] = MOUNT_POINT;

sdmmc_host_t host = SDMMC_HOST_DEFAULT();
sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

esp_vfs_fat_sdmmc_mount_config_t mount_config = {
		.format_if_mount_failed = false,
		.max_files = 5,
		.allocation_unit_size = 16 * 1024
	};

static sdmmc_card_t card_obj;
sdmmc_card_t *card = &card_obj;

tinyusb_config_t tusb_cfg = {
    .task = {
        .size = 4096,
        .priority = 5,
        .xCoreID = 0,
    },
};

tinyusb_msc_driver_config_t msc_drv_cfg = {
    .user_flags.auto_mount_off = 1,
};

tinyusb_msc_storage_config_t storage_cfg = {
    .mount_point = TINYUSB_MSC_STORAGE_MOUNT_APP,
};

tinyusb_msc_storage_handle_t storage_handle;

SemaphoreHandle_t sd_mutex;

int sd_msc_mode = 0;

#ifdef USE_SDCARD
int init_sd_card()
{
	esp_err_t ret;

	sd_pwr_ctrl_ldo_config_t ldo_config = {
		.ldo_chan_id = 4,
	};
	sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

	ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);

	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
		return ERR_SD_INIT_FAIL;
	}
	host.pwr_ctrl_handle = pwr_ctrl_handle;

	sd_pwr_ctrl_set_io_voltage(pwr_ctrl_handle, 3300);

	slot_config.width 	= 4;
	slot_config.clk 	= 43;
	slot_config.cmd 	= 44;
	slot_config.d0 		= 39;
	slot_config.d1 		= 40;
	slot_config.d2 		= 41;
	slot_config.d3 		= 42;

	slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

	/*ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
	
	if (ret != ESP_OK)
	{
		ESP_LOGE("SD", "Failed to mount SD card: %s\n", esp_err_to_name(ret));
		return ERR_SD_INIT_FAIL;
	}*/
	
	ret = sdmmc_host_init();
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to initialise SDMMC host: %s\n", esp_err_to_name(ret));
		return ERR_SD_INIT_FAIL;
	}

	ret = sdmmc_host_init_slot(host.slot, &slot_config);   // or SDMMC_HOST_SLOT_1 depending on your target
	if (ret != ESP_OK)if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to initialise SDMMC host slot: %s\n", esp_err_to_name(ret));
		return ERR_SD_INIT_FAIL;
	}

	ret = sdmmc_card_init(&host, card);
	if (ret != ESP_OK)if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to initialise SDMMC card: %s\n", esp_err_to_name(ret));
		return ERR_SD_INIT_FAIL;
	}
	
	ret = tinyusb_driver_install(&tusb_cfg);
	storage_cfg.medium.card = card;
	
	sdmmc_card_print_info(stdout, card);
	
	
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to installed tinyusb driver: %s\n", esp_err_to_name(ret));
		return ERR_SD_INIT_FAIL;
	}

	ret = tinyusb_msc_install_driver(&msc_drv_cfg);

	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to installed tinyusb MSC driver: %s\n", esp_err_to_name(ret));
		return ERR_SD_INIT_FAIL;
	}

	ret = tinyusb_msc_new_storage_sdmmc(&storage_cfg, &storage_handle);
	
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to create tinyusb MSC SDMMC storage decide: %s\n", esp_err_to_name(ret));
		return ERR_SD_INIT_FAIL;
	}
	
	sd_mutex = xSemaphoreCreateMutex();
	
	return NO_ERROR;
}

#else

int init_sd_card()
{
	return NO_ERROR;
}

#endif

int m_sd_mode_msc()
{
    xSemaphoreTake(sd_mutex, portMAX_DELAY);

    esp_err_t ret = tinyusb_msc_set_storage_mount_point(storage_handle, TINYUSB_MSC_STORAGE_MOUNT_USB);
	
	if (ret != NO_ERROR)
	{
		ESP_LOGE(TAG, "Failed to set SD storage point to USB\n");
		return ERR_SD_MOUNT_FAIL;
	}
	
	sd_msc_mode = 1;
	
	return NO_ERROR;
}

int m_sd_mode_local()
{
	esp_err_t ret = tinyusb_msc_set_storage_mount_point(storage_handle, TINYUSB_MSC_STORAGE_MOUNT_APP);
	
	if (ret != ESP_OK)
	{
		ESP_LOGE(TAG, "Failed to uninstall tinyusb driver: %s\n", esp_err_to_name(ret));
		return ERR_SD_INIT_FAIL;
	}
	
	sd_msc_mode = 0;
	xSemaphoreGive(sd_mutex);
	
	return NO_ERROR;
}

int m_sd_toggle_msc()
{
	int ret_val;
	if (sd_msc_mode)
		ret_val = m_sd_mode_local();
	else
		ret_val = m_sd_mode_msc();
	
	tinyusb_msc_mount_point_t mp;
	tinyusb_msc_get_storage_mount_point(storage_handle, &mp);
	ESP_LOGI(TAG, "MSC mount point now: %d");
	
	return ret_val;
}

string_ll *list_files_in_directory(char *dir)
{
	#ifndef USE_SDCARD
	return NULL;
	#endif
	
	if (!dir)
		return NULL;
	
	printf("Generating list of files in %s\n", dir);
	
	char *fname = NULL;
	
	DIR *directory = opendir(dir);
	
	if (!directory)
	{
		printf("Failed to open directory!\n");
		return NULL;
	}
	
	struct dirent *directory_entry = readdir(directory);
	
	string_ll *list = NULL;
	string_ll *nl;
	
	while (directory_entry)
	{
		printf("Directory entry: %s\n", directory_entry->d_name);
		if (directory_entry->d_type == DT_DIR)
		{
			printf("... is itself a directory!\n");
			directory_entry = readdir(directory);
			continue;
		}
		
		fname = m_alloc(strlen(dir) + 1 + 255);
		
		if (!fname)
		{
			ESP_LOGE(TAG, "Error: couldn't allocate string to list directory entry %s/%s", dir, directory_entry->d_name);
			return list;
		}
		
		sprintf(fname, "%s%s", dir, directory_entry->d_name);
		
		nl = char_pll_append(list, fname);
		
		if (nl)
		{
			list = nl;
		}
		else
		{
						ESP_LOGE(TAG, "Error: couldn't append linked list to list directory entry %s/%s", dir, directory_entry->d_name);
			return list;
		}
		
		directory_entry = readdir(directory);
	}
	
	return list;
}

void erase_sd_card_void_cb(void *data)
{
	#ifdef USE_SDCARD
	erase_sd_card();
	#endif
}

int erase_folder(const char *dir)
{
	printf("Erasing directory %s...\n", dir);
	
	DIR *directory = opendir(dir);
	
	if (!directory)
	{
		printf("Failed to open directory!\n");
		return ERR_BAD_ARGS;
	}
	
	struct dirent *directory_entry = readdir(directory);
	
	int ret_val = NO_ERROR;
	int bufsize = strlen(dir) + NAME_MAX + 1;
	char *buf = m_alloc(sizeof(bufsize));
	
	if (!buf)
		return ERR_ALLOC_FAIL;
	
	while (directory_entry)
	{
		printf("Directory entry: %s\n", directory_entry->d_name);
		if (directory_entry->d_type == DT_DIR)
		{
			printf("... is itself a directory!\n");
			snprintf(buf, bufsize, "%s/%s", dir, directory_entry->d_name);
			printf("Full name: %s\n", buf);
			m_free(buf);
			ret_val = erase_folder(buf);
			rmdir(buf);
			buf = m_alloc(sizeof(bufsize));
			if (!buf)
				return ERR_ALLOC_FAIL;
		}
		else
		{
			printf("... is a file. Deleting...\n");
			snprintf(buf, bufsize, "%s/%s", dir, directory_entry->d_name);
			printf("Full name: %s\n", buf);
			remove(buf);
		}
		
		directory_entry = readdir(directory);
	}
	
	return ret_val;
}

void erase_sd_card()
{
	erase_folder(MOUNT_POINT);
}

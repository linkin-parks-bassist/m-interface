#ifndef M_INT_SD_CARD_H_
#define M_INT_SD_CARD_H_

#define MOUNT_POINT "/sdcard"

#include "m_linked_list.h"

int init_sd_card();

string_ll *list_files_in_directory(char *dir);

void erase_sd_card_void_cb(void *data);
void erase_sd_card();

int m_sd_mode_msc();
int m_sd_mode_local();

int m_sd_toggle_msc();

extern int sd_msc_mode;

extern SemaphoreHandle_t sd_mutex;

#endif

#ifndef KEST_INT_FOOTSWITCH_H_
#define KEST_INT_FOOTSWITCH_H_

#define HW_SWITCH_DEBOUNCE_MS 100

#define N_FOOTSWITCHES 2

void kest_init_footswitches();
int init_footswitch_task();

#endif

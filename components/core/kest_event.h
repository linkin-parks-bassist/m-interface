#ifndef KEST_EVENT_H_
#define KEST_EVENT_H_

#define KEST_EVENT_NONE					0
#define KEST_EVENT_STARTUP 				1
#define KEST_EVENT_PARAM_CHANGE			2
#define KEST_EVENT_PRESET_CHANGE 		3
#define KEST_EVENT_PRESET_UPDATE		4
#define KEST_EVENT_SEQUENCE_START		5
#define KEST_EVENT_SEQUENCE_ADVANCE		6
#define KEST_EVENT_SEQUENCE_REGRESS		7
#define KEST_EVENT_FOOTSWITCH			8
#define KEST_EVENT_PRESET_CREATE 		9
#define KEST_EVENT_PRESET_NAME_CHANGE 	10
#define KEST_EVENT_SEQUENCE_CREATE 		11
#define KEST_EVENT_SEQUENCE_NAME_CHANGE 12
#define KEST_EVENT_ENTER_PAGE			13

typedef struct {
	int type;
	
	int   val_i;
	float val_f;
	void *val_ptr;
} kest_event;

int kest_event_task_start();

int kest_event_log(kest_event event);
void kest_event_log_from_ISR(kest_event event, BaseType_t *xHigherPriorityTaskWoken);

struct kest_sequence;

kest_event kest_event_preset_name_change(kest_preset *preset);
kest_event kest_event_sequence_name_change(struct kest_sequence *sequence);
#ifdef KEST_ENABLE_UI
struct kest_ui_page;
kest_event kest_event_enter_page(struct kest_ui_page *page);
#endif

#endif

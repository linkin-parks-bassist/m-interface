#ifndef KEST_UPDATE_H_
#define KEST_UPDATE_H_

//#define PRINT_ALLOCS
//#define PRINT_INSTR_WRITES
//#define PRINT_REG_WRITES
//#define PRINT_FILTER_WRITES
//#define PRINT_COMMAND_LIST
//#define PRINT_UPDATES

#define KEST_UPDATE_NONE 		0
#define KEST_UPDATE_PARAM		1
#define KEST_UPDATE_PRESET		2
#define KEST_UPDATE_MEM			3
#define KEST_UPDATE_SCOPE_ENTRY	4

typedef struct {
	int type;
	
	union {
		kest_parameter *param;
		kest_preset *preset;
		
		struct {
			kest_effect *effect;
			const char *key;
		} scope_entry;
	} data;
} kest_update;

DECLARE_LIST(kest_update);

#define KEST_UPDATER_STATE_READY 		0
#define KEST_UPDATER_STATE_REPROGRAM 	1

#define KEST_BLOCK_INSTR_WRITE 	1
#define KEST_BLOCK_REG_WRITE 	2
#define KEST_BLOCK_REG_UPDATE 	3
#define KEST_FILTER_COEF_WRITE 	4
#define KEST_FILTER_COEF_UPDATE 5

typedef struct {
	int type;
	int addr_1;
	int addr_2;
	int format;
	uint32_t instr;
	kest_scope *scope;
	kest_expression *expr;
} kest_fpga_write;

#define KEST_ALLOC_TYPE_FILTER  1
#define KEST_ALLOC_TYPE_DELAY	2

#define KEST_TAIL_CUTOFF (0.1 * KEST_FPGA_SAMPLE_RATE)

typedef struct {
	int type;
	int size_1;
	int size_2;
	int format;
} kest_fpga_alloc;


typedef struct {
	int addr;
	uint64_t period_ms;
	uint64_t last_t;
	
	kest_fpga_read_spec read;
} kest_fpga_mem_read;

DECLARE_LIST(kest_fpga_write);
DECLARE_LIST(kest_fpga_alloc);
DECLARE_LIST(kest_fpga_mem_read);

typedef struct {
	int state;
	
	kest_update_list updates;
	
	kest_preset *active_preset;
	
	kest_fpga_alloc_list allocs;
	
	kest_fpga_write_list instr_writes;
	kest_fpga_write_list reg_writes;
	kest_fpga_write_list filter_writes;
	
	kest_fpga_command_list cmds;
	kest_fpga_transfer_batch batch;
	
	kest_fpga_mem_read_list reads;
	
	kest_dsp_resource_ptr_list resources;
	
	int tick_ctr;
	
} kest_updater_state;

int kest_update_task_start();
void kest_update_task(void *arg);

int kest_update_queue(kest_update update);

int kest_updater_notify_param(kest_parameter *param);
int kest_updater_notify_preset(kest_preset *preset);
int kest_updater_notify_scope_entry(kest_effect *effect, const char *key);

int kest_updater_drain_lists(kest_updater_state *state);

int kest_updater_generate_command_list(kest_updater_state *state);
int kest_updater_generate_tx_batch(kest_updater_state *state);
int kest_updater_send(kest_updater_state *state);

int kest_updater_handle_resource_updates(kest_updater_state *state);
int kest_updater_handle_update(kest_updater_state *state, kest_update update);
int kest_updater_handle_preset_update(kest_updater_state *state, kest_preset *preset);
int kest_updater_handle_scope_entry_update(kest_updater_state *state, kest_scope_entry *entry, kest_effect *effect);

kest_fpga_transfer_batch kest_standalone_generate_program_batch(kest_effect_ptr_list *effects);


void kest_updater_print_reg_writes(kest_updater_state *state);
void kest_updater_print_filter_writes(kest_updater_state *state);
void kest_updater_print_command_list(kest_updater_state *state);

#endif

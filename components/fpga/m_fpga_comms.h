#ifndef M_FPGA_COMMS_H_
#define M_FPGA_COMMS_H_

//#define PRINT_TRANSFER_BATCHES
//#define PRINT_COMMANDS

void m_fpga_comms_task(void *param);

int m_fpga_queue_transfer_batch(m_fpga_transfer_batch batch);
int m_fpga_queue_pipeline_program(m_fpga_transfer_batch batch);

int m_fpga_queue_input_gain_set(float gain_db);
int m_fpga_queue_output_gain_set(float gain_db);

int m_fpga_queue_register_commit();

#endif

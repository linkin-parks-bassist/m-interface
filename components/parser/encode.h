#ifndef M_ENCODE_H_
#define M_ENCODE_H_

#define COMMAND_WRITE_BLOCK_INSTR 	0b10010000
#define COMMAND_WRITE_BLOCK_REG 	0b11100000
#define COMMAND_UPDATE_BLOCK_REG 	0b11101000
#define COMMAND_WRITE_BLOCK_REG_0 	0b11100000
#define COMMAND_UPDATE_BLOCK_REG_0 	0b11101000
#define COMMAND_WRITE_BLOCK_REG_1 	0b11100001
#define COMMAND_UPDATE_BLOCK_REG_1 	0b11101001
#define COMMAND_ALLOC_DELAY 		0b00100000
#define COMMAND_SWAP_PIPELINES 		0b00000001
#define COMMAND_RESET_PIPELINE 		0b00001001
#define COMMAND_SET_INPUT_GAIN 		0b00000010
#define COMMAND_SET_OUTPUT_GAIN 	0b00000011
#define COMMAND_COMMIT_REG_UPDATES 	0b00001010

typedef struct
{
	int len;
	uint8_t *buf;
	int buf_len;
	int buffer_owned;
} m_fpga_transfer_batch;

m_fpga_transfer_batch m_new_fpga_transfer_batch();
void m_free_fpga_transfer_batch(m_fpga_transfer_batch batch);

int m_fpga_batch_print(m_fpga_transfer_batch seq);

int m_send_bytes_to_fpga(uint8_t *buf, int n);
int m_send_byte_to_fpga(uint8_t byte);

int m_fpga_send_byte(uint8_t byte);

void m_fpga_set_input_gain(float gain_db);
void m_fpga_set_output_gain(float gain_db);
void m_fpga_commit_reg_updates();

int m_fpga_batch_append   (m_fpga_transfer_batch *seq, uint8_t byte);
int m_fpga_batch_append_16(m_fpga_transfer_batch *seq, uint16_t x);
int m_fpga_batch_append_24(m_fpga_transfer_batch *seq, uint32_t x);
int m_fpga_batch_append_32(m_fpga_transfer_batch *seq, uint32_t x);
int m_fpga_batch_append_block_number(m_fpga_transfer_batch *batch, int block);

uint32_t m_block_instr_encode_resource_aware(m_block *block, const m_eff_resource_report *res);

int m_tx_batch_append_transformer(m_fpga_transfer_batch *batch, m_transformer *trans, m_eff_resource_report *res, int *pos);
int m_tx_batch_append_transformers(m_fpga_transfer_batch *batch, m_transformer_pll *list, m_eff_resource_report *res, int *pos);

#endif

#ifndef M_INT_FPGA_H_
#define M_INT_FPGA_H_

#define M_FPGA_SAMPLE_RATE 44100

//#define PRINT_SPI_BYTES

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

#define M_FPGA_N_BLOCKS 256

#if M_FPGA_N_BLOCKS > 256
  #define M_FPGA_BLOCK_ADDR_BYTES 2
  typedef uint16_t m_fpga_block_addr_t;
#else
  #define M_FPGA_BLOCK_ADDR_BYTES 1
  typedef uint8_t m_fpga_block_addr_t;
#endif

#define M_FPGA_DATA_WIDTH 16
#define M_FPGA_DATA_BYTES (M_FPGA_DATA_WIDTH / 8)

#if M_FPGA_DATA_WIDTH == 16
  typedef int16_t m_fpga_sample_t;
#elif M_FPGA_DATA_WIDTH == 24
  typedef int32_t m_fpga_sample_t;
#endif

typedef struct {
	int len;
	uint8_t *buf;
	int buf_len;
	int buffer_owned;
} m_fpga_transfer_batch;

m_fpga_transfer_batch m_new_fpga_transfer_batch();
void m_free_fpga_transfer_batch(m_fpga_transfer_batch batch);

int m_send_bytes_to_fpga(uint8_t *buf, int n);
int m_send_byte_to_fpga(uint8_t byte);

int m_fpga_send_byte(uint8_t byte);

void m_fpga_set_input_gain (float gain_db);
void m_fpga_set_output_gain(float gain_db);
void m_fpga_commit_reg_updates();

int m_fpga_batch_append   (m_fpga_transfer_batch *seq, uint8_t  x);
int m_fpga_batch_append_16(m_fpga_transfer_batch *seq, uint16_t x);
int m_fpga_batch_append_24(m_fpga_transfer_batch *seq, uint32_t x);
int m_fpga_batch_append_32(m_fpga_transfer_batch *seq, uint32_t x);

int m_fpga_transfer_batch_send(m_fpga_transfer_batch batch);

int m_fpga_spi_init();

// Floating point -> fixed point format conversion
m_fpga_sample_t float_to_q_nminus1(float x, int shift);
int16_t float_to_q15(float x);

char *m_fpga_command_to_string(int command);

#endif

#include <freertos/FreeRTOS.h>
#include <driver/spi_master.h>
#include <string.h>
#include <math.h>

#include "m_error_codes.h"
#include "m_fpga_io.h"
#include "m_hfunc.h"
#include "m_alloc.h"

#define PIN_NUM_MISO  	14
#define PIN_NUM_MOSI  	6
#define PIN_NUM_CLK   	5
#define PIN_NUM_CS		4

spi_device_handle_t spi_handle;

QueueHandle_t m_fpga_send_queue;

int16_t float_to_q_nminus1(float x, int shift)
{
    int n = (M_FPGA_DATA_WIDTH - 1) - shift;

    float scale = (float)(1 << n);

    float max =  (float)((1 << (M_FPGA_DATA_WIDTH - 1)) - 1) / scale;
    float min = -(float)(1  << (M_FPGA_DATA_WIDTH - 1))       / scale;

    if (x > max) x = max;
    if (x < min) x = min;

    return (m_fpga_sample_t)lrintf(x * scale);
}


int16_t float_to_q15(float x)
{
	if (x >= 0.999969482421875f) return  32767;
    if (x <= -1.0f)              return -32768;
    
    return (int16_t)lrintf(x * 32768.0f);
}

int m_fpga_spi_init()
{
    esp_err_t ret;

    spi_bus_config_t buscfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK)
        return ERR_SPI_FAIL;

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 4,
    };

    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK)
        return ERR_SPI_FAIL;
    
    return NO_ERROR;
}

int m_fpga_txrx(uint8_t *tx, uint8_t *rx, size_t len)
{
	if (len == 0)
		return 0;
		
	uint8_t reply_buf[len];
	
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = tx,
        .rx_buffer = reply_buf,
    };

	esp_err_t err = spi_device_transmit(spi_handle, &t);
	
	#ifdef PRINT_SPI_BYTES
	char print_buf[len * 10 + 1];
	int buf_index = 0;
	for (int i = 0; i < len; i++)
	{
		sprintf(&print_buf[buf_index], "SEND 0x%02x\n", tx[i]);
		buf_index += 10;
	}
	print_buf[buf_index] = 0;
	printf("\n\n\n\n\n\n\n\n\n\n\n\n\nSPI send to FPGA (%d bytes):\n", len);
	printf(print_buf);
	
	printf("FPGA replied with\n");
	buf_index = 0;
	for (int i = 0; i < len; i++)
	{
		sprintf(&print_buf[buf_index], "RCVD 0x%02x\n", reply_buf[i]);
		buf_index += 10;
	}
	print_buf[buf_index] = 0;
	printf(print_buf);
	#endif
	
	return (err == ESP_OK) ? NO_ERROR : ERR_SPI_FAIL;
}

int m_fpga_send_byte(uint8_t byte)
{
	return m_fpga_txrx(&byte, NULL, 1);
}

m_fpga_transfer_batch m_new_fpga_transfer_batch()
{
	m_fpga_transfer_batch seq;
	
	seq.buf = m_alloc(sizeof(uint32_t) * M_FPGA_N_BLOCKS);
	seq.len = 0;
	seq.buf_len = (int)(sizeof(uint32_t) * M_FPGA_N_BLOCKS);
	seq.buffer_owned = 0;
	
	return seq;
}

void m_free_fpga_transfer_batch(m_fpga_transfer_batch batch)
{
	if (batch.buf && !batch.buffer_owned) m_free(batch.buf);
}

int m_fpga_batch_append(m_fpga_transfer_batch *seq, uint8_t x)
{
	if (!seq)
		return ERR_NULL_PTR;
	
	if (seq->len >= seq->buf_len)
	{
		uint8_t *new_ptr = m_realloc(seq->buf,(seq->buf_len * 2) * sizeof(uint8_t));
		
		if (!new_ptr)
			return ERR_ALLOC_FAIL;
		
		seq->buf = new_ptr;
		seq->buf_len *= 2;
	}
	
	seq->buf[seq->len++] = x;
	
	return NO_ERROR;
}


int m_fpga_batch_append_16(m_fpga_transfer_batch *seq, uint16_t x)
{
	uint8_t bytes[2];
	
	bytes[0] = range_bits(x, 8,  8);
	bytes[1] = range_bits(x, 8,  0);
	
	int ret_val;
	
	if ((ret_val = m_fpga_batch_append(seq, bytes[0])) != NO_ERROR)
		return ret_val;
	
	if ((ret_val = m_fpga_batch_append(seq, bytes[1])) != NO_ERROR)
		return ret_val;
	
	return ret_val;
}


int m_fpga_batch_append_24(m_fpga_transfer_batch *seq, uint32_t x)
{
	uint8_t bytes[3];
	
	bytes[0] = range_bits(x, 8,  16);
	bytes[1] = range_bits(x, 8,  8);
	bytes[2] = range_bits(x, 8,  0);
	
	int ret_val;
	
	if ((ret_val = m_fpga_batch_append(seq, bytes[0])) != NO_ERROR)
		return ret_val;
	
	if ((ret_val = m_fpga_batch_append(seq, bytes[1])) != NO_ERROR)
		return ret_val;
	
	if ((ret_val = m_fpga_batch_append(seq, bytes[2])) != NO_ERROR)
		return ret_val;
	
	return ret_val;
}

int m_fpga_batch_append_32(m_fpga_transfer_batch *seq, uint32_t x)
{
	uint8_t bytes[4];
	
	bytes[0] = range_bits(x, 8, 24);
	bytes[1] = range_bits(x, 8, 16);
	bytes[2] = range_bits(x, 8,  8);
	bytes[3] = range_bits(x, 8,  0);
	
	int ret_val;
	
	if ((ret_val = m_fpga_batch_append(seq, bytes[0])) != NO_ERROR)
		return ret_val;
	
	if ((ret_val = m_fpga_batch_append(seq, bytes[1])) != NO_ERROR)
		return ret_val;
	
	if ((ret_val = m_fpga_batch_append(seq, bytes[2])) != NO_ERROR)
		return ret_val;
	
	if ((ret_val = m_fpga_batch_append(seq, bytes[3])) != NO_ERROR)
		return ret_val;
	
	return ret_val;
}

int m_fpga_batch_concat(m_fpga_transfer_batch *seq, m_fpga_transfer_batch *seq2)
{
	if (!seq || !seq2)
		return ERR_NULL_PTR;
	
	if (!seq2->buf)
		return ERR_BAD_ARGS;
	
	if (!seq->buf)
	{
		seq->buf = m_alloc(seq2->len);
		
		if (!seq->buf)
			return ERR_ALLOC_FAIL;
		
		seq->buf_len = seq2->len;
		seq->len = 0;
	}
	else if (seq->len + seq2->len > seq->buf_len)
	{
		int new_len = seq->buf_len;
		while (new_len < seq->len + seq2->len)
			new_len *= 2;
		
		uint8_t *new_ptr = m_realloc(seq->buf, new_len * sizeof(uint8_t));
		
		if (!new_ptr) return ERR_ALLOC_FAIL;
		
		seq->buf_len = new_len;
		seq->buf = new_ptr;
	}
	
	for (int i = 0; i < seq2->len; i++)
		seq->buf[seq->len + i] = seq2->buf[i];
	
	seq->len += seq2->len;
	
	return 0;
}

int m_fpga_transfer_batch_send(m_fpga_transfer_batch batch)
{
	#ifdef PRINT_TRANSFER_BATCHES
	m_fpga_batch_print(batch);
	#endif
	
	return m_fpga_txrx(batch.buf, NULL, batch.len);
}

void m_fpga_set_input_gain(float gain_db)
{
	float v = powf(10, gain_db / 20.0);
	uint16_t s = float_to_q_nminus1(v, 5);
	
	m_fpga_send_byte(COMMAND_SET_INPUT_GAIN);
	m_fpga_send_byte((s & 0xFF00) >> 8);
	m_fpga_send_byte(s & 0x00FF);
}

void m_fpga_set_output_gain(float gain_db)
{
	float v = powf(10, gain_db / 20.0);
	uint16_t s = float_to_q_nminus1(v, 5);
	
	m_fpga_send_byte(COMMAND_SET_OUTPUT_GAIN);
	m_fpga_send_byte((s & 0xFF00) >> 8);
	m_fpga_send_byte(s & 0x00FF);
}


void m_fpga_commit_reg_updates()
{
	m_fpga_send_byte(COMMAND_COMMIT_REG_UPDATES);
}

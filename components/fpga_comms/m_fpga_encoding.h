#ifndef M_ENCODE_H_
#define M_ENCODE_H_

#define    ZERO_REGISTER_ADDR   2
#define POS_ONE_REGISTER_ADDR  	3
#define NEG_ONE_REGISTER_ADDR   4

#define INSTR_FORMAT_A 0
#define INSTR_FORMAT_B 1

#define NO_SHIFT 255

int m_fpga_batch_append_block_number(m_fpga_transfer_batch *batch, int block);

uint32_t m_block_instr_encode_resource_aware(m_block *block, const m_eff_resource_report *res);

int m_fpga_batch_append_transformer(m_fpga_transfer_batch *batch, m_transformer *trans, m_eff_resource_report *res, int *pos);
int m_fpga_batch_append_transformers(m_fpga_transfer_batch *batch, m_transformer_pll *list, m_eff_resource_report *res, int *pos);

#endif

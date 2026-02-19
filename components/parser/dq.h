#ifndef DQ_H_
#define DQ_H_

#include "parameter.h"

#define M_FPGA_SAMPLE_RATE 44100

#define M_DERIVED_QUANTITY_CONST_FLT 	0
#define M_DERIVED_QUANTITY_REFERENCE 	1
#define M_DERIVED_QUANTITY_NEG		 	2
#define M_DERIVED_QUANTITY_FCALL_ADD 	3
#define M_DERIVED_QUANTITY_FCALL_SUB 	4
#define M_DERIVED_QUANTITY_FCALL_MUL 	5
#define M_DERIVED_QUANTITY_FCALL_DIV 	6
#define M_DERIVED_QUANTITY_FCALL_ABS 	7
#define M_DERIVED_QUANTITY_FCALL_SQR 	8
#define M_DERIVED_QUANTITY_FCALL_SQRT 	9
#define M_DERIVED_QUANTITY_FCALL_EXP 	10
#define M_DERIVED_QUANTITY_FCALL_LN 	11
#define M_DERIVED_QUANTITY_FCALL_POW 	12
#define M_DERIVED_QUANTITY_FCALL_SIN 	13
#define M_DERIVED_QUANTITY_FCALL_SINH 	14
#define M_DERIVED_QUANTITY_FCALL_COS 	15
#define M_DERIVED_QUANTITY_FCALL_COSH 	16
#define M_DERIVED_QUANTITY_FCALL_TAN 	17
#define M_DERIVED_QUANTITY_FCALL_TANH 	18
#define M_DERIVED_QUANTITY_FCALL_ASIN 	19
#define M_DERIVED_QUANTITY_FCALL_ACOS 	20
#define M_DERIVED_QUANTITY_FCALL_ATAN 	21

#define M_DERIVED_QUANTITY_TYPE_MAX_VAL M_DERIVED_QUANTITY_FCALL_TANH

typedef struct m_derived_quantity
{
	int type;
	int constant;
	int cached;
	float cached_val;
	union {
		float val_float;
		char *ref_name;
		struct m_derived_quantity **sub_dqs;
	} val;
} m_derived_quantity;

m_derived_quantity *new_m_derived_quantity_from_string(char *str);
m_derived_quantity *new_m_derived_quantity_from_tokens(m_token_ll *tokens, m_token_ll *tokens_end);

int m_derived_quantity_references_param(m_derived_quantity *dq, m_parameter *param);
float m_derived_quantity_compute(m_derived_quantity *dq, m_parameter_pll *params);

m_derived_quantity *new_m_derived_quantity_const_float(float v);

float m_dq_min(m_derived_quantity *dq, m_parameter_pll *params);
float m_dq_max(m_derived_quantity *dq, m_parameter_pll *params);

typedef struct {
	float a;
	float b;
} m_interval;

m_interval m_interval_real_line();
m_interval m_interval_ab(float a, float b);
m_interval m_interval_a_(float a);
m_interval m_interval__b(float b);
m_interval m_interval_singleton(float v);

m_interval m_derived_quantity_compute_range(m_derived_quantity *dq, m_parameter_pll *params);

#endif

#include "kest_int.h"

static const char *FNAME = "kest_fpga_instr.c";
#define PRINTLINES_ALLOWED 0

static const kest_instr_arg_fmt arg_format_std_0 = {
	.n_args = 0,
	
	.arg_a_pos = KEST_ARG_POS_NONE,
	.arg_b_pos = KEST_ARG_POS_NONE,
	.arg_c_pos = KEST_ARG_POS_NONE,
	.dest_pos  = KEST_ARG_POS_NONE,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_std_1 = {
	.n_args = 2,
	
	.arg_a_pos = 0,
	.arg_b_pos = KEST_ARG_POS_NONE,
	.arg_c_pos = KEST_ARG_POS_NONE,
	.res_pos   = KEST_ARG_POS_NONE,
	.dest_pos  = 1,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_std_2 = {
	.n_args = 3,
	
	.arg_a_pos = 0,
	.arg_b_pos = 1,
	.arg_c_pos = KEST_ARG_POS_NONE,
	.res_pos   = KEST_ARG_POS_NONE,
	.dest_pos  = 2,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_mac = {
	.n_args = 2,
	
	.arg_a_pos = 0,
	.arg_b_pos = 1,
	.arg_c_pos = KEST_ARG_POS_NONE,
	.res_pos   = KEST_ARG_POS_NONE,
	.dest_pos  = KEST_ARG_POS_NONE,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_add = {
	.n_args = 3,
	
	.arg_a_pos = 0,
	.arg_b_pos = KEST_ARG_POS_NONE,
	.arg_c_pos = 1,
	.res_pos   = KEST_ARG_POS_NONE,
	.dest_pos  = 2,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_sub = {
	.n_args = 3,
	
	.arg_a_pos = 1,
	.arg_b_pos = KEST_ARG_POS_NONE,
	.arg_c_pos = 0,
	.res_pos   = KEST_ARG_POS_NONE,
	.dest_pos  = 2,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_std_3 = {
	.n_args = 4,
	
	.arg_a_pos = 0,
	.arg_b_pos = 1,
	.arg_c_pos = 2,
	.res_pos   = KEST_ARG_POS_NONE,
	.dest_pos  = 3,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_shift = {
	.n_args = 3,
	
	.arg_a_pos = 0,
	.arg_b_pos = 1,
	.arg_c_pos = KEST_ARG_POS_NONE,
	.res_pos   = KEST_ARG_POS_NONE,
	.dest_pos  = 3,
	.shift_pos = 2
};

static const kest_instr_arg_fmt arg_format_read = {
	.n_args = 1,
	
	.arg_a_pos = KEST_ARG_POS_NONE,
	.arg_b_pos = KEST_ARG_POS_NONE,
	.arg_c_pos = KEST_ARG_POS_NONE,
	.res_pos   = KEST_ARG_POS_NONE,
	.dest_pos  = 0,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_res_read = {
	.n_args = 2,
	
	.arg_a_pos = KEST_ARG_POS_NONE,
	.arg_b_pos = KEST_ARG_POS_NONE,
	.arg_c_pos = KEST_ARG_POS_NONE,
	.res_pos   = 0,
	.dest_pos  = 1,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_res_read_2 = {
	.n_args = 3,
	
	.arg_a_pos = 1,
	.arg_b_pos = KEST_ARG_POS_NONE,
	.arg_c_pos = KEST_ARG_POS_NONE,
	.res_pos   = 0,
	.dest_pos  = 2,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_res_read_3 = {
	.n_args = 4,
	
	.arg_a_pos = 1,
	.arg_b_pos = 2,
	.arg_c_pos = KEST_ARG_POS_NONE,
	.res_pos   = 0,
	.dest_pos  = 3,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_res_write = {
	.n_args = 2,
	
	.arg_a_pos = 0,
	.arg_b_pos = KEST_ARG_POS_NONE,
	.arg_c_pos = KEST_ARG_POS_NONE,
	.res_pos   = 1,
	.dest_pos  = KEST_ARG_POS_NONE,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_write_svf_write = {
	.n_args = 3,
	
	.arg_a_pos = 0,
	.arg_b_pos = 1,
	.arg_c_pos = 2,
	.res_pos   = KEST_ARG_POS_NONE,
	.dest_pos  = KEST_ARG_POS_NONE,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_write_svf_read = {
	.n_args = 1,
	
	.arg_a_pos = KEST_ARG_POS_NONE,
	.arg_b_pos = KEST_ARG_POS_NONE,
	.arg_c_pos = KEST_ARG_POS_NONE,
	.res_pos   = KEST_ARG_POS_NONE,
	.dest_pos  = 0,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_instr_arg_fmt arg_format_res_rw = {
	.n_args = 3,
	
	.arg_a_pos = 0,
	.arg_b_pos = KEST_ARG_POS_NONE,
	.arg_c_pos = KEST_ARG_POS_NONE,
	.res_pos   = 1,
	.dest_pos  = 2,
	.shift_pos = KEST_ARG_POS_NONE
};

static const kest_asm_instr_desc kest_instr_desc_nop = {
	.name = "nop",
	.opcode = BLOCK_INSTR_NOP,
	.arg_fmt = arg_format_std_0,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_mov = {
	.name = "mov",
	.opcode = BLOCK_INSTR_MADD,
	.arg_fmt = arg_format_std_1,
	.shift_policy = SHIFT_POLICY_1
};


static const kest_asm_instr_desc kest_instr_desc_add = {
	.name = "add",
	.opcode = BLOCK_INSTR_MADD,
	.arg_fmt = arg_format_add,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_sub = {
	.name = "sub",
	.opcode = BLOCK_INSTR_MADD,
	.arg_fmt = arg_format_sub,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_mul = {
	.name = "mul",
	.opcode = BLOCK_INSTR_MADD,
	.arg_fmt = arg_format_std_2,
	.shift_policy = SHIFT_POLICY_SFAB
};


static const kest_asm_instr_desc kest_instr_desc_madd = {
	.name = "madd",
	.opcode = BLOCK_INSTR_MADD,
	.arg_fmt = arg_format_std_3,
	.shift_policy = SHIFT_POLICY_SFAB
};


static const kest_asm_instr_desc kest_instr_desc_arsh = {
	.name = "arsh",
	.opcode = BLOCK_INSTR_ARSH,
	.arg_fmt = arg_format_shift,
	.shift_policy = SHIFT_POLICY_SET
};


static const kest_asm_instr_desc kest_instr_desc_lsh = {
	.name = "lsh",
	.opcode = BLOCK_INSTR_LSH,
	.arg_fmt = arg_format_shift,
	.shift_policy = SHIFT_POLICY_SET
};


static const kest_asm_instr_desc kest_instr_desc_rsh = {
	.name = "rsh",
	.opcode = BLOCK_INSTR_RSH,
	.arg_fmt = arg_format_shift,
	.shift_policy = SHIFT_POLICY_SET
};


static const kest_asm_instr_desc kest_instr_desc_abs = {
	.name = "abs",
	.opcode = BLOCK_INSTR_ABS,
	.arg_fmt = arg_format_std_1,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_min = {
	.name = "min",
	.opcode = BLOCK_INSTR_MIN,
	.arg_fmt = arg_format_std_2,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_max = {
	.name = "max",
	.opcode = BLOCK_INSTR_MAX,
	.arg_fmt = arg_format_std_2,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_clamp = {
	.name = "clamp",
	.opcode = BLOCK_INSTR_CLAMP,
	.arg_fmt = arg_format_std_3,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_mov_acc = {
	.name = "mov_acc",
	.opcode = BLOCK_INSTR_MOV_ACC,
	.arg_fmt = arg_format_read,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_mov_uacc = {
	.name = "mov_uacc",
	.opcode = BLOCK_INSTR_MOV_UACC,
	.arg_fmt = arg_format_read,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_mov_lacc = {
	.name = "mov_lacc",
	.opcode = BLOCK_INSTR_MOV_LACC,
	.arg_fmt = arg_format_read,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_macz = {
	.name = "macz",
	.opcode = BLOCK_INSTR_MACZ,
	.arg_fmt = arg_format_mac,
	.shift_policy = SHIFT_POLICY_SFAB
};


static const kest_asm_instr_desc kest_instr_desc_umacz = {
	.name = "umacz",
	.opcode = BLOCK_INSTR_UMACZ,
	.arg_fmt = arg_format_mac,
	.shift_policy = SHIFT_POLICY_SFAB
};


static const kest_asm_instr_desc kest_instr_desc_mac = {
	.name = "mac",
	.opcode = BLOCK_INSTR_MAC,
	.arg_fmt = arg_format_mac,
	.shift_policy = SHIFT_POLICY_SFAB
};


static const kest_asm_instr_desc kest_instr_desc_umac = {
	.name = "umac",
	.opcode = BLOCK_INSTR_UMAC,
	.arg_fmt = arg_format_mac,
	.shift_policy = SHIFT_POLICY_SFAB
};


static const kest_asm_instr_desc kest_instr_desc_delay_read = {
	.name = "delay_read",
	.opcode = BLOCK_INSTR_DELAY_READ,
	.arg_fmt = arg_format_res_read,
	.shift_policy = SHIFT_POLICY_0
};

static const kest_asm_instr_desc kest_instr_desc_delay_mread = {
	.name = "delay_mread",
	.opcode = BLOCK_INSTR_DELAY_READ,
	.arg_fmt = arg_format_res_read_3,
	.shift_policy = SHIFT_POLICY_F0
};

static const kest_asm_instr_desc kest_instr_desc_delay_write = {
	.name = "delay_write",
	.opcode = BLOCK_INSTR_DELAY_WRITE,
	.arg_fmt = arg_format_res_write,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_mem_read = {
	.name = "mem_read",
	.opcode = BLOCK_INSTR_MEM_READ,
	.arg_fmt = arg_format_res_read,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_mem_write = {
	.name = "mem_write",
	.opcode = BLOCK_INSTR_MEM_WRITE,
	.arg_fmt = arg_format_res_write,
	.shift_policy = SHIFT_POLICY_0
};


static const kest_asm_instr_desc kest_instr_desc_filter = {
	.name = "filter",
	.opcode = BLOCK_INSTR_FILTER,
	.arg_fmt = arg_format_res_rw,
	.shift_policy = SHIFT_POLICY_0
};

static const kest_asm_instr_desc kest_instr_desc_fcasc = {
	.name = "fcasc",
	.opcode = BLOCK_INSTR_FCASC,
	.arg_fmt = arg_format_res_read,
	.shift_policy = SHIFT_POLICY_0
};

static const kest_asm_instr_desc kest_instr_desc_svf = {
	.name = "svf",
	.opcode = BLOCK_INSTR_SVF,
	.arg_fmt = arg_format_write_svf_write,
	.shift_policy = SHIFT_POLICY_FC
};

static const kest_asm_instr_desc kest_instr_desc_svf_low = {
	.name = "svf_low",
	.opcode = BLOCK_INSTR_SVF_LOW,
	.arg_fmt = arg_format_write_svf_read,
	.shift_policy = SHIFT_POLICY_0
};

static const kest_asm_instr_desc kest_instr_desc_svf_high = {
	.name = "svf_high",
	.opcode = BLOCK_INSTR_SVF_HIGH,
	.arg_fmt = arg_format_write_svf_read,
	.shift_policy = SHIFT_POLICY_0
};

static const kest_asm_instr_desc kest_instr_desc_svf_band = {
	.name = "svf_band",
	.opcode = BLOCK_INSTR_SVF_BAND,
	.arg_fmt = arg_format_write_svf_read,
	.shift_policy = SHIFT_POLICY_0
};

static const kest_asm_instr_desc kest_instr_desc_lut_read = {
	.name = "lut_read",
	.opcode = BLOCK_INSTR_LUT_READ,
	.arg_fmt = arg_format_res_read_2,
	.shift_policy = SHIFT_POLICY_0
};

static const kest_asm_instr_desc kest_instr_desc_tanh4 = {
	.name = "tanh4",
	.opcode = BLOCK_INSTR_LUT_READ,
	.arg_fmt = arg_format_std_1,
	.shift_policy = SHIFT_POLICY_0
};

static const kest_asm_instr_desc kest_instr_desc_sin2pi = {
	.name = "sin2pi",
	.opcode = BLOCK_INSTR_LUT_READ,
	.arg_fmt = arg_format_std_1,
	.shift_policy = SHIFT_POLICY_0
};

static const kest_asm_instr_desc kest_instr_desc_poly = {
	.name = "poly",
	.opcode = BLOCK_INSTR_POLY,
	.arg_fmt = arg_format_res_rw,
	.shift_policy = SHIFT_POLICY_0
};

const kest_asm_instr_desc *kest_instr_name_to_desc(char *name)
{
	if (!name)
		return NULL;
	
	if (strcmp(name, "nop"         ) == 0) return &kest_instr_desc_nop;
	if (strcmp(name, "mov"         ) == 0) return &kest_instr_desc_mov;
	if (strcmp(name, "add"         ) == 0) return &kest_instr_desc_add;
	if (strcmp(name, "sub"         ) == 0) return &kest_instr_desc_sub;
	if (strcmp(name, "mul"         ) == 0) return &kest_instr_desc_mul;
	if (strcmp(name, "madd"        ) == 0) return &kest_instr_desc_madd;
	if (strcmp(name, "arsh"        ) == 0) return &kest_instr_desc_arsh;
	if (strcmp(name, "lsh"         ) == 0) return &kest_instr_desc_lsh;
	if (strcmp(name, "rsh"         ) == 0) return &kest_instr_desc_rsh;
	if (strcmp(name, "abs"         ) == 0) return &kest_instr_desc_abs;
	if (strcmp(name, "min"         ) == 0) return &kest_instr_desc_min;
	if (strcmp(name, "max"         ) == 0) return &kest_instr_desc_max;
	if (strcmp(name, "clamp"       ) == 0) return &kest_instr_desc_clamp;
	if (strcmp(name, "mov_acc"     ) == 0) return &kest_instr_desc_mov_acc;
	if (strcmp(name, "mov_lacc"    ) == 0) return &kest_instr_desc_mov_lacc;
	if (strcmp(name, "mov_uacc"    ) == 0) return &kest_instr_desc_mov_uacc;
	if (strcmp(name, "macz"        ) == 0) return &kest_instr_desc_macz;
	if (strcmp(name, "umacz"       ) == 0) return &kest_instr_desc_umacz;
	if (strcmp(name, "mac"         ) == 0) return &kest_instr_desc_mac;
	if (strcmp(name, "umac"        ) == 0) return &kest_instr_desc_umac;
	if (strcmp(name, "delay_read"  ) == 0) return &kest_instr_desc_delay_read;
	if (strcmp(name, "delay_mread" ) == 0) return &kest_instr_desc_delay_mread;
	if (strcmp(name, "delay_write" ) == 0) return &kest_instr_desc_delay_write;
	if (strcmp(name, "mem_read"    ) == 0) return &kest_instr_desc_mem_read;
	if (strcmp(name, "mem_write"   ) == 0) return &kest_instr_desc_mem_write;
	if (strcmp(name, "filter"      ) == 0) return &kest_instr_desc_filter;
	if (strcmp(name, "fcasc"       ) == 0) return &kest_instr_desc_fcasc;
	if (strcmp(name, "svf"         ) == 0) return &kest_instr_desc_svf;
	if (strcmp(name, "svf_low"     ) == 0) return &kest_instr_desc_svf_low;
	if (strcmp(name, "svf_high"    ) == 0) return &kest_instr_desc_svf_high;
	if (strcmp(name, "svf_band"    ) == 0) return &kest_instr_desc_svf_band;
	if (strcmp(name, "tanh4"       ) == 0) return &kest_instr_desc_tanh4;
	if (strcmp(name, "sin2pi"      ) == 0) return &kest_instr_desc_sin2pi;
	if (strcmp(name, "poly"        ) == 0) return &kest_instr_desc_poly;
	
	return NULL;
}

const kest_asm_instr_desc *kest_instr_opcode_to_desc(int opcode)
{
	switch (opcode)
	{
		case BLOCK_INSTR_NOP: 			return &kest_instr_desc_nop;
		case BLOCK_INSTR_MADD: 			return &kest_instr_desc_madd;
		case BLOCK_INSTR_ARSH:			return &kest_instr_desc_arsh;
		case BLOCK_INSTR_LSH: 			return &kest_instr_desc_lsh;
		case BLOCK_INSTR_RSH: 			return &kest_instr_desc_rsh;
		case BLOCK_INSTR_ABS: 			return &kest_instr_desc_abs;
		case BLOCK_INSTR_MIN: 			return &kest_instr_desc_min;
		case BLOCK_INSTR_MAX: 			return &kest_instr_desc_max;
		case BLOCK_INSTR_CLAMP: 		return &kest_instr_desc_clamp;
		case BLOCK_INSTR_MOV_ACC: 		return &kest_instr_desc_mov_acc;
		case BLOCK_INSTR_MOV_LACC: 		return &kest_instr_desc_mov_lacc;
		case BLOCK_INSTR_MOV_UACC: 		return &kest_instr_desc_mov_uacc;
		case BLOCK_INSTR_MACZ: 			return &kest_instr_desc_macz;
		case BLOCK_INSTR_UMACZ: 		return &kest_instr_desc_umacz;
		case BLOCK_INSTR_MAC: 			return &kest_instr_desc_mac;
		case BLOCK_INSTR_UMAC: 			return &kest_instr_desc_umac;
		case BLOCK_INSTR_DELAY_READ: 	return &kest_instr_desc_delay_read;
		case BLOCK_INSTR_DELAY_WRITE: 	return &kest_instr_desc_delay_write;
		case BLOCK_INSTR_MEM_READ: 		return &kest_instr_desc_mem_read;
		case BLOCK_INSTR_MEM_WRITE: 	return &kest_instr_desc_mem_write;
		case BLOCK_INSTR_FILTER: 		return &kest_instr_desc_filter;
		case BLOCK_INSTR_FCASC: 		return &kest_instr_desc_fcasc;
		case BLOCK_INSTR_LUT_READ: 		return &kest_instr_desc_lut_read;
		case BLOCK_INSTR_SVF: 			return &kest_instr_desc_svf;
		case BLOCK_INSTR_SVF_LOW: 		return &kest_instr_desc_svf_low;
		case BLOCK_INSTR_SVF_HIGH: 		return &kest_instr_desc_svf_high;
		case BLOCK_INSTR_SVF_BAND: 		return &kest_instr_desc_svf_band;
		case BLOCK_INSTR_POLY: 			return &kest_instr_desc_poly;
	}
	
	return NULL;
}

void kest_instr_print_format_fr(kest_string *str, 
	int opcode,
	int src_a,
	int src_a_reg,
	int src_b,
	int src_b_reg,
	int src_c,
	int src_c_reg,
	int dest,
	int shift,
	int res_addr)
{
	if (!str) return;
	
	const kest_asm_instr_desc *desc = kest_instr_opcode_to_desc(opcode);
	
	if (!desc)
	{
		kest_string_appendf(str, "unrecognised (0x%04x)", opcode);
		return;
	}
	
	int len = str->count;
	if (desc->name)
		kest_string_appendf(str, "%s", desc->name);
	else
		kest_string_appendf(str, "(no name... what? how? ptr is %p)", desc);
	len = str->count - len;
	
	while (len++ < 11)
		kest_string_append(str, ' ');
	
	for (int i = 0; i < INSTR_MAX_ARGS && i < desc->arg_fmt.n_args; i++)
	{
		if (i == desc->arg_fmt.arg_a_pos)
		{
			if (src_a_reg) kest_string_appendf(str, " r%d", src_a);
			else 		   kest_string_appendf(str, " c%d", src_a);
			
		}
		else if (i == desc->arg_fmt.arg_b_pos)
		{
			if (src_b_reg) kest_string_appendf(str, " r%d", src_b);
			else 		   kest_string_appendf(str, " c%d", src_b);
		}
		else if (i == desc->arg_fmt.arg_c_pos)
		{
			if (src_c_reg) kest_string_appendf(str, " r%d", src_c);
			else 		   kest_string_appendf(str, " c%d", src_c);
		}
		else if (i == desc->arg_fmt.res_pos)
		{
			kest_string_appendf(str, " $%d", res_addr);
		}
		else if (i == desc->arg_fmt.shift_pos)
		{
			kest_string_appendf(str, " %s%d", shift < 10 ? " " : "", shift);
		}
		else if (i == desc->arg_fmt.dest_pos)
		{
			kest_string_appendf(str, " c%d", dest);
		}
	}
}
	
void kest_instr_print_format_a(kest_string *str, uint32_t instr)
{
	if (!str) return;
	
	int opcode = range_bits(instr, 5, 0);
	
	int src_a 	 = range_bits(instr, 4, 6);
	int src_a_reg = !!(instr & (1 << 10));
	
	int src_b 	 = range_bits(instr, 4, 11);
	int src_b_reg = !!(instr & (1 << 15));
	
	int src_c 	 = range_bits(instr, 4, 16);
	int src_c_reg = !!(instr & (1 << 20));
	
	int dest = range_bits(instr, 4, 21);
	int shift = range_bits(instr, 5, 25);
	int sat = !!(instr & (1 << 30));
	int no_shift = !!(instr & (1 << 31));
	
	kest_instr_print_format_fr(str, 
		opcode,
		src_a,
		src_a_reg,
		src_b,
		src_b_reg,
		src_c,
		src_c_reg,
		dest,
		shift,
		0
	);
}

void kest_instr_print_format_b(kest_string *str, uint32_t instr)
{
	if (!str) return;
	
	int opcode = range_bits(instr, 5, 0);
	
	int src_a 	 = range_bits(instr, 4, 6);
	int src_a_reg = !!(instr & (1 << 10));
	
	int src_b 	 = range_bits(instr, 4, 11);
	int src_b_reg = !!(instr & (1 << 15));
	
	int dest = range_bits(instr, 4, 16);
	
	int res_addr = range_bits(instr, 8, 20);
	
	kest_instr_print_format_fr(str, 
		opcode,
		src_a,
		src_a_reg,
		src_b,
		src_b_reg,
		0,
		0,
		dest,
		0,
		res_addr);
}
void kest_instr_print(kest_string *str, uint32_t instr)
{
	if (!str) return;
	
	int format = !!(instr & (1 << 5));
	
	if (format)
		kest_instr_print_format_b(str, instr);
	else
		kest_instr_print_format_a(str, instr);
}

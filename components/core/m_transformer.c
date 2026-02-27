#include "m_int.h"

#define INITIAL_PARAMETER_ARRAY_LENGTH 	8
#define PARAMETER_ARRAY_CHUNK_SIZE	 	8

#define INITIAL_OPTION_ARRAY_LENGTH 	8
#define OPTION_ARRAY_CHUNK_SIZE	 		8

const char *TAG = "Transformer";

IMPLEMENT_LINKED_PTR_LIST(m_transformer);

const char *m_transformer_name(m_transformer *trans)
{
	if (!trans)
		return "(NULL)";
	
	if (!trans->eff)
		return "(Unknown)";
	
	return trans->eff->name;
}


int init_transformer(m_transformer *trans)
{
	if (!trans)
		return ERR_NULL_PTR;
	
	trans->id = 0;
	trans->type = 0;
	
	trans->position = 0;
	
	trans->parameters = NULL;
	trans->settings = NULL;
	trans->scope = NULL;
	
	#ifdef M_ENABLE_UI
	trans->view_page = NULL;
	#endif
	trans->profile = NULL;
	
	init_parameter(&trans->wet_mix, "Wet Mix", 1.0, 0.0, 1.0);
	trans->wet_mix.id.parameter_id = TRANSFORMER_WET_MIX_PID;
	
	init_setting(&trans->band_mode, "Apply to", TRANSFORMER_MODE_FULL_SPECTRUM);
	trans->band_mode.id.setting_id = TRANSFORMER_BAND_MODE_SID;
	
	trans->band_mode.n_options = 4;
	trans->band_mode.options = m_alloc(sizeof(m_setting_option) * trans->band_mode.n_options);
	
	if (!trans->band_mode.options)
		return ERR_ALLOC_FAIL;
	
	trans->band_mode.options[0].value = TRANSFORMER_MODE_FULL_SPECTRUM;
	trans->band_mode.options[0].name  = "All freq";
	
	trans->band_mode.options[1].value = TRANSFORMER_MODE_UPPER_SPECTRUM;
	trans->band_mode.options[1].name  = "Freq > cutoff";
	
	trans->band_mode.options[2].value = TRANSFORMER_MODE_LOWER_SPECTRUM;
	trans->band_mode.options[2].name  = "Freq < cutoff";
	
	trans->band_mode.options[3].value = TRANSFORMER_MODE_BAND;
	trans->band_mode.options[3].name  = "Freq in band";
	
	init_parameter(&trans->band_lp_cutoff, "Cutoff", 4000.0, 1, 4000);
	trans->band_lp_cutoff.scale = PARAMETER_SCALE_LOGARITHMIC;
	trans->band_lp_cutoff.id.parameter_id = TRANSFORMER_BAND_LP_CUTOFF_PID;
	
	init_parameter(&trans->band_hp_cutoff, "Cutoff", 1.0, 1, 4000);
	trans->band_hp_cutoff.scale = PARAMETER_SCALE_LOGARITHMIC;
	trans->band_hp_cutoff.id.parameter_id = TRANSFORMER_BAND_HP_CUTOFF_PID;
	
	trans->eff = NULL;
	
	#ifdef M_USE_FREERTOS
	trans->mutex = xSemaphoreCreateMutex();
	#endif
	
	return NO_ERROR;
}

int init_transformer_from_effect_desc(m_transformer *trans, m_effect_desc *eff)
{
	init_transformer(trans);
	trans->eff = eff;
	
	m_parameter_pll *current_param = eff->parameters;
	m_setting_pll *current_setting = eff->settings;
	
	while (current_param)
	{
		m_parameter_pll_safe_append(&trans->parameters, m_parameter_make_clone(current_param->data));
		current_param = current_param->next;
	}
	
	m_setting *setting;
	
	while (current_setting)
	{
		m_setting_pll_safe_append(&trans->settings, m_setting_make_clone(current_setting->data));
		current_setting = current_setting->next;
	}
	
	trans->scope = m_transformer_create_scope(trans);
	
	return NO_ERROR;
}

int transformer_rectify_param_ids(m_transformer *trans)
{
	if (!trans)
		return ERR_NULL_PTR;
	
	m_parameter_pll *current_param = trans->parameters;
	
	while (current_param)
	{
		if (current_param->data)
		{
			if (trans->profile)
				current_param->data->id.profile_id = trans->profile->id;
			current_param->data->id.transformer_id = trans->id;
		}
		
		current_param = current_param->next;
	}
	
	setting_ll *current_setting = trans->settings;
	
	while (current_setting)
	{
		if (current_setting->data)
		{
			if (trans->profile)
				current_setting->data->id.profile_id = trans->profile->id;
			current_setting->data->id.transformer_id = trans->id;
		}
		
		current_setting = current_setting->next;
	}
	
	if (trans->profile)
		trans->band_mode.id.profile_id = trans->profile->id;
	trans->band_mode.id.transformer_id = trans->id;
	
	if (trans->profile)
		trans->band_lp_cutoff.id.profile_id = trans->profile->id;
	trans->band_lp_cutoff.id.transformer_id = trans->id;
	
	if (trans->profile)
		trans->band_hp_cutoff.id.profile_id = trans->profile->id;
	trans->band_hp_cutoff.id.transformer_id = trans->id;
	
	return NO_ERROR;
}

int transformer_set_id(m_transformer *trans, uint16_t profile_id, uint16_t transformer_id)
{
	if (!trans)
		return ERR_NULL_PTR;
	
	trans->id = transformer_id;
	
	transformer_rectify_param_ids(trans);
	
	return NO_ERROR;
}

int request_append_transformer(uint16_t type, m_transformer *local)
{
	#ifdef USE_TEENSY
	if (!local)
		return ERR_NULL_PTR;
	
	if (!local->profile)
		return ERR_BAD_ARGS;
	
	m_message msg = create_m_message(M_MESSAGE_APPEND_TRANSFORMER, "ss", local->profile->id, local->type);
	msg.callback = transformer_receive_id;
	msg.cb_arg = local;
	
	return queue_msg_to_teensy(msg);
	#endif
	
	return NO_ERROR;
}

#ifdef USE_TEENSY
void transformer_receive_id(m_message message, m_response response)
{
	printf("Transformer receive ID!\n");
	m_transformer *trans = message.cb_arg;
	
	if (!trans)
		return;
	
	uint16_t pid, tid;
	
	memcpy(&pid, &response.data[0], sizeof(uint16_t));
	memcpy(&tid, &response.data[2], sizeof(uint16_t));
	
	if (!trans->profile || pid != trans->profile->id)
	{
		ESP_LOGE(TAG, "Transformer ID for transformer in profile %d sent to transformer in %d\n", pid, trans->profile->id);
	}
	else
	{
		printf("Transformer %p obtains id %d.%d\n", trans, pid, tid);
		trans->id = tid;
		
		transformer_rectify_param_ids(trans);
	}
}
#endif

m_parameter *transformer_add_parameter(m_transformer *trans)
{
	if (!trans)
		return NULL;
	
	int ret_val;
	
	m_parameter *param = m_alloc(sizeof(m_parameter));
	
	if (!param)
		return NULL;
	
	init_parameter_str(param);
	
	m_parameter_pll *nl = m_parameter_pll_append(trans->parameters, param);
	
	if (!nl)
	{
		m_free(param);
		return NULL;
	}
	
	trans->parameters = nl;
	
	return param;
}

m_setting *transformer_add_setting(m_transformer *trans)
{
	if (!trans)
		return NULL;
	
	int ret_val;
	
	m_setting *setting = m_alloc(sizeof(m_setting));
	
	if (!setting)
		return NULL;
	
	init_setting_str(setting);
	
	setting_ll *nl = m_setting_pll_append(trans->settings, setting);
	
	if (!nl)
	{
		m_free(setting);
		return NULL;
	}
	
	trans->settings = nl;
	
	return setting;
}

#ifdef M_ENABLE_UI
int transformer_init_ui_page(m_transformer *trans, m_ui_page *parent)
{
	printf("transformer_init_ui_page. trans = %p, parent = %p\n", trans, parent);
	if (!trans)
		return ERR_NULL_PTR;
	
	trans->view_page = m_alloc(sizeof(m_ui_page));
	
	if (!trans->view_page)
		return ERR_ALLOC_FAIL;
	
	init_ui_page(trans->view_page);
	init_transformer_view(trans->view_page);
	configure_transformer_view(trans->view_page, trans);
	trans->view_page->parent = parent;
	
	return NO_ERROR;
}
#endif

int clone_transformer(m_transformer *dest, m_transformer *src)
{
	if (!src || !dest)
		return ERR_NULL_PTR;
	
	uint16_t profile_id;
	uint16_t transformer_id;
	
	init_transformer(dest);
	
	dest->type = src->type;
	dest->position = src->position;
	
	m_parameter_pll *current_param = src->parameters;
	m_parameter *param;
	
	while (current_param)
	{
		if(current_param->data)
		{
			param = transformer_add_parameter(dest);
			
			if (param)
			{
				memcpy(param, current_param->data, sizeof(m_parameter));
			}
			else
			{
				return ERR_ALLOC_FAIL;
			}
		}
		
		current_param = current_param->next;
	}
	
	setting_ll *current_setting = src->settings;
	m_setting *setting;
	
	while (current_setting)
	{
		if(current_setting->data)
		{
			setting = transformer_add_setting(dest);
			
			if (setting)
			{
				clone_setting(setting, current_setting->data);
			}
			else
			{
				return ERR_ALLOC_FAIL;
			}
		}
		
		current_setting = current_setting->next;
	}
	
	#ifdef M_ENABLE_UI
	src->view_page = NULL;
	#endif
	
	return NO_ERROR;
}


void gut_transformer(m_transformer *trans)
{
	if (!trans)
		return;
	
	free_m_parameter_pll(trans->parameters);
	destructor_free_m_setting_pll(trans->settings, gut_setting);
	trans->parameters = NULL;
	trans->settings = NULL;
	
	gut_setting(&trans->band_mode);
	
	#ifdef M_ENABLE_UI
	free_transformer_view(trans->view_page);
	trans->view_page = NULL;
	#endif
	
	trans->id 		= 0;
	trans->type 	= 0;
	trans->position = 0;
}


void free_transformer(m_transformer *trans)
{
	if (!trans)
		return;
	
	m_free(trans->parameters);
	m_free(trans->settings);
	
	#ifdef M_ENABLE_UI
	free_transformer_view(trans->view_page);
	#endif
	
	m_free(trans);
}

m_parameter *transformer_get_parameter(m_transformer *trans, int n)
{
	if (!trans)
		return NULL;
	
	m_parameter_pll *current = trans->parameters;
	int i = 0;
	
	while (current && i < n)
	{
		current = current->next;
		i++;
	}
	
	return current ? current->data : NULL;
}


m_setting *transformer_get_setting(m_transformer *trans, int n)
{
	if (!trans)
		return NULL;
	
	setting_ll *current = trans->settings;
	int i = 0;
	
	while (current && i < n)
	{
		current = current->next;
		i++;
	}
	
	return current ? current->data : NULL;
}

int m_transformer_update_fpga_registers(m_transformer *trans)
{
	#ifdef M_ENABLE_FPGA

	if (!trans)
		return ERR_NULL_PTR;
	
	if (!trans->eff)
		return ERR_BAD_ARGS;
	
	m_fpga_transfer_batch batch = m_new_fpga_transfer_batch();
	
	if (!batch.buf)
		return ERR_ALLOC_FAIL;
	
	m_fpga_transfer_batch_append_effect_register_updates(&batch, trans->eff, trans->scope, trans->block_position);
	
	int ret_val = m_fpga_queue_transfer_batch(batch);
	
	return ret_val;
	#else
	return ERR_FEATURE_DISABLED;
	#endif
}


m_expr_scope *m_transformer_create_scope(m_transformer *trans)
{
	if (!trans)
		return NULL;
	
	m_expr_scope *scope = m_new_expr_scope();
	
	if (!scope)
		return NULL;
	
	m_parameter_pll *current_param = trans->parameters;
	m_setting_pll *current_setting = trans->settings;
	
	while (current_param)
	{
		if (current_param->data)
			m_expr_scope_add_param(scope, current_param->data);
		
		current_param = current_param->next;
	}
	
	while (current_setting)
	{
		if (current_setting->data)
			m_expr_scope_add_setting(scope, current_setting->data);
		
		current_setting = current_setting->next;
	}
	
	return scope;
}

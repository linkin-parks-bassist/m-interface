#include <sys/unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "kest_int.h"

#define PRINTLINES_ALLOWED 0

static const char *FNAME = "kest_files.c";

#define IO_BUFFER_SIZE 128

#define write_byte(x) 	do {		    fputc(x, file); POS++; } while(0)
#define write_short(x)  do { arg16 = x; fwrite(&arg16,                 1, 2, file); POS += 2;               } while (0)
#define write_int32(x)  do { arg32 = x; fwrite(&arg32,  sizeof( int32_t), 1, file); POS += sizeof( int32_t);} while (0)
#define write_uint32(x) do {uarg32 = x; fwrite(&uarg32, sizeof(uint32_t), 1, file); POS += sizeof(uint32_t);} while (0)
#define write_float(x) 	do {		    fwrite(&x,      sizeof(float),    1, file); POS += sizeof(float);   } while (0)
#define write_string(x) \
	do {if (x) {for (int a = 0; x[a] != 0; a++) {fputc(x[a], file); POS++;}} fputc(0, file); POS++;} while (0)

#define read_byte(x)   x = fgetc(file);
#define read_short(x)  fread(&x, sizeof(uint16_t), 1, file);
#define read_int32(x)  fread(&x, sizeof(int32_t),  1, file);
#define read_uint32(x) fread(&x, sizeof(uint32_t), 1, file);
#define read_float(x)  fread(&x, sizeof(float),    1, file);
#define read_string() \
	do {\
		for (int i = 0; i < IO_BUFFER_SIZE; i++)\
		{\
			string_read_buffer[i] = fgetc(file);\
			if (!string_read_buffer[i])\
				break;\
		}\
	} while (0);
#define read_and_strndup_string(x) \
	do {\
		for (int i = 0; i < IO_BUFFER_SIZE; i++)\
		{\
			string_read_buffer[i] = fgetc(file);\
			if (!string_read_buffer[i])\
				break;\
		}\
		x = kest_strndup(string_read_buffer, IO_BUFFER_SIZE);\
	} while (0);

//#define DUMP_PRESET_SAVE
//#define DUMP_PRESET_READ

//#define DUMP_SEQUENCE_SAVE
//#define DUMP_SEQUENCE_READ

//#define DUMP_STATE_SAVE
//#define DUMP_STATE_READ

void dump_file_contents(char *fname)
{
	KEST_PRINTF_FORCE("FILE HEX DUMP: %s\n", fname);
	FILE *file = fopen(fname, "rb");
	
	if (!file)
	{
		KEST_PRINTF_FORCE("Failed to open file %s\n", fname);
		return;
	}
	
	uint8_t byte;
	
	int i = 1;
	while (fread(&byte, 1, 1, file))
	{
		if (i % 8 == 1) KEST_PRINTF_FORCE_("\n %s%d | ", (i < 10) ? "  " : ((i < 100) ? " " : ""), i - 1);
		KEST_PRINTF_FORCE_("0x%02x ", byte);
		i++;
	}
	
	KEST_PRINTF_FORCE_((i % 8 == 1) ? "\n" : "\n\n");
	
	KEST_PRINTF_FORCE("Dump of \"%s\" finished. Total bytes: %d\n", fname, i - 1);
	fclose(file);
}

int file_validity_check(FILE *file, uint8_t magic_byte, uint8_t *byte_out)
{
	uint8_t byte;
	
	read_byte(byte);
	
	if (byte_out)
		*byte_out = byte;
	
	if (byte != magic_byte)
		return 1;
	
	read_byte(byte);
	
	if (byte_out)
		*byte_out = byte;
	
	if (byte != KEST_WRITE_FINISHED_BYTE)
		return 2;
	
	return 0;
}

int save_preset_as_file(kest_preset *preset, const char *fname)
{
	KEST_PRINTF("save_preset_as_file\n");
	
	if (!fname || !preset)
	{
		KEST_PRINTF("NULL pointer lol\n");
		return ERR_NULL_PTR;
	}
	
	FILE *file = fopen(fname, "wb");
	
	uint8_t len;
	int POS = 0;
	
	if (!file)
	{
		KEST_PRINTF("Could not open file %s\n", fname);
		return ERR_FOPEN_FAIL;
	}
	
	// Declare that this is a preset file
	write_byte(KEST_PRESET_MAGIC_BYTE);
	
	// Write status byte; overwritten at the end
	write_byte(KEST_WRITE_UNFINISHED_BYTE);
	
	uint8_t buffer[IO_BUFFER_SIZE];
	uint16_t arg16;
	int32_t arg32;
	int ret_val;
	int n;
	
	char *units;
	char *name = preset->name ? preset->name : "Unnamed Preset";
	
	write_string(name);
	
	kest_effect_pll *current_effect = preset->pipeline.effects;
	kest_parameter_pll *current_param;
	kest_setting_pll *current_setting;
	
	n = 0;
	
	while (current_effect)
	{
		current_effect = current_effect->next;
		n++;
	}
	
	write_short(n);
	
	current_effect = preset->pipeline.effects;
	
	while (current_effect)
	{
		if (!current_effect->data || !current_effect->data->eff)
		{
			write_short(KEST_PRESET_BROKEN_EFFECT);
			current_effect = current_effect->next;
			continue;
		}
		
		write_string(current_effect->data->eff->cname);
		write_short(current_effect->data->id);
		
		current_param = current_effect->data->parameters;
		
		while (current_param)
		{
			if (current_param->data)
				write_float(current_param->data->value);
			
			current_param = current_param->next;
		}
		
		current_setting = current_effect->data->settings;
		
		while (current_setting)
		{
			if (current_setting->data)
				write_int32(current_setting->data->value);
			
			current_setting = current_setting->next;
		}
		
		current_effect = current_effect->next;
	}
	
	fseek(file, 1, SEEK_SET);
	write_byte(KEST_WRITE_FINISHED_BYTE);
	
	fclose(file);
	
	KEST_PRINTF("save_preset_as_file done\n");
	
	return NO_ERROR;
}

int save_sequence_as_file(kest_sequence *sequence, const char *fname)
{
	if (!sequence || !fname)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("Saving sequence %s to sd card!\n", sequence->name ? "(unnamed)" : sequence->name);
	FILE *file = fopen(fname, "wb");
	
	if (!file)
	{
		KEST_PRINTF("Failed to open file %s\n", fname);
		return ERR_FOPEN_FAIL;
	}
	
	int POS = 0;
	
	write_byte(KEST_SEQUENCE_MAGIC_BYTE);
	
	write_byte(KEST_WRITE_UNFINISHED_BYTE);
	
	char *name = sequence->name ? sequence->name : "Unnamed Sequence";
	
	write_string(name);
	
	seq_kest_preset_pll *current = sequence->presets;
	
	uint16_t n_presets = 0;
	uint16_t arg16;
	
	while (current)
	{
		n_presets++;
		current = current->next;
	}
	
	KEST_PRINTF("Sequence has %d presets...\n", n_presets);
	write_short(n_presets);
	
	current = sequence->presets;
	while (current)
	{
		if (current->data)
		{
			if (!current->data->has_fname || current->data->unsaved_changes)
			{
				save_preset(current->data);
			}
			
			KEST_PRINTF("Preset %s...\n", current->data->fname);
			write_string(current->data->fname);
		}
		
		current = current->next;
	}
	
	fseek(file, 1, SEEK_SET);
	write_byte(KEST_WRITE_FINISHED_BYTE);
	
	fclose(file);
	
	KEST_PRINTF("Success\n");
	
	#ifdef DUMP_SEQUENCE_SAVE
	dump_file_contents(fname);
	#endif
	return NO_ERROR;
}

int save_state_to_file(kest_state *state, const char *fname)
{
	if (!state || !fname)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("Saving state to sd card!\n");
	FILE *file = fopen(fname, "wb");
	int32_t arg32;
	int POS = 0;
	
	if (!file)
	{
		KEST_PRINTF("Failed to open file %s\n", fname);
		return ERR_FOPEN_FAIL;
	}
	
	write_byte(KEST_STATE_MAGIC_BYTE);
	
	write_byte(KEST_WRITE_UNFINISHED_BYTE);
	
	write_float(state->input_gain);
	write_float(state->output_gain);
	
	write_string(state->active_preset_fname);
	write_string(state->active_sequence_fname);
	
	KEST_PRINTF("Write state->current_page.type = %d = 0x%08x, POS = %d\n", state->current_page.type, state->current_page.type, POS);
	write_int32(state->current_page.type);
	write_int32(state->current_page.id);
	write_string(state->current_page.fname);
	
	fseek(file, 1, SEEK_SET);
	write_byte(KEST_WRITE_FINISHED_BYTE);
	
	fclose(file);
	
	KEST_PRINTF("Success\n");
	
	#ifdef DUMP_STATE_SAVE
	dump_file_contents(fname);
	#endif
	
	return NO_ERROR;
}

int load_state_from_file(kest_state *state, const char *fname)
{
	if (!state || !fname)
		return ERR_NULL_PTR;
	
	KEST_PRINTF("load state from file %s...\n", fname);
	
	#ifdef DUMP_STATE_READ
	dump_file_contents(fname);
	#endif
	
	FILE *file = fopen(fname, "rb");
	
	char string_read_buffer[IO_BUFFER_SIZE];
	int ret_val = NO_ERROR;
	uint8_t byte;
	void *ptr;
	float f;
	int i;
	int j;
	
	if (!file)
	{
		KEST_PRINTF("Failed to open file %s\n", fname);
		return ERR_FOPEN_FAIL;
	}
	
	fseek(file, 0, SEEK_END);
	int file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	uint8_t *content = kest_alloc(file_size * sizeof(uint8_t));
	
	if (!content)
		return ERR_ALLOC_FAIL;
	
	fread(content, 1, file_size, file);
	fclose(file);
	
	byte = content[0];
	
	if (byte != KEST_STATE_MAGIC_BYTE)
	{
		ret_val = ERR_MANGLED_FILE;
		goto read_settings_exit;
	}
	
	byte = content[1];
	
	if (byte != KEST_WRITE_FINISHED_BYTE)
	{
		ret_val = ERR_MANGLED_FILE;
		goto read_settings_exit;
	}
	
	ptr = (void*)&content[2];
	
	state->input_gain  = *((float*)(&content[2]));
	KEST_PRINTF("Obtained input gain as %f = 0x%08x\n", state->input_gain, *((int*)(&content[2])));
	state->output_gain = *((float*)(&content[6]));
	KEST_PRINTF("Obtained output gain as %f = 0x%08x\n", state->output_gain, *((int*)(&content[6])));
	
	i = 10;
	KEST_PRINTF("Reading active preset fname from position %d\n", i);
	if (content[i])
	{
		j = 10;
		while (content[i] && i - j < 31)
		{
			KEST_PRINTF("\t0x%02x = '%c'\n", content[i], content[i]);
			state->active_preset_fname[i - j] = (char)content[i];
			i++;
		}
		state->active_preset_fname[i - j] = 0;
		
		i++;
	}
	else
	{
		state->active_preset_fname[0] = 0;
		i++;
	}
	
	KEST_PRINTF("Reading active sequence fname from position %d\n", i);
	if (content[i])
	{
		j = i;
		
		while (content[i] && i - j  < 31)
		{
			state->active_sequence_fname[i - j] = (char)content[i];
			i++;
		}
		state->active_sequence_fname[i - j] = 0;
		
		i++;
	}
	else
	{
		state->active_sequence_fname[0] = 0;
		i++;
	}
	
	KEST_PRINTF("Reading current page identifier struct, starting at position %d\n", i);
	
	state->current_page.type = *((int32_t*)(&content[i]));
	KEST_PRINTF("Obtained current_page.type as %d = 0x%08x\n", state->current_page.type, *((int*)(&content[i])));
	i += sizeof(int32_t);
	state->current_page.id = *((int32_t*)(&content[i]));
	KEST_PRINTF("Obtained current_page.type as %d = 0x%08x\n", state->current_page.id, *((int*)(&content[i])));
	i += sizeof(int32_t);

	KEST_PRINTF("Reading state->current_page.fname from position %d\n", i);
	if (content[i])
	{
		j = i;
		
		while (content[i] && i - j < 32)
		{
			KEST_PRINTF("\t0x%02x = '%c'\n", content[i], content[i]);
			state->current_page.fname[i - j] = (char)content[i];
			i++;
		}
		state->current_page.fname[i - j] = 0;
	}
	else
	{
		state->current_page.fname[0] = 0;
		i++;
	}
	
	i++;

	KEST_PRINTF("read input gain: %f\n",  state->input_gain);
	KEST_PRINTF("read output gain: %f\n", state->output_gain);
	KEST_PRINTF("read active preset fname: %s\n", state->active_preset_fname);
	KEST_PRINTF("read active sequence fname: %s\n", state->active_sequence_fname);
	KEST_PRINTF("read current page: {type = %d, id = %d, fname = \"%s\"}\n", state->current_page.type, state->current_page.id,
		state->current_page.fname);
	
read_settings_exit:
	
	return ret_val;
}

int read_preset_from_file(kest_preset *preset, const char *fname)
{
	//kest_printf("read_preset_from_file\n");
	if (!fname || !preset)
	{
		//kest_printf("NULL pointer lol\n");
		return ERR_NULL_PTR;
	}
	
	#ifdef DUMP_PRESET_READ
	dump_file_contents(fname);
	#endif
	
	FILE *file = fopen(fname, "r");
	
	if (!file)
	{
		//kest_printf("Could not open file %s\n", fname);
		return ERR_FOPEN_FAIL;
	}
	
	
	KEST_PRINTF("Reading preset from %s\n", fname);
	
	uint8_t byte;
	uint16_t arg16;
	int32_t arg32;
	uint16_t n_effects;
	char string_read_buffer[IO_BUFFER_SIZE];
	char *name = NULL;
	int ret_val = NO_ERROR;
	kest_effect *effect = NULL;
	kest_parameter_pll *current_param = NULL;
	kest_setting_pll *current_setting = NULL;
	
	// Check that this is a preset file
	byte = fgetc(file);
	
	if (byte != KEST_PRESET_MAGIC_BYTE)
	{
		KEST_PRINTF("Attempted load of preset from file \"%s\", whose first byte 0x%02x is not the preset magic byte 0x%02x\n",
			fname, byte, KEST_PRESET_MAGIC_BYTE);
		ret_val = ERR_BAD_ARGS;
		goto preset_read_bail;
	}
	
	// Check that the write was finished
	byte = fgetc(file);
	
	if (byte != KEST_WRITE_FINISHED_BYTE)
	{
		KEST_PRINTF("Attempted load of preset from file \"%s\", whose second byte 0x%02x indicates that its write was unfinished\n",
			fname, byte);
		ret_val = ERR_UNFINISHED_WRITE;
		goto preset_read_bail;
	}
	
	read_and_strndup_string(name);
	
	if (!name)
	{
		KEST_PRINTF("Allocation fail allocating string of length %d for preset name from file %s\n", (int)byte, fname);
		goto preset_read_bail;
	}
	
	preset->name = name;
	
	KEST_PRINTF("Loaded preset name: %s\n", preset->name);
	
	read_short(n_effects);
	
	kest_effect_desc *eff = NULL;
	
	for (int i = 0; i < n_effects; i++)
	{
		KEST_PRINTF("Preset professes to contain %d effects\n", n_effects);
		//Get effect type
		read_string();
		
		eff = kest_cxt_get_effect_desc_from_cname(&global_cxt, string_read_buffer);
		
		if (!eff)
		{
			KEST_PRINTF("Preset references non-existent effect. Aborting.\n");
			ret_val = ERR_MANGLED_FILE;
			goto preset_read_bail;
		}
		
		KEST_PRINTF("Encountered %s in position %d\n", eff->name, (int)i);
		
		effect = kest_preset_append_effect_eff(preset, eff);
		
		if (!effect)
		{
			KEST_PRINTF("Failed to append effect \"%s\"", eff->name);
			ret_val = ERR_MANGLED_FILE;
			goto preset_read_bail;
		}
		
		// Get effect ID
		read_short(arg16);
		
		
		KEST_PRINTF("Effect ID: %d\n", (int)arg16);
		effect->id = arg16;
		
		current_param = effect->parameters;
		while (current_param)
		{
			if (current_param->data)
			{
				read_float(current_param->data->value);
			}
			
			current_param = current_param->next;
		}
		
		current_setting = effect->settings;
		while (current_setting)
		{
			if (current_setting->data)
				read_float(current_setting->data->value);
			
			current_setting = current_setting->next;
		}
		
		//kest_effect_init_view_page(effect);
	}
	
	KEST_PRINTF("File done! closing...\n");
	fclose(file);
	KEST_PRINTF("Closed. Returning\n");
	
	for (int k = 0; k < KEST_FILENAME_LEN; k++)
	{
		if (!fname[k])
		{
			preset->fname[k] = 0;
			break;
		}
		
		preset->fname[k] = fname[k];
	}
	
	preset->has_fname = 1;
	
	
	
	preset->unsaved_changes = 0;
	
	return ret_val;
	
preset_read_bail:
	//kest_printf("BAILING\n");
	fclose(file);
	
	//kest_printf("BAILED\n");
	return ret_val;
}

int read_sequence_from_file(kest_sequence *sequence, const char *fname)
{
	KEST_PRINTF("read_sequence_from_file\n");
	if (!fname || !sequence)
	{
		return ERR_NULL_PTR;
	}
	
	#ifdef DUMP_SEQUENCE_READ
	dump_file_contents(fname);
	#endif
	
	FILE *file = fopen(fname, "r");
	
	if (!file)
	{
		KEST_PRINTF("Could not open file \"%s\"\n", fname);
		return ERR_FOPEN_FAIL;
	}
	
	KEST_PRINTF("Reading sequence from %s\n", fname);
	
	uint8_t byte;
	uint16_t arg16;
	uint16_t n_presets;
	char string_read_buffer[IO_BUFFER_SIZE];
	char *name = NULL;
	char *preset_fname = NULL;
	
	int ret_val = NO_ERROR;
	
	switch (file_validity_check(file, KEST_SEQUENCE_MAGIC_BYTE, &byte))
	{
		case 0:
			break;
		
		case 1:
			KEST_PRINTF("Attempted load of sequence from file \"%s\", whose first byte 0x%02x is not the sequence magic byte 0x%02x",
				fname, byte, KEST_PRESET_MAGIC_BYTE);
			ret_val = ERR_BAD_ARGS;
			goto sequence_read_bail;
		
		case 2:
			KEST_PRINTF("Attempted load of sequence from file \"%s\", whose second byte 0x%02x indicates that its write was unfinishedn",
				fname, byte);
			ret_val = ERR_UNFINISHED_WRITE;
			goto sequence_read_bail;
	}
	
	read_and_strndup_string(name);
	
	if (!name)
	{
		KEST_PRINTF("Allocation fail allocating string of length %d for sequence name from file %s", (int)byte, fname);
		goto sequence_read_bail;
	}
	
	sequence->name = name;
	
	KEST_PRINTF("Loaded sequence name: %s\n", sequence->name);
	
	read_short(n_presets);
	
	kest_preset *preset;
	
	for (int i = 0; i < n_presets; i++)
	{
		read_string();
		
		KEST_PRINTF("Sequence contains preset %s...\n", string_read_buffer);
		preset = cxt_get_preset_by_fname(&global_cxt, string_read_buffer);
		
		if (preset)
		{
			sequence_append_preset(sequence, preset);
		}
		else
		{
			KEST_PRINTF("Error: sequence %s contains preset %s, but no such preset found!\n", fname, string_read_buffer);
		}
	}
	
	KEST_PRINTF("File done! closing...\n");
	fclose(file);
	KEST_PRINTF("Closed. Returning\n");
	
	for (int k = 0; k < KEST_FILENAME_LEN; k++)
	{
		if (!fname[k])
		{
			sequence->fname[k] = 0;
			break;
		}
		
		sequence->fname[k] = fname[k];
	}
	
	sequence->has_fname = 1;
	sequence->unsaved_changes = 0;
	
	return ret_val;
	
sequence_read_bail:
	fclose(file);
	
	return ret_val;
}

int kest_init_directories()
{
	struct stat statbuf;

	if (stat(KEST_PRESETS_DIR, &statbuf) == 0)
	{
		KEST_PRINTF("Presets directory %s found\n", KEST_PRESETS_DIR);
	}
	else
	{
		KEST_PRINTF("Presets directory %s doesn't exist. Creating...\n", KEST_PRESETS_DIR);
		if (mkdir(KEST_PRESETS_DIR, 07777) != 0)
		{
			KEST_PRINTF("Failed to create presets directory\n");
		}
		else
		{
			KEST_PRINTF("Directory created sucessfully\n");
		}
	}

	if (stat(KEST_SEQUENCES_DIR, &statbuf) == 0)
	{
		KEST_PRINTF("Sequences directory %s found", KEST_SEQUENCES_DIR);
	}
	else
	{
		KEST_PRINTF("Sequences directory %s doesn't exist. Creating...\n", KEST_SEQUENCES_DIR);
		if (mkdir(KEST_SEQUENCES_DIR, 07777) != 0)
		{
			KEST_PRINTF("Failed to create sequences directory\n");
		}
		else
		{
			KEST_PRINTF("Directory created sucessfully\n");
		}
	}
	
	return NO_ERROR;
}

int safe_file_write(int (*write_func)(void *arg, const char *fname), void *arg, const char *fname)
{
	if (!write_func)
		return ERR_NULL_PTR;
	
	// Check if the file exists
	FILE *target = fopen(fname, "r");
	char buf[strlen(fname) + 5];
	int backup = 0;
	
	if (target)
	{
		backup = 1;
		
		// Append ".bak" to the filename
		sprintf(buf, "%s.bak", fname);
		
		// Check if a file with that name exists
		FILE *bakfile = fopen(buf, "r");
		if (bakfile)
		{
			// If so, delete it
			remove(buf);
			fclose(bakfile);
		}
		
		// Rename the current version
		rename(fname, buf);
		
		fclose(target);
	}
	
	int ret_val = write_func(arg, fname);
	
	// If we backed up but the write failed,
	// replace the newly written file with
	// the old backup
	if (backup && ret_val != NO_ERROR)
	{
		// Remove any busted file we wrote
		remove(fname);
		// Move the backup back in place
		rename(buf, fname);
	}
	
	return ret_val;
}

int save_preset_as_file_safe(kest_preset *preset, const char *fname)
{
	// Check if the file exists
	FILE *target = fopen(fname, "r");
	char buf[strlen(fname) + 5];
	int backup = 0;
	
	if (target)
	{
		backup = 1;
		
		// Append ".bak" to the filename
		sprintf(buf, "%s.bak", fname);
		
		// Check if a file with that name exists
		FILE *bakfile = fopen(buf, "r");
		if (bakfile)
		{
			// If so, delete it
			remove(buf);
			fclose(bakfile);
		}
		
		// Rename the current version
		rename(fname, buf);
		
		fclose(target);
	}
	
	int ret_val = save_preset_as_file(preset, fname);
	
	// If we backed up but the write failed,
	// replace the newly written file with
	// the old backup
	if (backup && ret_val != NO_ERROR)
	{
		// Remove any busted file we wrote
		remove(fname);
		// Move the backup back in place
		rename(buf, fname);
	}
	
	return ret_val;
}

#define FNAME_DIGITS 4

void generate_filename(char *prefix, char *suffix, char *dest)
{
	if (!dest)
		return;
	
	int plen = 0, slen = 0;
	
	if (prefix)
		plen = strlen(prefix);
	if (suffix)
		slen = strlen(suffix);
	
	char fname[KEST_FILENAME_LEN];
	
	int index = 0;
	
	for (int i = 0; i < plen; i++)
		fname[index++] = prefix[i];
	
	char c;
	int x;
	
	for (int i = 0; i < FNAME_DIGITS; i++)
	{
		x = rand() % 36;

		fname[index++] = (x < 10) ? '0' + x : 'A' + (x - 10);
	}
	
	for (int i = 0; i < slen; i++)
		fname[index++] = (suffix[i] == '%') ? '_' : suffix[i];
	
	fname[index++] = 0;
	
	KEST_PRINTF("Generated filename %s\n", fname);
	
	for (int k = 0; k < index && k < KEST_FILENAME_LEN; k++)
	{
		if (fname[k] == 0)
		{
			dest[k] = 0;
			break;
		}
		
		dest[k] = fname[k];
	}
	
	return;
}

int save_preset(kest_preset *preset)
{
	if (!preset->has_fname)
	{
		FILE *test = NULL;
		
		do {
			generate_filename(KEST_PRESETS_DIR, PRESET_EXTENSION, preset->fname);
			
			if (!preset)
				return ERR_ALLOC_FAIL;
			
			test = fopen(preset->fname, "r");
			
			if (test)
			{
				fclose(test);
			}
		} while (test);
		
		preset->has_fname = 1;
	}
	
	int ret_val = save_preset_as_file(preset, preset->fname);
	
	if (ret_val == NO_ERROR)
	{
		KEST_PRINTF("Sucessfully saved preset as %s. Dumping file...\n", preset->fname);
		
		#ifdef DUMP_PRESET_SAVE
		dump_file_contents(fname);
		#endif
	}
	else
	{
		KEST_PRINTF("Preset save error: %s\n", kest_error_code_to_string(ret_val));
	}
	
	return ret_val;
}

int save_sequence(kest_sequence *sequence)
{
	if (!sequence)
		return ERR_NULL_PTR;
	
	if (!sequence->has_fname)
	{
		FILE *test = NULL;
		
		do {
			generate_filename(KEST_SEQUENCES_DIR, SEQUENCE_EXTENSION, sequence->fname);
			
			test = fopen(sequence->fname, "r");
			
			if (test)
			{
				fclose(test);
			}
		} while (test);
		
		sequence->has_fname = 1;
	}
	
	int ret_val = save_sequence_as_file(sequence, sequence->fname);
	
	if (ret_val == NO_ERROR)
	{
		KEST_PRINTF("Sucessfully saved sequence as %s. Dumping file...\n", sequence->fname);
		
		#ifdef DUMP_SEQUENCE_SAVE
		dump_file_contents(fname);
		#endif
	}
	else
	{
		KEST_PRINTF("Sequence save error: %s\n", kest_error_code_to_string(ret_val));
	}
	
	return ret_val;
}

int load_saved_presets(kest_context *cxt)
{
	KEST_PRINTF("load_saved_presets...\n");
	string_ll *current_file = list_files_in_directory(KEST_PRESETS_DIR);
	
	string_ll *cf = current_file;
	
	KEST_PRINTF("Preset files fonund:\n");
	if (!cf)
	{
		KEST_PRINTF("none!!!\n");
	}
	else
	{
		while (cf)
		{
			KEST_PRINTF("%s\n", cf->data);
			cf = cf->next;
		}
	}
	
	kest_preset *preset;
	
	kest_preset_pll *nl;
	
	int ret_val;
	
	while (current_file)
	{
		KEST_PRINTF("Loading preset %s...\n", current_file->data);
		preset = kest_allocator_alloc(&kest_preset_allocator, 1);
		
		if (!preset)
			return ERR_ALLOC_FAIL;
		
		init_m_preset(preset);
		ret_val = read_preset_from_file(preset, current_file->data);
		
		if (ret_val == NO_ERROR)
		{
			KEST_PRINTF("Sucessfully read preset %s, giving it ID %d\n", current_file->data, preset->id);
			kest_preset_rectify_ids(preset);
			nl = kest_preset_pll_append(cxt->presets, preset);
		
			if (!nl)
			{
				free_preset(preset);
				return ERR_ALLOC_FAIL;
			}
			cxt->presets = nl;
			
			create_preset_view_for(preset);
		}
		else
		{
			free_preset(preset);
		}
		
		current_file = current_file->next;
	}
	
	global_cxt.saved_presets_loaded = 1;
	
	return NO_ERROR;
}

int load_saved_sequences(kest_context *cxt)
{
	int ret_val;
	
	read_sequence_from_file(&cxt->main_sequence, MAIN_SEQUENCE_FNAME);
	
	string_ll *current_file = list_files_in_directory(KEST_SEQUENCES_DIR);
	
	kest_sequence *sequence;
	
	kest_sequence_pll *nl;
	
	while (current_file)
	{
		sequence = kest_allocator_alloc(&kest_sequence_allocator, 1);
		
		if (!sequence)
			return ERR_ALLOC_FAIL;
		
		init_m_sequence(sequence);
		ret_val = read_sequence_from_file(sequence, current_file->data);
		
		if (ret_val == NO_ERROR)
		{	
			nl = kest_sequence_pll_append(cxt->sequences, sequence);
		
			if (!nl)
			{
				free_sequence(sequence);
				return ERR_ALLOC_FAIL;
			}
			cxt->sequences = nl;
			
			create_sequence_view_for(sequence);
		}
		else
		{
			free_sequence(sequence);
		}
		
		current_file = current_file->next;
	}
	
	global_cxt.saved_sequences_loaded = 1;
	
	return NO_ERROR;
}

int load_effects(kest_context *cxt)
{
	if (!cxt)
		return ERR_NULL_PTR;
	
	string_ll *current = list_files_in_directory(KEST_EFFECT_DESC_DIR);
	
	kest_effect_desc *eff = NULL;
	int ret_val = NO_ERROR;
	
	if (current)
	{
		ret_val = kest_eff_parser_deinit_mempool();
		if (ret_val != NO_ERROR)
		{
			return ret_val;
		}
	}
	
	while (current)
	{
		KEST_PRINTF("Attempting to load effect from file \"%s\"...\n", current->data);
		
		eff = kest_read_eff_desc_from_file(current->data);
		
		KEST_PRINTF("kest_read_eff_desc_from_file returned the pointer %p\n", eff);
		
		if (eff)
		{
			KEST_PRINTF("Obtained an effect descriptor by the name of \"%s\" ! Adding it to the list %p...\n",
				eff->name, &cxt->effects);
			ret_val = kest_effect_desc_pll_safe_append(&cxt->effects, eff);
			
			if (ret_val != NO_ERROR)
			{
				KEST_PRINTF("Error adding effect \"%s\"; error %s\n", eff->name, kest_error_code_to_string(ret_val));
			}
		}
		
		kest_eff_parser_reset_mempool();
		current = current->next;
	}
	
	kest_eff_parser_deinit_mempool();
	
	return NO_ERROR;
}


string_ll *list_files_in_directory(char *dir)
{
	#ifndef USE_SDCARD
	return NULL;
	#endif
	
	if (!dir)
		return NULL;
	
	KEST_PRINTF("Generating list of files in %s\n", dir);
	
	char *fname = NULL;
	
	DIR *directory = opendir(dir);
	
	if (!directory)
	{
		KEST_PRINTF("Failed to open directory!\n");
		return NULL;
	}
	
	struct dirent *directory_entry = readdir(directory);
	
	string_ll *list = NULL;
	string_ll *nl;
	
	while (directory_entry)
	{
		KEST_PRINTF("Directory entry: %s\n", directory_entry->d_name);
		if (directory_entry->d_type == DT_DIR)
		{
			KEST_PRINTF("... is itself a directory!\n");
			directory_entry = readdir(directory);
			continue;
		}
		
		fname = kest_alloc(strlen(dir) + 1 + 255);
		
		if (!fname)
		{
			KEST_PRINTF("Error: couldn't allocate string to list directory entry %s/%s", dir, directory_entry->d_name);
			return list;
		}
		
		sprintf(fname, "%s%s", dir, directory_entry->d_name);
		
		nl = char_pll_append(list, fname);
		
		if (nl)
		{
			list = nl;
		}
		else
		{
						KEST_PRINTF("Error: couldn't append linked list to list directory entry %s/%s", dir, directory_entry->d_name);
			return list;
		}
		
		directory_entry = readdir(directory);
	}
	
	return list;
}

void erase_sd_card_void_cb(void *data)
{
	#ifdef USE_SDCARD
	erase_sd_card();
	#endif
}

int erase_folder(const char *dir)
{
	KEST_PRINTF("Erasing directory %s...\n", dir);
	
	DIR *directory = opendir(dir);
	
	if (!directory)
	{
		KEST_PRINTF("Failed to open directory!\n");
		return ERR_BAD_ARGS;
	}
	
	struct dirent *directory_entry = readdir(directory);
	
	int ret_val = NO_ERROR;
	int len = strnlen(dir, NAME_MAX);
	int bufsize = len + NAME_MAX + 1;
	const char *sep = (len > 0 && dir[len - 1] == '/') ? "" : "/";
	
	char *buf = kest_alloc(bufsize);
	
	if (!buf)
		return ERR_ALLOC_FAIL;
	
	while (directory_entry)
	{
		KEST_PRINTF("Directory entry: %s\n", directory_entry->d_name);
		
		if (strcmp(directory_entry->d_name, ".") == 0 || strcmp(directory_entry->d_name, "..") == 0)
		{
			directory_entry = readdir(directory);
			continue;
		}
		
		if (directory_entry->d_type == DT_DIR)
		{
			KEST_PRINTF("... is itself a directory!\n");
			snprintf(buf, bufsize, "%s%s%s", dir, sep, directory_entry->d_name);
			KEST_PRINTF("Full name: %s\n", buf);
			ret_val = erase_folder(buf);
		}
		else
		{
			KEST_PRINTF("... is a file. Deleting...\n");
			snprintf(buf, bufsize, "%s%s%s", dir, sep, directory_entry->d_name);
			KEST_PRINTF("Full name: %s\n", buf);
			remove(buf);
		}
		
		directory_entry = readdir(directory);
	}
	
	kest_free(buf);
	
	return ret_val;
}

void erase_sd_card()
{
	erase_folder(KEST_PRESETS_DIR);
	erase_folder(KEST_SEQUENCES_DIR);
	remove(MAIN_SEQUENCE_FNAME);
	remove(SETTINGS_FNAME);
}

int fnames_agree(char *a, char *b)
{
	if (!a || !b)
		return 0;
	
	for (int k = 0; a[k] && b[k]; k++)
	{
		if (a[k] != b[k])
		{
			if (char_is_letter(a[k]) && char_is_letter(b[k]) && ((a[k] - b[k] == 'A' - 'a') || (a[k] - b[k] == 'a' - 'A')))
				continue;
			else
				return 0;
		}
	}
	
	return 1;
}

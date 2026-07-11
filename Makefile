top_objdir := bin
app_objdir := $(top_objdir)/app
lib_objdir := $(top_objdir)/lib
test_objdir := $(top_objdir)/tests

app_cfiles := 	core/kest_error_codes.c		\
				core/kest_init.c			\
				core/kest_alloc.c			\
				core/kest_pool.c			\
				core/kest_block.c			\
				core/kest_bump_arena.c		\
				core/kest_context.c			\
				core/kest_eff_desc.c		\
				core/kest_expression.c		\
				core/kest_expr_scope.c		\
				core/kest_files.c			\
				core/kest_file_task.c		\
				core/kest_helper_fn.c		\
				core/kest_lv_log.c			\
				core/kest_driver.c			\
				core/kest_parameter.c		\
				core/kest_param_update.c	\
				core/kest_update.c			\
				core/kest_effect.c			\
				core/kest_state.c			\
				core/kest_pipeline.c		\
				core/kest_sequence.c		\
				core/kest_preset.c			\
				core/kest_resource.c		\
				core/kest_representation.c	\
				core/kest_printf.c			\
				core/kest_string.c			\
				core/kest_global.c			\
				core/kest_event.c			\
				core/kest_dependent.c		\
				ui/kest_button.c			\
				ui/kest_home_view.c			\
				ui/kest_menu.c				\
				ui/kest_parameter_widget.c	\
				ui/kest_preset_settings.c	\
				ui/kest_preset_view.c		\
				ui/kest_sequence_list.c		\
				ui/kest_sequence_view.c		\
				ui/kest_effect_select.c		\
				ui/kest_effect_settings.c	\
				ui/kest_effect_view.c		\
				ui/kest_page_id.c			\
				ui/kest_ui.c				\
				fpga/kest_fpga_encoding.c	\
				fpga/kest_fpga_comms.c		\
				fpga/kest_fpga_instr.c		\
				fpga/kest_fpga_dma.c		\
				fpga/kest_fpga_io.c			\
				fpga/kest_reg_format.c		\
				fpga/kest_fixed_point.c		\
				fpga/kest_fpga_position.c	\
				fpga/kest_fpga_update.c		\
				fpga/kest_fpga_cmd.c		\
				parser/kest_asm_parser.c	\
				parser/kest_dict_extract.c	\
				parser/kest_dictionary.c	\
				parser/kest_eff_parser.c	\
				parser/kest_eff_section.c	\
				parser/kest_expr_parser.c	\
				parser/kest_tokenizer.c

lib_cfiles := 	core/kest_error_codes.c		\
				core/kest_alloc.c			\
				core/kest_pool.c			\
				core/kest_representation.c	\
				core/kest_parameter.c		\
				core/kest_resource.c		\
				core/kest_expression.c		\
				core/kest_expr_scope.c		\
				core/kest_block.c			\
				core/kest_eff_desc.c		\
				core/kest_effect.c			\
				core/kest_pipeline.c		\
				core/kest_preset.c			\
				core/kest_driver.c			\
				core/kest_printf.c			\
				core/kest_helper_fn.c		\
				core/kest_string.c			\
				core/kest_bump_arena.c		\
				core/kest_global.c			\
				core/kest_update.c			\
				core/kest_dependent.c		\
				parser/kest_tokenizer.c		\
				parser/kest_expr_parser.c	\
				parser/kest_dictionary.c	\
				parser/kest_eff_parser.c	\
				parser/kest_eff_section.c	\
				parser/kest_asm_parser.c	\
				parser/kest_dict_extract.c	\
				fpga/kest_fpga_encoding.c	\
				fpga/kest_reg_format.c		\
				fpga/kest_fixed_point.c		\
				fpga/kest_fpga_position.c	\
				fpga/kest_fpga_instr.c		\
				fpga/kest_fpga_dma.c		\
				fpga/kest_fpga_io.c			\
				fpga/kest_fpga_cmd.c			


standalone_headers := core/kest_linked_list.h \
					  core/kest_dict.h 		  \
					  fpga/kest_fpga_defs.h   \
					  fpga/kest_fpga_comms.h
					  
lib_hfiles := $(standalone_headers) $(lib_cfiles:.c=.h)
app_hfiles := $(standalone_headers) $(app_cfiles:.c=.h)

lib_srcs := $(addprefix $(components)/,$(lib_cfiles))
lib_objs := $(addprefix $(lib_objdir)/,$(lib_cfiles:.c=.o))
lib_hdrs := main/kest_lib.h main/kest_lib_cmph.h components/core/kest_list.h $(addprefix components/,$(lib_hfiles))

app_objs := $(app_objdir)/kest_desktop.o $(addprefix $(app_objdir)/,$(app_cfiles:.c=.o))
app_hdrs := desktop/kest_desktop.h $(addprefix components/,$(app_hfiles))

COMP_DIRS := $(wildcard components/*)
INC_FLAGS_LIB := $(addprefix -I,$(COMP_DIRS))
INC_FLAGS_APP := \
	-Idesktop \
	-Idesktop/lvgl \
	-Idesktop/freertos/include \
	-Idesktop/freertos/portable/ThirdParty/GCC/Posix \
	-Idesktop/freertos/portable/MemMang \
	$(addprefix -I,$(COMP_DIRS))

CFLAGS_LIB := -fPIC -lm -DKEST_LIBRARY -Imain $(INC_FLAGS_LIB) -g
CFLAGS_APP := -DKEST_DESKTOP -Idesktop -Imain $(INC_FLAGS_APP) -g
LDFLAGS_APP := -lm -lSDL2 -lpthread
CFLAGS_TEST := $(CFLAGS_APP) -g -Itests

HDR_INSTALL_DIR := /usr/include/libkest/

LVGL_DIR := desktop/lvgl

LVGL_SRC := $(shell find $(LVGL_DIR)/src -name "*.c"  ! -name "lv_linux.c"  ! -name "lv_pthread.c")
LVGL_OBJ := $(patsubst $(LVGL_DIR)/src/%.c,$(app_objdir)/lvgl/%.o,$(LVGL_SRC))

APP_SRC := desktop/kest_desktop.c $(addprefix components/,$(app_cfiles))
APP_OBJ := $(patsubst %.c,$(app_objdir)/%.o,$(APP_SRC))

FREERTOS_DIR := desktop/freertos

FREERTOS_CORE_SRC := \
	$(FREERTOS_DIR)/tasks.c \
	$(FREERTOS_DIR)/queue.c \
	$(FREERTOS_DIR)/list.c \
	$(FREERTOS_DIR)/timers.c \
	$(FREERTOS_DIR)/event_groups.c \
	$(FREERTOS_DIR)/stream_buffer.c

FREERTOS_HEAP_SRC := \
	$(FREERTOS_DIR)/portable/MemMang/heap_4.c

FREERTOS_PORT_SRC := \
	$(FREERTOS_DIR)/portable/ThirdParty/GCC/Posix/port.c \
	$(FREERTOS_DIR)/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c

FREERTOS_SRC := \
	$(FREERTOS_CORE_SRC) \
	$(FREERTOS_HEAP_SRC) \
	$(FREERTOS_PORT_SRC)

FREERTOS_OBJ := $(patsubst %.c,$(app_objdir)/%.o,$(FREERTOS_SRC))

ALL_APP_OBJ := $(APP_OBJ) $(LVGL_OBJ) $(FREERTOS_OBJ)

TEST_SRC := $(shell find tests -name "*.c")
TEST_OBJ := $(patsubst tests/%.c,$(test_objdir)/%.o,$(TEST_SRC))
TEST_APP_OBJ := $(filter-out $(app_objdir)/desktop/kest_desktop.o,$(APP_OBJ))

app: kest

tests: kest_tests

clean:
	rm -r $(top_objdir)

testclean:
	rm ./kest_tests && rm -r $(test_objdir)

appclean:
	rm -r $(app_objdir)/desktop $(app_objdir)/components

appclean_full: 
	rm -r $(app_objdir)

fullclean:
	rm -r $(top_objdir)
	
kest: $(ALL_APP_OBJ) | $(app_objdir) $(app_objdir)/desktop
	gcc -o $@ $^ `sdl2-config --libs` -lm -lpthread

kest_tests: $(TEST_OBJ) $(TEST_APP_OBJ) $(LVGL_OBJ) $(FREERTOS_OBJ)
	gcc $(CFLAGS_TEST) -Wl,--undefined=__start___m_tests -Wl,--undefined=__stop___m_tests \
	    -o $@ $^ `sdl2-config --libs` -lm -lpthread

all : app lib
	idf.py build

$(app_objdir)/%.o: %.c $(app_hdrs) | $(app_objdir)
	mkdir -p $(dir $@)
	gcc $(CFLAGS_APP) `sdl2-config --cflags` -c $< -o $@

$(app_objdir)/%.o : components/%.c | $(app_objdir)
	gcc $(CFLAGS_APP) -c $< -o $@

$(app_objdir)/desktop/%.o : desktop/%.c | $(app_objdir)/desktop
	gcc $(CFLAGS_APP) -c $< -o $@

$(app_objdir)/lvgl/%.o: $(LVGL_DIR)/src/%.c | $(app_objdir)
	mkdir -p $(dir $@)
	gcc $(CFLAGS_APP) `sdl2-config --cflags` -c $< -o $@

$(app_objdir)/desktop/freertos/%.o: desktop/freertos/%.c | $(app_objdir)
	mkdir -p $(dir $@)
	gcc $(CFLAGS_APP) `sdl2-config --cflags` -c $< -o $@

$(test_objdir)/%.o: tests/%.c | $(test_objdir)
	mkdir -p $(dir $@)
	gcc $(CFLAGS_TEST) -c $< -o $@

lib: $(lib_objdir)/libkest.so

$(lib_objdir)/libkest.so: $(lib_objs)
	gcc -shared -o $@ $^

lib_install: | $(HDR_INSTALL_DIR)
	cp $(lib_objdir)/libkest.so /usr/lib/
	cp $(lib_hdrs) $(HDR_INSTALL_DIR)
	ldconfig

$(lib_objdir)/%.o : components/%.c | $(lib_objdir)
	gcc $(CFLAGS_LIB) -c $< -o $@

$(top_objdir):
	mkdir $(top_objdir)

$(app_objdir): | $(top_objdir)
	mkdir $(app_objdir)
	mkdir $(app_objdir)/components
	mkdir $(app_objdir)/components/core
	mkdir $(app_objdir)/components/parser
	mkdir $(app_objdir)/components/fpga
	mkdir $(app_objdir)/components/ui

$(app_objdir)/desktop : | $(app_objdir)
	mkdir $(app_objdir)/desktop

$(lib_objdir):  | $(top_objdir)
	mkdir $(lib_objdir)
	mkdir $(lib_objdir)/core
	mkdir $(lib_objdir)/parser
	mkdir $(lib_objdir)/fpga

$(test_objdir): | $(top_objdir)
	mkdir $(test_objdir)

$(HDR_INSTALL_DIR) :
	mkdir $(HDR_INSTALL_DIR)

libclean:
	rm -r $(lib_objdir)

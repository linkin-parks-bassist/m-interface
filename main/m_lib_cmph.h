#ifndef M_LIB_COMPILATION_HELPER_H_
#define M_LIB_COMPILATION_HELPER_H_

// Definitions, etc, needed for compilation to proceed, but not
// actually needed for the library to function. Better than having
// #ifdef M_LIBRARY\#endif everywhere in the code proper

#define PARAM_WIDGET_VIRTUAL_POT  0
#define PARAM_WIDGET_HSLIDER 	  1
#define PARAM_WIDGET_VSLIDER 	  2
#define PARAM_WIDGET_VSLIDER_TALL 3

#define SETTING_WIDGET_DROPDOWN	 0
#define SETTING_WIDGET_SWITCH 	 1
#define SETTING_WIDGET_FIELD 	 2

#define m_alloc 	malloc
#define m_free 		free 
#define m_strndup 	strndup
#define m_realloc	realloc

#define M_FPGA_SIMULATED

DECLARE_LINKED_PTR_LIST(char);
typedef char_pll string_ll;

#endif

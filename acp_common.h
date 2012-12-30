/*
 * acp_common.h
 *
 *  Created on: Nov 8, 2012
 */
#ifndef _ACP_COMMON_H_
#define _ACP_COMMON_H_

#include<unistd.h>
#include<stdlib.h>
#include<curses.h>

#include "acp_config.h"
#include "acp_error.h"
#include "cdk_wrap.h"

#ifdef HAVE_XCURSES
char *XCursesProgramName = "ACP";
#endif

//some basic datatypes
#define TRUE 1
#define FALSE 0
#define LOG_BUFF_SIZE 100
#define BOOL short int //represents boolean value --TRUE or FALSE
//store windows related information
struct window_state_t {
	WINDOW *wptr;
	int beg_x, beg_y, height, width; //window coordinates
	int cur_x, cur_y;
	CDKSCREEN *cdksptr;
};
struct acp_global_labels{
	struct window_state_t win; //in which window does this label lie
	int beg_x,beg_y;
	CDKLABEL *lblptr;
}; //to be shorted as agl
//store acp_state information
struct ACP_STATE {
	struct window_state_t menubar_win, GMM_win, CMM_win, console_win;
	// ----------global labels------------------------
	// mostly these labels will be updated by separate threads
	struct acp_global_labels agl_gmm_total_memory,agl_gmm_total_pages,agl_gmm_pages_used;
	struct acp_global_labels agl_gmm_swap_max_size,agl_gmm_swap_used_space,
			agl_gmm_swap_pageout_timelag,agl_gmm_swap_pagein_timelag;

	struct acp_global_labels agl_cmm_ucm_cur_memsize,agl_cmm_ucm_max_pages,agl_cmm_ucm_pages_used;
	struct acp_global_labels agl_cmm_cc_cur_memsize,agl_cmm_cc_stored_pages,agl_cmm_cc_cells,agl_cmm_cc_pageout_timelag,
			agl_cmm_cc_pagein_timelag;
	struct acp_global_labels agl_cmm_swap_max_size,agl_cmm_swap_used_space,agl_cmm_swap_pages_used,
				agl_cmm_swap_pageout_timelag,agl_cmm_swap_pagein_timelag;

	BOOL color_ok;
	char log_buffer[LOG_BUFF_SIZE];
	BOOL gui_ready;
	int hori_pad, vert_pad;
	//cdk specific window pointers
	CDK_PARAMS params;
	CDKSWINDOW *console;
	CDKSCREEN *master_screen;
	WINDOW *cursesWin; //main curses window--stdscr
} acp_state; //globally accessible -across file. access with extern
typedef enum {
	LOG_DEBUG, LOG_WARN, LOG_ERROR
} log_level_t;
//some debug,warning and error macros
//#define sdebug(s) fprintf(stderr, "\n[" __FILE__ ":%i] debug: " s "",__LINE__)
#define sdebug(s) sprintf(acp_state.log_buffer, "[" __FILE__ ":%i] Debug: " s "",__LINE__); \
			log_generic(acp_state.log_buffer,LOG_DEBUG);
#define var_debug(s, ...) sprintf(acp_state.log_buffer, "[" __FILE__ ":%i] Debug: " s "" ,\
	__LINE__,__VA_ARGS__);\
	log_generic(acp_state.log_buffer,LOG_DEBUG);

#define swarn(s) sprintf(acp_state.log_buffer, "[" __FILE__ ":%i] Warning: " s "",__LINE__);\
		log_generic(acp_state.log_buffer,LOG_WARN);
#define var_warn(s, ...) sprintf(acp_state.log_buffer, "[" __FILE__ ":%i] Warning: " s "",\
		__LINE__,__VA_ARGS__); \
		log_generic(acp_state.log_buffer,LOG_WARN);

#define serror(s) sprintf(acp_state.log_buffer, "[" __FILE__ ":%i] Error: " s "",__LINE__);\
		log_generic(acp_state.log_buffer,LOG_ERROR);
#define var_error(s, ...) sprintf(acp_state.log_buffer, "[" __FILE__ ":%i] Error: " s "",\
		__LINE__,__VA_ARGS__);\
		log_generic(acp_state.log_buffer,LOG_ERROR);

//routines
void exit_acp(); //call when fatal error has occurred such as window size too
// short or could not create windows
//call when error reporting through gui-console is not
//available
void init_acp_state();
void log_generic(const char* msg, log_level_t log_level);
void report_error(error_codes_t error_code); //report errors which are fatal

int calculate_msg_center_position(int msg_len);
void close_ui();
void destroy_win(WINDOW *local_win);
/*
 * remember that routines which name resembles those in curses package,
 * prepend their name with "acp_".
 */
void acp_move_wxy(struct window_state_t *win, int new_y, int new_x);
void acp_mv_wprintw(struct window_state_t *win, int new_y, int new_x,
		char *string);
void set_focus_to_console();
void log_msg_to_console(const char* msg, log_level_t level);
void destroy_cdkscreens();
void init_window_pointers();
void init_cdks_pointers();
void destroy_acp_window(); //main routine for deleting windows
void redraw_cdkscreens(); //draw cdkscrens after drawing a popup window or
//when needed
void print_max_console_size();
#endif

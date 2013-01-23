/*
 * acp_common.h
 *
 *  Created on: Nov 8, 2012
 */
#ifndef _ACP_COMMON_H_
#define _ACP_COMMON_H_

#ifndef ACP_DTYPES_H_
	#include "acp_dtypes.h"
#endif

#include<unistd.h>
#include<stdlib.h>
#include <pthread.h>
#include<curses.h>

#include "acp_config.h"
#include "acp_error.h"
#include "cdk_wrap.h"



#ifdef HAVE_XCURSES
char *XCursesProgramName = "ACP";
#endif

//some basic datatypes



#define LOG_BUFF_SIZE 200
//store windows related information
struct window_state_t {
    WINDOW *wptr;
    int beg_x, beg_y, height, width; //window coordinates
    int cur_x, cur_y;
    CDKSCREEN *cdksptr;
};
struct acp_global_labels {
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

    bool color_ok;
    char log_buffer[LOG_BUFF_SIZE];
    bool gui_ready;
    int hori_pad, vert_pad;
    //cdk specific window pointers
    CDK_PARAMS params;
    CDKSWINDOW *console;
    CDKSCREEN *master_screen;
    WINDOW *cursesWin; //main curses window--stdscr
    // mutex for getting lock if log_buffer
    pthread_mutex_t log_buffer_lock;
} acp_state;

//some debug,warning and error macros
//#define sdebug(s) fprintf(stderr, "\n[" __FILE__ ":%i] debug: " s "",__LINE__)
#define sdebug(s) pthread_mutex_lock(&acp_state.log_buffer_lock); \
				   snprintf(acp_state.log_buffer, LOG_BUFF_SIZE,"[" __FILE__ ":%i] Debug: " s "",__LINE__); \
				   log_generic(acp_state.log_buffer,LOG_DEBUG); \
				   pthread_mutex_unlock(&acp_state.log_buffer_lock);

#define var_debug(s, ...) pthread_mutex_lock(&acp_state.log_buffer_lock); \
						   snprintf(acp_state.log_buffer,LOG_BUFF_SIZE, "[" __FILE__ ":%i] Debug: " s "" ,\
								    __LINE__,__VA_ARGS__);\
						   log_generic(acp_state.log_buffer,LOG_DEBUG);\
	                       pthread_mutex_unlock(&acp_state.log_buffer_lock);

#define swarn(s) pthread_mutex_lock(&acp_state.log_buffer_lock); \
				  snprintf(acp_state.log_buffer,LOG_BUFF_SIZE, "[" __FILE__ ":%i] Warning: " s "",__LINE__);\
		          log_generic(acp_state.log_buffer,LOG_WARN);\
		          pthread_mutex_unlock(&acp_state.log_buffer_lock);

#define var_warn(s, ...) pthread_mutex_lock(&acp_state.log_buffer_lock); \
						  snprintf(acp_state.log_buffer,LOG_BUFF_SIZE, "[" __FILE__ ":%i] Warning: " s "",\
		                           __LINE__,__VA_ARGS__); \
		                  log_generic(acp_state.log_buffer,LOG_WARN);\
		                  pthread_mutex_unlock(&acp_state.log_buffer_lock);

#define serror(s) pthread_mutex_lock(&acp_state.log_buffer_lock); \
			       snprintf(acp_state.log_buffer,LOG_BUFF_SIZE, "[" __FILE__ ":%i] Error: " s "",__LINE__);\
		           log_generic(acp_state.log_buffer,LOG_ERROR);\
		           pthread_mutex_unlock(&acp_state.log_buffer_lock);

#define var_error(s, ...) pthread_mutex_lock(&acp_state.log_buffer_lock); \
						   snprintf(acp_state.log_buffer,LOG_BUFF_SIZE, "[" __FILE__ ":%i] Error: " s "",\
		                            __LINE__,__VA_ARGS__);\
		                   log_generic(acp_state.log_buffer,LOG_ERROR);\
	                       pthread_mutex_unlock(&acp_state.log_buffer_lock);

//routines
int init_acp_state();
/*
 * Error reporting can be done through two ways. first is to use macros
 * which in turn use log_generic(). other is to use report_error() which accepts
 * error codes in place of message strings. report_error() after converting the
 * error code to a string calls log_generic().
 */
void log_generic(const char* msg, log_level_t log_level);
void report_error(error_codes_t error_code); //report errors which are fatal
void close_ui();
void destroy_win(WINDOW *local_win);
/*
 * remember that routines which name resembles those in curses package,
 * prepend their name with "acp_".
 */
//static void acp_mv_wprintw(struct window_state_t *win, int new_y, int new_x,
//		char *string);
void set_focus_to_console();
void destroy_cdkscreens();
void display_help();
#endif

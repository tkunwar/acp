/**
 * @file acp_common.h
 * @brief header file for acp_common.c
 * @author Tej
 */
#ifndef _ACP_COMMON_H_
#define _ACP_COMMON_H_

#ifndef ACP_DTYPES_H_
	#include "acp_dtypes.h"
#endif

#include<unistd.h>
#include<execinfo.h>
#include<stdlib.h>
#include <pthread.h>
#include<curses.h>
#include<signal.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include <error.h>
#include <errno.h>
#include "acp_config.h"
#include "acp_error.h"
#include "cdk_wrap.h"



#ifdef HAVE_XCURSES
char *XCursesProgramName = "ACP";
#endif
#ifndef __USE_GNU
	#define __USE_GNU
#endif
/**
 * @def PARENT_WAIT_FOR_CHILD_THREADS
 * @brief Defines how long parent thread waits for all child threads before
 * 		doing a final exit.
 */
#define PARENT_WAIT_FOR_CHILD_THREADS 3
/**
 * @def LOG_BUFF_SIZE
 * @brief Size of log buffer used in logging macros.
 */
#define LOG_BUFF_SIZE 200
#define ACP_PAGE_SIZE 4096
/**
 * @struct window_state_t
 * @brief A structure for holding window related information.
 */
struct window_state_t {
    WINDOW *wptr;
    int beg_x, beg_y, height, width; //window coordinates
    int cur_x, cur_y;
    CDKSCREEN *cdksptr;
};
/**
 * @struct acp_global_labels
 * @brief Structure for global labels that will display information on the
 * 	screen.
 */
struct acp_global_labels {
    struct window_state_t win; //in which window does this label lie
    int beg_x,beg_y;
    CDKLABEL *lblptr;
};
/**
 * @enum storage_loc_t
 * @brief Tells about current location of a given page.
 */
typedef enum{
	SL_NCM, /**< storage location is non-compressed-memory */
	SL_CC, /**< storage location is compressed cache*/
	SL_SWAP /**< storage location is swap*/
}storage_loc_t;
/**
 * @struct page_t
 * @brief Defines properties of a page.
 */
struct page_t{
	unsigned int page_id; /**< The page id*/
	bool dirty; /**< Has the page been modified ?*/
	unsigned char page_data[ACP_PAGE_SIZE]; /**< page data of size 4 KB */
	storage_loc_t page_cur_loc; /**< Where is this page currently located ?*/
	unsigned int LAT;
};
struct page_table{
	struct page_t page;
	struct page_table *next;
};
/**
 * @struct ACP_STATE
 * @brief Main structure which holds state information for acp. Almost all
 * major variables needed by acp are present in this structure. Note that
 * variables in this structure are accessible throughout acp.
 */
struct ACP_STATE {
    struct window_state_t menubar_win, GMM_win, CMM_win, console_win;
    // ----------global labels------------------------
    // mostly these labels will be updated by separate threads
    struct acp_global_labels agl_gmm_total_memory,agl_gmm_total_pages,agl_gmm_pages_active;
    struct acp_global_labels agl_gmm_swap_max_size,agl_gmm_swap_used_space,
            agl_gmm_swap_pageout_timelag,agl_gmm_swap_pagein_timelag;

    struct acp_global_labels agl_cmm_ucm_total_memsize,agl_cmm_ucm_total_pages,agl_cmm_ucm_pages_active;
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
    bool shutdown_in_progress;
    bool shutdown_completed ;
    int recieved_signal_code;
    pthread_t gmm_main_thread,cmm_main_thread;

    unsigned int acp_cmm_page_id_counter;
    FILE *log_ptr;
} acp_state;

//some debug,warning and error macros
/**
 * @def sdebug(s)
 * @brief Debug macro which accepts a single string. Message is expanded to
 * include location of call in the source file along with other information.
 * The message after expansion is saved in a global log buffer and then finally
 * passed to log_generic().
 */
#define sdebug(s) pthread_mutex_lock(&acp_state.log_buffer_lock); \
				   snprintf(acp_state.log_buffer, LOG_BUFF_SIZE,"[" __FILE__ ":%i] Debug: " s "",__LINE__); \
				   log_generic(acp_state.log_buffer,LOG_DEBUG); \
				   pthread_mutex_unlock(&acp_state.log_buffer_lock);
/**
 * @def var_debug(s, ...)
 * @brief Debug macro which accepts multiple parameters. Message is expanded to
 * include location of call in the source file along with other information.
 * The message after expansion is saved in a global log buffer and then finally
 * passed to log_generic().
 */
#define var_debug(s, ...) pthread_mutex_lock(&acp_state.log_buffer_lock); \
						   snprintf(acp_state.log_buffer,LOG_BUFF_SIZE, "[" __FILE__ ":%i] Debug: " s "" ,\
								    __LINE__,__VA_ARGS__);\
						   log_generic(acp_state.log_buffer,LOG_DEBUG);\
	                       pthread_mutex_unlock(&acp_state.log_buffer_lock);
/**
 * @def swarn(s)
 * @brief Warning macro which accepts single string. Message is expanded to
 * include location of call in the source file along with other information.
 * The message after expansion is saved in a global log buffer and then finally
 * passed to log_generic().
 */
#define swarn(s) pthread_mutex_lock(&acp_state.log_buffer_lock); \
				  snprintf(acp_state.log_buffer,LOG_BUFF_SIZE, "[" __FILE__ ":%i] Warning: " s "",__LINE__);\
		          log_generic(acp_state.log_buffer,LOG_WARN);\
		          pthread_mutex_unlock(&acp_state.log_buffer_lock);
/**
 * @def var_warn(s,...)
 * @brief Warning macro which accepts multiple parameters. Message is expanded to
 * include location of call in the source file along with other information.
 * The message after expansion is saved in a global log buffer and then finally
 * passed to log_generic().
 */
#define var_warn(s, ...) pthread_mutex_lock(&acp_state.log_buffer_lock); \
						  snprintf(acp_state.log_buffer,LOG_BUFF_SIZE, "[" __FILE__ ":%i] Warning: " s "",\
		                           __LINE__,__VA_ARGS__); \
		                  log_generic(acp_state.log_buffer,LOG_WARN);\
		                  pthread_mutex_unlock(&acp_state.log_buffer_lock);
/**
 * @def serror(s)
 * @brief Error macro which accepts single string. Message is expanded to
 * include location of call in the source file along with other information.
 * The message after expansion is saved in a global log buffer and then finally
 * passed to log_generic().
 */
#define serror(s) pthread_mutex_lock(&acp_state.log_buffer_lock); \
			       snprintf(acp_state.log_buffer,LOG_BUFF_SIZE, "[" __FILE__ ":%i] Error: " s "",__LINE__);\
		           log_generic(acp_state.log_buffer,LOG_ERROR);\
		           pthread_mutex_unlock(&acp_state.log_buffer_lock);
/**
 * @def var_error(s,...)
 * @brief Error macro which accepts variable parameters. Message is expanded to
 * include location of call in the source file along with other information.
 * The message after expansion is saved in a global log buffer and then finally
 * passed to log_generic().
 */
#define var_error(s, ...) pthread_mutex_lock(&acp_state.log_buffer_lock); \
						   snprintf(acp_state.log_buffer,LOG_BUFF_SIZE, "[" __FILE__ ":%i] Error: " s "",\
		                            __LINE__,__VA_ARGS__);\
		                   log_generic(acp_state.log_buffer,LOG_ERROR);\
	                       pthread_mutex_unlock(&acp_state.log_buffer_lock);

/*
 * Error reporting can be done through two ways. first is to use macros
 * which in turn use log_generic(). other is to use report_error() which accepts
 * error codes in place of message strings. report_error() after converting the
 * error code to a string calls log_generic().
 */

//routines

int init_acp_state();
void log_generic(const char* msg, log_level_t log_level);
void report_error(error_codes_t error_code); //report errors which are fatal
void close_ui();
void destroy_win(WINDOW *local_win);
void set_focus_to_console();
void destroy_cdkscreens();
void display_help();
long int gettime_in_nsecs();
int open_log_file();

// store and load routines for 16,32 and 64 bit integers into/from unsigned char buffers
void store16(unsigned char *buff, short int num) ;
void load16(unsigned char *buff, unsigned short int *num);
void store32(unsigned char *buff, unsigned int num);
void load32(unsigned char *buff, unsigned int *num);
void store64(unsigned char *buff, unsigned long int num);
void load64(unsigned char *buff, unsigned long int *num);
#endif

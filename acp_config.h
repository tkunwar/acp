/**
 * @file acp_config.h
 * @brief Define configuration options. variables that might alter the
 * 		 behaviour of program such as window sizes, swappiness factor etc. Also
 * 		 contains the structure for saving the information loaded from config
 * 		 file. Globally accessible .
 * @author Tej
 */
#ifndef _ACP_CONFIG_H_
#define _ACP_CONFIG_H_

#ifndef ACP_DTYPES_H_
	#include "acp_dtypes.h"
#endif

#include<stdio.h>
#include<unistd.h>
#include <string.h>
#include<libconfig.h>

#include "acp_error.h"

/*
 * the program's max. screen coordinates for our program
 */
/**
 * @def MIN_LINES
 * @brief Minimum lines needed for acp to init UI mode.
 */
#define MIN_LINES 40
/**
 * @def MIN_COLS
 * @brief Minimum columns needed for acp to init UI mode.
 */
#define MIN_COLS 100

/*
 * status windows will be given preference over console window.
 * so first sizes of the two windows will be calculated then console's size.
 * if needed, console size can be overriden.
 *
 * Size is represented in no. of lines or rows.
 */
/**
 * @def MENUBAR_SIZE
 * @brief Size of menubar window in no. of lines.
 */
#define MENUBAR_SIZE 3
/**
 * @def CONSOLE_SIZE
 * @brief Size of console window in no. of lines.
 */
#define CONSOLE_SIZE 6
/**
 * @def STATUS_WINDOW_SIZE
 * @brief Size of status_window in no. of lines.
 */
#define STATUS_WINDOW_SIZE 12

/**
 * @def RAW_MODE
 * @brief Turn on/off curses raw mode
 */
#define RAW_MODE off
/**
 * @def NOECHO
 * @brief Turn on/off curses no-echo feature
 */
#define NOECHO on
/**
 * @def KEYPAD_MODE
 * @brief Turn on/off curses keypad feature
 */
#define KEYPAD_MODE on
/**
 * @def CBREAK_MODE
 * @brief Turn on/off cbreak feature of curses
 */
#define CBREAK_MODE on
/**
 * @def ACP_CONF_FILE
 * @brief Path of Configuration file
 */
#define ACP_CONF_FILE "acp.conf"
/**
 * @struct acp_config_t
 * @brief Structure for storing information loaded from configuration file.
 */
struct acp_config_t {
	int max_buff_size;
	int swappiness;
	bool cell_compaction_enabled;
	unsigned int stats_refresh_rate; // in seconds
	char log_filename[200];
} acp_config;

// --------routines-----------
int load_config();
#endif


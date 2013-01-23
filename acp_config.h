/*
 * acp_config.h
 *
 *  Created on: Nov 8, 2012
 *      Author: tej
 *  Description: define configuration options. variables that might alter the
 * 				behaviour of program such as window sizes, swappiness factor etc.
 *
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
#define MIN_LINES 40
#define MIN_COLS 100

/*
 * status windows will be given preference over console window.
 * so first sizes of the two windows will be calculated then console's size.
 * if needed, console size can be overriden.
 *
 * Size is represented in no. of lines or rows.
 */

#define MENUBAR_SIZE 3
#define CONSOLE_SIZE 6
#define STATUS_WINDOW_SIZE 12

//modify initialization behaviour of acp
#define RAW_MODE off
#define NOECHO on
#define KEYPAD_MODE on
#define CBREAK_MODE on

#define ACP_CONF_FILE "acp.conf"

struct acp_config_t {
	int max_buff_size;
	int swappiness;
	bool cell_compaction_enabled;
} acp_config;

// --------routines-----------
int load_config();
#endif


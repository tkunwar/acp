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

/*
 * the program's max. screen coordinates for our program
 */
#define MIN_LINES 40
#define MIN_COLS 100

/*
 * status windows wil be given preference over console window.
 * so first sizes of the two windows will be calcualted then console's size.
 * if needed, console size can be overrriden.
 *
 * Size is represented in no. of lines or rows.
 */

#define MENUBAR_SIZE 3
#define CONSOLE_SIZE 6
#define STATUS_WINDOW_SIZE 12

#define ON 1
#define OFF 0

//modify initialization behaviour of acp
#define RAW_MODE OFF
#define NOECHO ON
#define KEYPAD_MODE ON
#define CBREAK_MODE ON


#endif


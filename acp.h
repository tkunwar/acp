/*
 * acp.h
 * main include file for acp
 */
#ifndef _ACP_H_
#define _ACP_H_

#include "acp_common.h"
#include "acp_gui.h"

/*
 * after UI has been initialized go in a infinite loop to wait for user input
 * accordingly process.
 */
void process_user_response();
void acp_shutdown(); //close down program -- successfull termination
void display_help();
#endif

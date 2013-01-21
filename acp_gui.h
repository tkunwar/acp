/*
 * acp_gui.h
 *
 *  Created on: Nov 8, 2012
 *      Author: tej
 *  Description : define gui related routines/variables that will be
 *  			accessible from outside code.
 */
/* WINDOW drawing rules
 * --------------------
 * window's coordinates are specified in its handle in acp_state
 *  do not cross these coordinates for.e.g writing must be in between
 *  console.wptr.beg_x+1 till console.wptr.height-1
 *  and width:
 *  	conso;e.wptr.beg_y+1 till console.wptr.width-1
 *
 */
#ifndef _ACP_GUI_H_
#define _ACP_GUI_H_

#include"acp_common.h"

struct acp_ui_label {
	WINDOW *wptr; // in which window does this label exist ?
	int beg_x, beg_y; //the starting coordinates
	char caption[30]; //label fot this label
	int last_label_text_len; /* what was label's last text length ? will help
	 in deleting that many characters when new label
	 text is to be displayed.
	 */
};
//routines
int init_curses(); //initialize curses mode and other secondary routines
int acp_ui_main(); //draw the curses UI for ACP
#endif


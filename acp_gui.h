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
void init_curses(); //initialize curses mode and other secondary routines
void acp_ui_main(); //draw the curses UI for ACP
WINDOW *create_newwin(int height, int width, int starty, int startx);

void draw_windows(); //draw parent windows
struct acp_ui_label *create_new_label(WINDOW *ptr, int beg_x, int beg_y,
		char *caption);
void draw_menubar();
void calculate_padding(int *hor_padding, int *ver_padding);
void print_window_state(struct window_state_t window);
void init_cdk();
void check_window_configuration();
void draw_GMM_window();
void draw_CMM_window();
void create_cdkscreens();
void draw_console();
#endif


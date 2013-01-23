/**
 * @file acp_gui.h
 * @brief header file for acp_gui.c
 * @author Tej
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

#ifndef ACP_DTYPES_H_
	#include "acp_dtypes.h"
#endif

#include"acp_common.h"
/**
 * @struct acp_ui_label
 * @brief Defines parameters of label in CDK context.
 */
struct acp_ui_label {
    WINDOW *wptr; /**< In which window does this label exist */
    int beg_x; /** The starting x coordinate */
    int beg_y; /** The starting y coordinate */
    char caption[30]; /**< caption for this label */
    int last_label_text_len; /**< what was label's last text length ? will help
	 	 	 	 	 	 	 	 in deleting that many characters when new label
	 	 	 	 	 	 	 	 text is to be displayed. */
};
//routines
int init_curses(); //initialize curses mode and other secondary routines
int acp_ui_main(); //draw the curses UI for ACP
#endif


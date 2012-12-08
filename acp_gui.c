/*
 * acp_gui.c
 *
 *  Created on: Nov 8, 2012
 *      Author: tej
 */
#include "acp_gui.h"
void init_curses() {
	//master curses window
	acp_state.cursesWin = initscr();
	//enusre that we do not have any window configuration mismatch before we
	//proceed any futher
	check_window_configuration();

	//init cdk specific routines
	init_cdk();
	//now enable some specific features
	if (CBREAK_MODE)
		cbreak(); //disable line buffering
	if (RAW_MODE)
		raw(); //get everything that is passed to u
	if (KEYPAD_MODE)
		keypad(stdscr, TRUE); //enable to accept key presses like F1,F2 etc.
	if (NOECHO)
		noecho(); //disable echoing
}
/**
 * @@draw_windows()
 * Description: draw three master windows. return error code accordingly.
 */
void acp_ui_main() {
	//first draw parent windows
	draw_windows();
	create_cdkscreens();
	//now display intial display screen for these windows
	draw_menubar();
//	draw_info_window();
	draw_GMM_window();
	draw_CMM_window();
	draw_console();
	//set gui_ok in acp_state
	acp_state.gui_ready = TRUE;
	//every thing is fine so far
}
void create_cdkscreens() {
	//by now we have window pointers, lets create cdkscreens out of them.
	acp_state.console_win.cdksptr = initCDKScreen(acp_state.console_win.wptr);
	acp_state.menubar_win.cdksptr = initCDKScreen(acp_state.menubar_win.wptr);
	acp_state.GMM_win.cdksptr = initCDKScreen(acp_state.GMM_win.wptr);
	acp_state.CMM_win.cdksptr = initCDKScreen(acp_state.CMM_win.wptr);

	//also create the master cdkscreen which will be used for popup messages.
	acp_state.master_screen = initCDKScreen(acp_state.cursesWin);
}
void draw_windows() {
	/*
	 * 1.	calculate size of these three windows. Size of menubar and console
	 * 		are provided in acp_config.h.
	 * 2.	draw these windows. Also report any error as fatal if any occur
	 * 		while drawing windows
	 * 3.	also draw border around these windows.
	 * 4.	store window pointers in the acp_state structure with relevant
	 * 		information for windows.
	 *
	 * 	Note: these three master windows have full width equal to COLS
	 */

	int horizontal_padding, vertical_padding;
	int height, width, startx, starty = 1;

	/*
	 * leave one line after meubar_win, one after info_bar and one for the
	 * generic_debug line.
	 */
	//calculate paddings if needed
	calculate_padding(&horizontal_padding, &vertical_padding);

	//draw  window menubar
	height = MENUBAR_SIZE;
	width = MIN_COLS;
	startx = horizontal_padding;
	starty = vertical_padding;
	if ((acp_state.menubar_win.wptr = create_newwin(height, width, starty,
			startx)) == NULL)
		exit_acp("Error: failed to create window:menubar");
	//save the starting coordinates of menubar
	acp_state.menubar_win.beg_x = startx;
	acp_state.menubar_win.beg_y = starty;
	acp_state.menubar_win.height = height;
	acp_state.menubar_win.width = width;

	//draw window GMM_win
	starty += height + 1;
	startx = horizontal_padding;
	height = STATUS_WINDOW_SIZE;
	width = MIN_COLS;

	if ((acp_state.GMM_win.wptr = create_newwin(height, width, starty, startx))
			== NULL)
		exit_acp("Error: failed to create window:info_win");
	acp_state.GMM_win.beg_x = startx;
	acp_state.GMM_win.beg_y = starty;
	acp_state.GMM_win.height = height;
	acp_state.GMM_win.width = width;

	//draw window CMM_win
	starty += height + 1;
	startx = horizontal_padding;
	height = STATUS_WINDOW_SIZE;
	width = MIN_COLS;

	if ((acp_state.CMM_win.wptr = create_newwin(height, width, starty, startx))
			== NULL)
		exit_acp("Error: failed to create window:info_win");
	acp_state.CMM_win.beg_x = startx;
	acp_state.CMM_win.beg_y = starty;
	acp_state.CMM_win.height = height;
	acp_state.CMM_win.width = width;

	//draw window console
	starty += height + 1;
	startx = horizontal_padding;
	height = MIN_LINES
			- (MENUBAR_SIZE + 1 + STATUS_WINDOW_SIZE + 1 + STATUS_WINDOW_SIZE
					+ 1);
	width = MIN_COLS;

	if ((acp_state.console_win.wptr = create_newwin(height, width, starty,
			startx)) == NULL)
		exit_acp("Error: failed to create window:console_win");

}
WINDOW *create_newwin(int height, int width, int starty, int startx) {
	WINDOW *local_win;

	local_win = newwin(height, width, starty, startx);
	refresh();
	if (!local_win)
		return NULL;
	box(local_win, 0, 0);

	/* 0, 0 gives default characters
	 * for the vertical and horizontal
	 * lines			*/
	wrefresh(local_win); /* Show that box 		*/
	return local_win;
}

void draw_menubar() {
	CDKLABEL *lblHelp;
	CDKLABEL *lblConfig;
	CDKLABEL *lblExit;
	const char *lblText[2]; //means 2 lines of strings of indefinite length
	//label for this window
	lblText[0] = "<C>Window Controls";
	lblHelp = newCDKLabel(acp_state.menubar_win.cdksptr, CENTER, TOP,
			(CDK_CSTRING2) lblText, 1, FALSE, FALSE);
	drawCDKLabel(lblHelp, ObjOf (lblHelp)->box);

	/*
	 * to create a horizontal menu of three labels using actually one label
	 * we can simply fill up this lblText[0] with spaces and other names.
	 */
	lblText[0] = "<C></U>F2:Help";
	lblHelp = newCDKLabel(acp_state.menubar_win.cdksptr, LEFT, CENTER,
			(CDK_CSTRING2) lblText, 1, FALSE, FALSE);
	drawCDKLabel(lblHelp, ObjOf (lblHelp)->box);

	lblText[0] = "<C></U>F3:Config";
	lblConfig = newCDKLabel(acp_state.menubar_win.cdksptr, CENTER, CENTER,
			(CDK_CSTRING2) lblText, 1, FALSE, FALSE);
	drawCDKLabel(lblConfig, ObjOf (lblConfig)->box);

	lblText[0] = "<C></U>F4:Exit";
	lblExit = newCDKLabel(acp_state.menubar_win.cdksptr, RIGHT - 2, CENTER,
			(CDK_CSTRING2) lblText, 1, FALSE, FALSE);
	drawCDKLabel(lblExit, ObjOf (lblExit)->box);
}

void print_window_state(struct window_state_t window) {
	var_debug("draw_info: window_state: beg_y %d beg_x %d width %d height %d",
			window.beg_y, window.beg_x, window.width, window.height);
}
void calculate_padding(int *hor_padding, int *ver_padding) {
	*hor_padding = (COLS - MIN_COLS) / 2;
	*ver_padding = (LINES - MIN_LINES) / 2;
	acp_state.hori_pad = *hor_padding;
	acp_state.vert_pad = *ver_padding;
}
void init_cdk() {
	//for all the windows that we want lets create cdkscreen out of them

//	acp_state.console_cdkscreen = initCDKScreen(acp_state.cursesWin);
	initCDKColor();
}

void check_window_configuration() {
	int min_req_height;
	//parent window size check
	if (COLS < MIN_COLS || LINES < MIN_LINES) {
		report_error(ACP_ERR_WINDOW_SIZE_TOO_SHORT);
	}
	/*
	 * ensure that window sizes are correctly speccified. we check if we
	 * adding all the window sizes plus 1 separators each time, we still have
	 * sufficient LINES remaining.
	 * for e.g. MENUBAR_SIZE+1+STATUS_WINDOW_SIZE(for GMM_win)+1+
	 * STATUS_WINDOW_SIZE(for CMM_win) + 1+CONSOLE_SIZE must be less than or
	 * equal to MIN_LINES. However note that we can adjust the size of
	 * console. However we still need at least 2 lines minimum for console.
	 *
	 */
	min_req_height = MENUBAR_SIZE + 1 + STATUS_WINDOW_SIZE + 1
			+ STATUS_WINDOW_SIZE + 1 + 3;
	if (MIN_LINES < min_req_height) {
		report_error(ACP_ERR_WINDOW_CONFIG_MISMATCH);
	}
}

void draw_GMM_window() {
	const char *lblText[2];
	CDKLABEL *lblTitle = NULL;
	//set the title bar
	lblText[0] = "<C>General Memory Model";
	lblTitle = newCDKLabel(acp_state.GMM_win.cdksptr, CENTER, TOP,
			(CDK_CSTRING2) lblText, 1, FALSE, FALSE);
	drawCDKLabel(lblTitle, ObjOf (lblTitle)->box);

}
void draw_CMM_window() {
	//set the title bar
	const char *lblText[2];
	CDKLABEL *lblTitle = NULL;
	//set the title bar
	lblText[0] = "<C>Compressed Memory Model";
	lblTitle = newCDKLabel(acp_state.CMM_win.cdksptr, CENTER, TOP,
			(CDK_CSTRING2) lblText, 1, FALSE, FALSE);
	drawCDKLabel(lblTitle, ObjOf (lblTitle)->box);
}
void draw_console() {
	const char *console_title = "<C></B/U/7>Log Window";
	/* Create the scrolling window. */
	acp_state.console = newCDKSwindow(acp_state.console_win.cdksptr,
			CDKparamValue(&acp_state.params, 'X', CENTER),
			CDKparamValue(&acp_state.params, 'Y', CENTER),
			CDKparamValue(&acp_state.params, 'H', acp_state.console_win.height),
			CDKparamValue(&acp_state.params, 'W', acp_state.console_win.height),
			console_title, 100, CDKparamValue(&acp_state.params, 'N', TRUE),
			CDKparamValue(&acp_state.params, 'S', FALSE));

	/* Is the window null. */
	if (acp_state.console == 0) {
		report_error(ACP_ERR_CDK_CONSOLE_DRAW);
	}
	/* Draw the scrolling window. */
	drawCDKSwindow(acp_state.console, ObjOf (acp_state.console)->box);
}

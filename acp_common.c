/*
 * acp_common.c
 *
 *  Created on: Nov 17, 2012
 *      Author: tej
 */
#include "acp_common.h"
/*
 * @@exit_acp(char *msg)
 * Description: Reports error but fatal ones only. After that program will exit.
 * 				If gui_ready is set then before exiting call close_gui()
 */
void exit_acp(const char *msg) {

	var_error("%s", msg);
	getch();

	if (acp_state.gui_ready == TRUE) {
		close_ui();
	} else {
		/* Exit CDK. */
		destroy_cdkscreens();
		endCDK();
	}
	exit(EXIT_FAILURE);
}
void destroy_cdkscreens() {
	destroyCDKScreen(acp_state.menubar_win.cdksptr);
	destroyCDKScreen(acp_state.GMM_win.cdksptr);
	destroyCDKScreen(acp_state.CMM_win.cdksptr);
	destroyCDKScreen(acp_state.console_win.cdksptr);
}
void init_acp_state() {
	//reset the acp_state
	init_window_pointers();
	init_cdks_pointers();
	acp_state.color_ok = FALSE;
	strcpy(acp_state.log_buffer, "acp_init: In progress");
	acp_state.gui_ready = FALSE;
	/*
	 * now for all three windows we set the cur_x and cur_y to 0
	 */
	acp_state.menubar_win.beg_x = acp_state.menubar_win.beg_y =
			acp_state.menubar_win.cur_x = acp_state.menubar_win.cur_y =
					acp_state.menubar_win.height = acp_state.menubar_win.width =
							0;

	acp_state.CMM_win.beg_x = acp_state.CMM_win.beg_y =
			acp_state.CMM_win.cur_x = acp_state.CMM_win.cur_y =
					acp_state.CMM_win.height = acp_state.CMM_win.width = 0;

	acp_state.GMM_win.beg_x = acp_state.GMM_win.beg_y =
			acp_state.GMM_win.cur_x = acp_state.GMM_win.cur_y =
					acp_state.GMM_win.height = acp_state.GMM_win.width = 0;

	//init cdk specific pointers
	acp_state.console = NULL;
	acp_state.cursesWin = NULL;

	//TODO: also init global_labels
}
void init_window_pointers() {
	acp_state.menubar_win.wptr = acp_state.GMM_win.wptr =
			acp_state.CMM_win.wptr = acp_state.console_win.wptr = NULL;
}
void init_cdks_pointers() {
	acp_state.menubar_win.cdksptr = acp_state.GMM_win.cdksptr =
			acp_state.CMM_win.cdksptr = acp_state.console_win.cdksptr = NULL;
	acp_state.master_screen = NULL;
}
/**
 * @@debug_generic(const char *msg)
 * Description: prints msg at LINES-1,COLS/2. Avoid using it. as it will
 * 				overwrite  last window. Also requires a full screen refresh.
 */
void log_generic(const char* msg, log_level_t log_level) {
	if (acp_state.gui_ready) {
		//log this msg to console
		log_msg_to_console(msg, log_level);
		return;
	}
	int msg_len = strlen(msg);
	int beg_x = calculate_msg_center_position(msg_len);
	/*
	 * if previously a message was logged, then logging this msg will
	 * corrupt the display. so we will delete this line and write the msg
	 *
	 */
	move(LINES-1, 0);
	deleteln();
	//donot forget to add horizontal padding
	mvprintw(LINES - 1, beg_x + acp_state.hori_pad, "%s", msg);
	refresh();
}

/**
 * @@calculate_msg_center_position(int msg_len)
 * Description : returns the x position where msg should be displayed such that
 * 				it appears in the center location
 */
int calculate_msg_center_position(int msg_len) {
	return ((MIN_COLS - msg_len) / 2);
}

/**
 * @@report_error(error_code)
 * Description: concerts error codes into understandable error strings.
 * 				Note that not all error might lead to program termination.
 * 				For those which might, route the msg string to exit_acp()
 */
void report_error(error_codes_t error_code) {
	char msg[LOG_BUFF_SIZE];
	int exit_needed = 0;
	switch (error_code) {
	case ACP_ERR_INIT_GUI:
		snprintf(msg, LOG_BUFF_SIZE, "Failed to initialize curses UI!");
		exit_needed = 1;
		break;
	case ACP_ERR_NO_COLOR:
		snprintf(msg, LOG_BUFF_SIZE, "No color support !");
		break;
	case ACP_ERR_COLOR_INIT_FAILED:
		snprintf(msg, LOG_BUFF_SIZE, "Failed to initialize color support !");
		break;
	case ACP_ERR_WINDOW_SIZE_TOO_SHORT:
		snprintf(msg, LOG_BUFF_SIZE,
				"Window size too short.Must be %d LINESx %d COLS!", MIN_LINES,
				MIN_COLS);
		exit_needed = 1;
		break;
	case ACP_ERR_CDK_CONSOLE_DRAW:
		snprintf(msg, LOG_BUFF_SIZE, "Failed to draw CDK window: console");
		exit_needed = 1;
		break;
	case ACP_ERR_WINDOW_CONFIG_MISMATCH:
		snprintf(msg, LOG_BUFF_SIZE,
				"Window configuration mismatch, check acp_config.h");
		exit_needed = 1;
		break;
	case ACP_ERR_GENERIC:
		snprintf(msg, LOG_BUFF_SIZE, "An unknown error has occurred!");
		break;
	default:
		snprintf(msg, LOG_BUFF_SIZE, "An unknown error has occurred!");
		break;
	}
	if (exit_needed)
		exit_acp(msg);
	else
		//termination not needed so just report the error
		var_error("%s", msg);
}
void close_ui() {
	sdebug("Closing down UI");
	//delete windows
	destroy_acp_window();
	//all windows and associated cdkscreens are gone now
	acp_state.gui_ready = FALSE;

	sdebug("Press any key to exit!!");
	getch();

	/* Exit CDK. */
//	destroy_cdkscreens();
	endCDK();
	endwin();
}

void destroy_acp_window() {
	//destroy window as well as evrything it contains--label,cdkscreens,widgets
	//etc.
	//order can be widgets->cdkscreen->window_pointers
	//place any window specific code in the case that matches

	//first cdkscreens
	destroy_cdkscreens();

	//now window pointers
	delwin(acp_state.menubar_win.wptr);
	delwin(acp_state.GMM_win.wptr);
	delwin(acp_state.CMM_win.wptr);
	delwin(acp_state.console_win.wptr);

}
void acp_mv_wprintw(struct window_state_t *win, int new_y, int new_x,
		char *string) {
	//update cur_x and cur_y
	win->cur_x = new_x;
	win->cur_y = new_y;

	//now at this new position display the string
	mvwprintw(win->wptr, win->cur_y, win->cur_x, "%s", string);
}
/**
 * @@set_focus_to_console()
 * Description: set focus to CDK console
 */
void set_focus_to_console() {
	activateCDKSwindow(acp_state.console, 0);
}

/**
 * @@log_msg_to_console(msg)
 * Description: log this message to CDK console that we have created.When gui
 * 				is fully ready, then log_genric() will call this routine only.
 */
void log_msg_to_console(const char *msg, log_level_t log_level) {
	char log_msg[LOG_BUFF_SIZE];
	/*
	 * color scheme: <C> is for centering
	 * </XX> defines a color scheme out of 64 available
	 * <!XX> ends turns this scheme off
	 *
	 * some of interest:
	 * 	32: yellow foreground on black background
	 * 	24: green foreground on black background
	 * 	16: red foreground on black background
	 * 	17: white foreground on black background
	 */
	switch (log_level) {
	case LOG_DEBUG:
		snprintf(log_msg, LOG_BUFF_SIZE, "<C></24>%s<!24>", msg);
		break;
	case LOG_WARN:
		snprintf(log_msg, LOG_BUFF_SIZE, "<C></32>%s<!32>", msg);
		break;
	case LOG_ERROR:
		snprintf(log_msg, LOG_BUFF_SIZE, "<C></16>%s<!16>", msg);
		break;
	default:
		//centre placement but no color
		snprintf(log_msg, LOG_BUFF_SIZE, "<C>%s", msg);
		break;
	}

	addCDKSwindow(acp_state.console, log_msg, BOTTOM);
}

void redraw_cdkscreens() {
	drawCDKScreen(acp_state.menubar_win.cdksptr);
	drawCDKScreen(acp_state.GMM_win.cdksptr);
	drawCDKScreen(acp_state.CMM_win.cdksptr);
	drawCDKScreen(acp_state.console_win.cdksptr);
}

void print_max_console_size(){
	var_debug("Max. console size: Max X: %d Max Y: %d",COLS,LINES);
}

/**
 * @file acp_common.c
 * @brief This file includes common code which will be used by other sections
 * 		  of acp. It includes logging macros,routines and init routines.
 */
#include "acp_common.h"
//=========== routines declaration============
static int calculate_msg_center_position(int msg_len);
static void log_msg_to_console(const char* msg, log_level_t level);
static void init_window_pointers();
static void init_cdks_pointers();
static void destroy_acp_window(); //main routine for deleting windows
static void redraw_cdkscreens(); //draw cdkscrens after drawing a popup window or
//===========================================
/**
 * @brief Destroy cdk screen pointers that are present in acp_state.
 */
void destroy_cdkscreens() {
    destroyCDKScreen(acp_state.menubar_win.cdksptr);
    destroyCDKScreen(acp_state.GMM_win.cdksptr);
    destroyCDKScreen(acp_state.CMM_win.cdksptr);
    destroyCDKScreen(acp_state.console_win.cdksptr);
}
/**
 * @brief Init the acp_state
 *
 * Initialize acp_state structure which is accessible throughout. Ensure
 * that pointers and other variables are initialized to null or empty or
 * relevant values.
 * @return Return ACP_OK if operation was successful else an error code.
 */
int init_acp_state() {
    //reset the acp_state
    init_window_pointers();
    init_cdks_pointers();
    acp_state.color_ok = false;
    acp_state.gui_ready = false;
    acp_state.cursesWin = NULL;
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

    //init the log_buffer_lock
    if ((pthread_mutex_init(&acp_state.log_buffer_lock, NULL)) != 0) {
        fprintf(stderr, "\nprocess1_mutex_init failed");
        return ACP_ERR_ACP_STATE_INIT;
    }
    //TODO: also init global_labels
    return ACP_OK;
}
/**
 * @brief Initialize window pointers of acp_state
 */
static void init_window_pointers() {
    acp_state.menubar_win.wptr = acp_state.GMM_win.wptr =
                                     acp_state.CMM_win.wptr = acp_state.console_win.wptr = NULL;
}
/**
 * @brief Initialize cdk screen pointers of acp_state
 */
static void init_cdks_pointers() {
    acp_state.menubar_win.cdksptr = acp_state.GMM_win.cdksptr =
                                        acp_state.CMM_win.cdksptr = acp_state.console_win.cdksptr = NULL;
    acp_state.master_screen = NULL;
}
/**
 * @brief Generic logger routine.
 *
 * Prints a message at LINES-1,COLS/2. Avoid using it. as it will overwrite
 * last logged message. Also requires a full screen refresh.
 * @param msg The message which is to be logged
 * @param log_level The priority of this message- whether it's a debug,warning
 * 			or error message.
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
     */
    move(LINES-1, 0);
    deleteln();
    //donot forget to add horizontal padding
    mvprintw(LINES - 1, beg_x + acp_state.hori_pad, "%s", msg);
    refresh();
}

/**
 * @brief Finds central position where a message can be displayed.
 *
 * Returns the position where message should be displayed such that	it appears
 * in the center of screen.
 * @param msg_len Length of message to be displayed
 * @return The beginning location from where message will be displayed
 */
static int calculate_msg_center_position(int msg_len) {
    return ((MIN_COLS - msg_len) / 2);
}

/**
 * @brief Converts error codes into understandable error strings.
 * @param error_code The error code which is to be converted
 */
void report_error(error_codes_t error_code) {
    char msg[LOG_BUFF_SIZE];
    switch (error_code) {
    case ACP_ERR_INIT_UI:
        snprintf(msg, LOG_BUFF_SIZE, "Failed to initialize curses UI!");
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
        break;
    case ACP_ERR_CDK_CONSOLE_DRAW:
        snprintf(msg, LOG_BUFF_SIZE, "Failed to draw CDK window: console");
        break;
    case ACP_ERR_WINDOW_CONFIG_MISMATCH:
        snprintf(msg, LOG_BUFF_SIZE,
                 "Window configuration mismatch, check acp_config.h");
        break;
    case ACP_ERR_GENERIC:
        snprintf(msg, LOG_BUFF_SIZE, "An unknown error has occurred!");
        break;
    default:
        snprintf(msg, LOG_BUFF_SIZE, "An unknown error has occurred!");
        break;
    }
    //just report the error
    log_generic(msg,LOG_ERROR);
}
/**
 * @brief Destroys all windows and clears UI.
 *
 * Call it when acp is being closed down.
 */
void close_ui() {
    sdebug("Closing down UI");
    //delete windows
    destroy_acp_window();
    //all windows and associated cdkscreens are gone now
    acp_state.gui_ready = false;

    sdebug("Press any key to exit!!");
    getch();

    /* Exit CDK. */
    endCDK();
    endwin();
}
/**
 * @brief Destroys all window pointers.
 */
static void destroy_acp_window() {
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
/**
 * @brief Brings focus to console window
 *
 * When processing user response through key presses one of the keys might
 * trigger events in other windows causing console window to loose focus. Then it
 * becomes necessary to transfer focus to console window. For e.g. as F2 will
 * bring Help window, giving focus to console is necessary else, program will
 * no longer be able to get attention.
 */
void set_focus_to_console() {
    activateCDKSwindow(acp_state.console, 0);
}

/**
 * Log a message to CDK console that we have created. When UI is fully ready,
 * then log_generic() will call this routine only.
 * @param msg The message which is to be logged
 * @param log_level One of LOG_DEBUG,LOG_WARN or LOG_ERR
 */
static void log_msg_to_console(const char *msg, log_level_t log_level) {
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
/**
 * After a popup screen is drawn in CDK, it is necessary to redraw under-
 * lying CDK windows otherwise they will not be displayed on the screen and
 * program is left in an inconsistent state.
 */
static void redraw_cdkscreens() {
    drawCDKScreen(acp_state.menubar_win.cdksptr);
    drawCDKScreen(acp_state.GMM_win.cdksptr);
    drawCDKScreen(acp_state.CMM_win.cdksptr);
    drawCDKScreen(acp_state.console_win.cdksptr);
}
/**
 * Displays help options in a popup window. Also do a redraw of all cdkscreens
 * since we will be displaying popup windows in master_cdkscreen.
 */
void display_help() {
    const char *mesg[15];
    const char *buttons[] = { "<OK>" };
    mesg[0] = "<C>This program shows how a compressed paging mechanism has";
    mesg[1] = "<C>advantage over traditional paging systems.";
    mesg[2] = "<C></U>Project authors";
    mesg[3] = "<C>Amrita,Rashmi and Varsha";

    popupDialog(acp_state.master_screen, (CDK_CSTRING2) mesg, 4,
                (CDK_CSTRING2) buttons, 1);
    redraw_cdkscreens();
}

/**
 * acp.c
 * main c source file for acp
 */
#include"acp.h"

int main(int argc, char *argv[]) {
	CDKparseParams(argc, argv, &acp_state.params, CDK_CLI_PARAMS);
	//initialize acp_state
	init_acp_state();
	//initialize curses and cdk mode
	init_curses();
	//try to draw windows
	acp_ui_main();
	//now wait for user input and process key-presses
	process_user_response();
	exit(EXIT_SUCCESS);
}

/**
 * @@process_user_response()
 * Description: now that we have everyting ready, we go in a loop
 * 				to wait for user response (KBHITs) and take actions
 * 				accordingly.
 */
void process_user_response() {
	int ch;
	//the main loop for waiting for key presses
	while ((ch = getch())) {
		switch (ch) {
		case KEY_F(4):
			acp_shutdown();
			break;
		case KEY_F(2):
				display_help();
				break;
		case KEY_UP:
			set_focus_to_console();
			break;
		case KEY_DOWN:
			set_focus_to_console();
			break;
		default:
			//write to the console window for now
			var_debug("key pressed: %c", ch);
			break;
		}
	}
}
/*
 * @@acp_shutdown()
 * Description: shutdown program. clean up any memory blocks,files etc.
 * 				We are terminating safely. Remember not to log/; use UI in any
 * 				way since we do not have ncurses. Therefore, shutdown UI must
 * 				be the last thing to do.
 */
void acp_shutdown() {
	//other stuff related to cleanup

	//shut down GUI
	close_ui();
	exit(EXIT_SUCCESS);
}
/**
 * @@display_help()
 * Description: Displays help options in a popup window. Also do a redraw of
 * 				all cdkscreens since we will be displaying popup windows
 * 				in master_cdkscreen.
 */
void display_help(){
	const char *mesg[15];
	const char *buttons[] =
		    {
		       "<OK>"
		    };
			mesg[0]="<C>This program shows how a compressed paging mechanism has";
			mesg[1]="<C>advantage over traditional paging systems.";
		    mesg[2] = "<C></U>Project authors";
		    mesg[3] = "<C>Amrita,Rashmi and Varsha";

		  popupDialog (acp_state.master_screen,
				     (CDK_CSTRING2) mesg, 4,
				     (CDK_CSTRING2) buttons, 1);
		  redraw_cdkscreens();
}



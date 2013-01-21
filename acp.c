/**
 * acp.c
 * main c source file for acp
 */
#include"acp.h"
//========== routines declaration============
/*
 * after UI has been initialized go in a infinite loop to wait for user input
 * accordingly process.
 */
void process_user_response();
void acp_shutdown(); //close down program -- successfull termination
void exit_acp(); //call when fatal error has occurred such as window size too

//===========================================
int main(int argc, char *argv[]) {
	CDKparseParams(argc, argv, &acp_state.params, CDK_CLI_PARAMS);
	//initialize acp_state
	if(init_acp_state()!=ACP_OK){
		fprintf(stderr,"acp_state_init: failed. Aborting");
		exit(EXIT_FAILURE);
	}
	//initialize curses and cdk mode
	if (init_curses()!=ACP_OK){
		exit_acp();
	}
	//try to draw windows
	if (acp_ui_main()!= ACP_OK){
		exit_acp();
	}
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

/*
 * @@exit_acp(char *msg)
 * Description: Reports error but fatal ones only. After that program will exit.
 * 				If gui_ready is set then before exiting call close_gui()
 */
void exit_acp() {
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

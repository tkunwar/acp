/**
 * @file acp.c
 * @brief main source file for acp
 * @author Tej
 */
#include"acp.h"
//========== routines declaration============
void process_user_response();
void acp_shutdown(); //close down program -- successful termination
void exit_acp(); //call when fatal error has occurred such as window size too

//===========================================
/**
 * @brief main function. Entry point for acp.
 * @param argc No. of parameters passed to this program
 * @param argv A pointer to list of parameters
 * @return Sucess or failure to OS
 */
int main(int argc, char *argv[]) {
	CDKparseParams(argc, argv, &acp_state.params, CDK_CLI_PARAMS);
	//initialize acp_state
	if (init_acp_state() != ACP_OK) {
		fprintf(stderr, "acp_state_init: failed. Aborting");
		exit(EXIT_FAILURE);
	}
	//intialize configuration
	if (load_config() == ACP_ERR_CONFIG_ABORT) {
		exit(EXIT_FAILURE);
	}
    //initialize curses and cdk mode
    if (init_curses()!=ACP_OK) {
        exit_acp();
    }
    //try to draw windows
    if (acp_ui_main()!= ACP_OK) {
        exit_acp();
    }
    //now wait for user input and process key-presses
    process_user_response();
	exit(EXIT_SUCCESS);
}

/**
 * @brief Here we go in a loop to wait for user response (KBHITs) and take actions
 * accordingly.
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
/**
 * @brief Prepares program for shutdown.
 *
 * Clean up any memory blocks,files etc. We are terminating safely. Remember
 * not to log or use UI in any way since we do not have ncurses at this moment.
 * Therefore, shutdown UI must be the last thing to do.
 */
void acp_shutdown() {
	//other stuff related to cleanup

	//shut down GUI
	close_ui();
	exit(EXIT_SUCCESS);
}
/**
 * @brief Reports error but fatal ones only.
 *
 * After that program will exit.If gui_ready is set then before exiting call
 * close_gui()
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

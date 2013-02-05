/**
 * @file acp.c
 * @brief main source file for acp
 * @author Tej
 */
#include"acp.h"
//========== routines declaration============
void process_user_response();
void acp_shutdown(); //close down program -- successful termination
void exit_acp(bool got_signal); //call when fatal error has occurred such as window size too
static void signal_handler(int);
static void dumpstack(void);
static void cleanup(void);
void init_signals(void);
void panic(const char *, ...);

struct sigaction sigact;
char *progname;

//===========================================
/**
 * @brief main function. Entry point for acp.
 * @param argc No. of parameters passed to this program
 * @param argv A pointer to list of parameters
 * @return Sucess or failure to OS
 */
int main(int argc, char *argv[]) {
	progname = *(argv);
	atexit(cleanup);
	init_signals();

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
	if (init_curses() != ACP_OK) {
		exit_acp();
	}

	//try to draw windows
	if (acp_ui_main() != ACP_OK) {
		exit_acp();
	}
//	abort();
	//now wait for user input and process key-presses
	process_user_response();

	sigemptyset(&sigact.sa_mask);
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
void exit_acp(bool got_signal) {
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
void init_signals(void){
    sigact.sa_handler = signal_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, (struct sigaction *)NULL);

    sigaddset(&sigact.sa_mask, SIGSEGV);
    sigaction(SIGSEGV, &sigact, (struct sigaction *)NULL);

    sigaddset(&sigact.sa_mask, SIGBUS);
    sigaction(SIGBUS, &sigact, (struct sigaction *)NULL);

    sigaddset(&sigact.sa_mask, SIGQUIT);
    sigaction(SIGQUIT, &sigact, (struct sigaction *)NULL);

    sigaddset(&sigact.sa_mask, SIGHUP);
    sigaction(SIGHUP, &sigact, (struct sigaction *)NULL);

    sigaddset(&sigact.sa_mask, SIGKILL);
    sigaction(SIGKILL, &sigact, (struct sigaction *)NULL);
}

static void signal_handler(int sig){
    if (sig == SIGHUP) panic("FATAL: Program hanged up\n");
    if (sig == SIGSEGV || sig == SIGBUS){
        dumpstack();
        panic("FATAL: %s Fault. Logged StackTrace\n", (sig == SIGSEGV) ? "Segmentation" : ((sig == SIGBUS) ? "Bus" : "Unknown"));
    }
    if (sig == SIGQUIT) panic("QUIT signal ended program\n");
    if (sig == SIGKILL) panic("KILL signal ended program\n");
    if (sig == SIGINT) panic("Interrupted");
}

void panic(const char *fmt, ...){
    char buf[50];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(buf, fmt, argptr);
    va_end(argptr);
    fprintf(stderr, buf);
    exit(-1);
}

static void dumpstack(void){
    /* Got this routine from http://www.whitefang.com/unix/faq_toc.html
    ** Section 6.5. Modified to redirect to file to prevent clutter
    */
    /* This needs to be changed... */
    char dbx[160];

    sprintf(dbx, "echo 'where\ndetach' | dbx -a %d > %s.dump", getpid(), progname);
    /* Change the dbx to gdb */

    system(dbx);
    return;
}

void cleanup(void){
    sigemptyset(&sigact.sa_mask);
    /* Do any cleaning up chores here */

    exit_acp(true);
}

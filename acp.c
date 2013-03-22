/**
 * @file acp.c
 * @brief main source file for acp
 * @author Tej
 */
#include"acp.h"
#include "acp_gmm_utils.h"
#include "acp_cmm_utils.h"

#define ACTIVATE_GMM_MODULE
#define ACTIVATE_CMM_MODULE

//========== routines declaration============
void process_user_response();
void acp_shutdown(); //close down program -- successful termination
static void signal_handler(int);
static void cleanup_after_failure(void);
void init_signals(void);
static void stack_trace();
int start_main_worker_threads();

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
//	atexit(acp_shutdown);
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
	if (open_log_file() != ACP_OK) {
		exit(EXIT_FAILURE);
	}
	//initialize curses and cdk mode
	if (init_curses() != ACP_OK) {
		exit(EXIT_FAILURE);
	}

	//try to draw windows
	if (acp_ui_main() != ACP_OK) {
		exit(EXIT_FAILURE);
	}
//	abort();
	//now start main worker threads
	if (start_main_worker_threads() != ACP_OK) {
		exit(EXIT_FAILURE);
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
			var_debug("key pressed: %c", ch)
			;
			break;
		}
	}
}
/**
 * @brief Prepares program for shutdown.
 *
 * Clean up any memory blocks,files etc. This is a callback routine which will
 * be executed whenever program terminates. In simple words, it is the last
 * routine to be executed.
 */
void acp_shutdown() {
	//Are we are exiting because we got a signal ?
	if (acp_state.recieved_signal_code != 0) {
		acp_state.shutdown_in_progress = true;
		cleanup_after_failure();
		exit(EXIT_FAILURE);
	}

	//other stuff related to cleanup but it's a normal cleanup
	//signal that we are shutting down
	acp_state.shutdown_in_progress = true;
	var_debug("Parent sleeping for %d seconds to let child threads finish.",
			PARENT_WAIT_FOR_CHILD_THREADS);
	//wait for child threads
	sleep(PARENT_WAIT_FOR_CHILD_THREADS);
	//shut down GUI
	if (acp_state.gui_ready == TRUE) {
		close_ui();
	} else {
		/* Exit CDK. */
		destroy_cdkscreens();
		endCDK();
	}
	//collect master threads
#ifdef ACTIVATE_GMM_MODULE
	if (pthread_join(acp_state.gmm_main_thread, NULL ) != 0) {
		fprintf(stderr, "\nError in collecting thread: gmm_main");
		exit(EXIT_FAILURE);
	}
#endif
#ifdef ACTIVATE_CMM_MODULE
	if (pthread_join(acp_state.cmm_main_thread, NULL ) != 0) {
		fprintf(stderr, "\nError in collecting thread: cmm_main");
		exit(EXIT_FAILURE);
	}
#endif
	fclose(acp_state.log_ptr);
	sigemptyset(&sigact.sa_mask);
//	exit(EXIT_SUCCESS);
	ExitProgram(EXIT_SUCCESS);
}
/**
 * @brief Intialize our signal handler
 *
 * Register what all signals can we mask. These are those signals
 * that we will be able to catch in our program.
 */
void init_signals(void) {
	sigact.sa_handler = signal_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, (struct sigaction *) NULL );

	sigaddset(&sigact.sa_mask, SIGSEGV);
	sigaction(SIGSEGV, &sigact, (struct sigaction *) NULL );

	sigaddset(&sigact.sa_mask, SIGBUS);
	sigaction(SIGBUS, &sigact, (struct sigaction *) NULL );

	sigaddset(&sigact.sa_mask, SIGQUIT);
	sigaction(SIGQUIT, &sigact, (struct sigaction *) NULL );

	sigaddset(&sigact.sa_mask, SIGHUP);
	sigaction(SIGHUP, &sigact, (struct sigaction *) NULL );

	sigaddset(&sigact.sa_mask, SIGKILL);
	sigaction(SIGKILL, &sigact, (struct sigaction *) NULL );
}
/**
 * @brief Signal handler routine.
 *
 * When a signal is raised then this routine will catch them. In this routine
 * we will mostly set flags and make an exit call so that acp_shutdown can
 * begin the shutdown and cleanup process.
 *
 * @param sig The signal code
 */
static void signal_handler(int sig) {
	//set the signal code that we got
	acp_state.recieved_signal_code = sig;
//	var_debug("Got signal: %d", sig);
//	if (sig == SIGHUP)
//		sdebug("Got signal SIGHUP");
//	if (sig == SIGSEGV) {
//		sdebug("Got signal SIGSEGV");
//	}
//	if (sig == SIGBUS) {
//		sdebug("Got signal SIGBUS");
//	}
//	var_debug("sig: %d", sig);
//	if (sig == SIGQUIT)
//		sdebug("Got signal SIGQUIT");
//	var_debug("sig: %d", sig);
//	if (sig == SIGKILL)
//		sdebug("Got signal SIGKILL");
//	var_debug("sig: %d", sig);
//	if (sig == SIGINT)
//		sdebug("Got singal SIGINT");
	// Attempt to perform cleanup_after_failure when we have SIGINT,SIGKILL and SIGQUIT
	// else give up
//	if ((sig == SIGINT) || (sig == SIGQUIT) || (sig == SIGKILL)) {
	sdebug("Prepairing to exit gracefully..");
	acp_state.shutdown_in_progress = true;
	//wait while cleanup_after_failure is finished
	//a mutex lock would have been more efficient
//		while(acp_state.shutdown_completed==false){
//			sleep(1);
//		}
//	}
//	exit(EXIT_FAILURE);
	acp_shutdown();
}
/**
 * @brief This routine performs cleanup after a failure.
 *
 * Cleanup resources hold by threads. Note that we are exiting because
 * we received a signal.
 * TODO: Should not we try to wait for child threads or if possible cancel them.
 * 		Better would be that we allow threads to perform cleanup. Or atleast sleep
 * 		for a second or two. This will give threads time to cleanup unless they
 * 		are in waiting channel.
 */
void cleanup_after_failure(void) {
	// No access curses UI, back to plain stderr
	sigemptyset(&sigact.sa_mask);
	//collect master threads
#ifdef ACTIVATE_GMM_MODULE
	if (pthread_join(acp_state.gmm_main_thread, NULL ) != 0) {
		fprintf(stderr, "\nError in collecting thread: gmm_main");
		exit(EXIT_FAILURE);
	}
#endif
#ifdef ACTIVATE_CMM_MODULE
	if (pthread_join(acp_state.cmm_main_thread, NULL ) != 0) {
			fprintf(stderr, "\nError in collecting thread: cmm_main");
			exit(EXIT_FAILURE);
		}
#endif

	if (acp_state.gui_ready == TRUE) {
		close_ui();
	} else {
		/* Exit CDK. */
		destroy_cdkscreens();
		endCDK();
	}
	fclose(acp_state.log_ptr);
	//print a stack trace
	if (acp_state.recieved_signal_code == SIGSEGV
			|| acp_state.recieved_signal_code == SIGBUS) {
		stack_trace();
	}
}
/**
 * @brief Prints stack trace. We will print stack trace when either
 * SIGSEGV or SIGBUS was raised.
 */
static void stack_trace() {
	void *array[20];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace(array, 10);
	strings = backtrace_symbols(array, size);

	printf("Obtained %zd stack frames.\n", size);

	for (i = 0; i < size; i++)
		printf("%s\n", strings[i]);

	free(strings);
}

int start_main_worker_threads() {
	//create main acp_gmm thread
#ifdef ACTIVATE_GMM_MODULE
	if (pthread_create(&acp_state.gmm_main_thread, NULL, acp_gmm_main, NULL )
			!= 0) {
		serror("\ngmm_main_thread creation failed ");
		return ACP_ERR_THREAD_INIT;
	}
#endif
#ifdef ACTIVATE_CMM_MODULE
	if (pthread_create(&acp_state.cmm_main_thread, NULL, acp_cmm_main, NULL )
			!= 0) {
		serror("\ncmm_main_thread creation failed ");
		return ACP_ERR_THREAD_INIT;
	}
#endif
	return ACP_OK;
}

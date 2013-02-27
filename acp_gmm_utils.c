/**
 * @file acp_gmm_utils.c
 * @brief This file code to implement functionality for acp's GMM module.
 */
#include "acp_gmm_utils.h"
static void *gmm_swap_stats_collector(void *args);
static void *gmm_mem_stats_collector(void *args);
static void *gmm_swap_manager(void *args);
static void *gmm_mem_manager(void *args);

static void init_acp_gmm_state() {
	//these two variables wont be modifed during program run
	gmm_module_state.max_mem_limit = 0;
	gmm_module_state.max_page_limit = 0;

	// these values will be modified during program
	gmm_module_state.pages_active = 0;
	gmm_module_state.total_memory = 0;
	gmm_module_state.total_pages = 0;
	gmm_module_state.page_table = NULL;
	gmm_module_state.page_table_last_ele = NULL;
}
/**
 * @brief the main routine for acp_gmm module.
 * @param args Arguments passed to this routine.
 */
void *acp_gmm_main(void *args) {
	sdebug("acp_gmm module active..");

	// total memory of page table
	struct page_t tmp;
	unsigned int page_overhead = sizeof(tmp);

	//init the gmm state
	init_acp_gmm_state();

	//size of memory (RAM) buffer that we can use
	gmm_module_state.max_mem_limit = (acp_config.max_buff_size * 1024 * 1024); // in bytes
	gmm_module_state.max_swap_size = 2 * gmm_module_state.max_mem_limit;
	/*
	 * TODO: For the time being we are fixing the swap size to be twice of
	 * max_mem_limit. so max. no of pages that we can have is
	 * max_pages = pages in 2*max_mem_limit
	 */
	//max pages limit: no. of pages in (max_mem_limit + max pages that can fit in swap)
	gmm_module_state.max_page_limit = (acp_config.max_buff_size * 1024) / 4;
	gmm_module_state.max_swap_pages = 2 * gmm_module_state.max_page_limit;

	//when do we start swapping
	gmm_module_state.swappiness_threshold_in_pages =
			(gmm_module_state.max_page_limit)
					* ((((float) acp_config.swappiness)) / 100); //swappiness is %age

	var_debug("acp_gmm: max_mem(B): [%lu] max_pages: [%u] st_pages: [%u]",
			gmm_module_state.max_mem_limit, gmm_module_state.max_page_limit, gmm_module_state.swappiness_threshold_in_pages);
	//lets limit values for now
	gmm_module_state.max_page_limit = 100;
	var_debug("stats_ref_rate: %d log: %s",
			acp_config.stats_refresh_rate, acp_config.log_filename);
	/**
	 * before starting to allocate memory start helper threads.
	 * swap_manager_thread:
	 * 		check if no. of allocated pages has crossed the swappiness_threshold.
	 * 		if yes then start swapping pages.
	 *
	 */
//	while(1){
//
//	}
	//start threads
	/*
	 * some rules that all threads must do.
	 * 1. Run in a while loop but make use of sleeps. Dont go on busy waiting.
	 * 		Waiting on locks is ok but see if we can have timed waits.
	 * 2. All threads must look if shutdown_in_progress is set to true. This must be
	 * 		the first code in the while loop.
	 * 3. The moment
	 */
	if ((pthread_create(&gmm_module_state.gmm_mem_manager, NULL,
			gmm_mem_manager, NULL )) != 0) {
		serror("pthread_create: Failed to create thread: gmm_mem_manager");
		pthread_exit(NULL );
	}
	if ((pthread_create(&gmm_module_state.gmm_swap_manager, NULL,
			gmm_swap_manager, NULL )) != 0) {
		serror("pthread_create: Failed to create thread: gmm_swap_manager");
		pthread_exit(NULL );
	}
	if ((pthread_create(&gmm_module_state.gmm_mem_stats_collector, NULL,
			gmm_mem_stats_collector, NULL )) != 0) {
		serror(
				"pthread_create: Failed to create thread: gmm_mem_stats_collector");
		pthread_exit(NULL );
	}
	if ((pthread_create(&gmm_module_state.gmm_swap_stats_collector, NULL,
			gmm_swap_stats_collector, NULL )) != 0) {
		serror(
				"pthread_create: Failed to create thread: gmm_swap_stats_collector");
		pthread_exit(NULL );
	}

	/*
	 * we beleive that probably because we caught a signal or because shutdown
	 * signal was raised by parent thread, our child threads have finished their
	 * work and are ready to be collected. We will collect in the order we started
	 * them to ensure any synchronization of cleanup events that might be needed.
	 */
	if (pthread_join(gmm_module_state.gmm_mem_manager, NULL ) != 0) {
		fprintf(stderr, "\nError in collecting thread: gmm_mem_manager");
		pthread_exit(NULL );
	}
	if (pthread_join(gmm_module_state.gmm_swap_manager, NULL ) != 0) {
		fprintf(stderr, "\nError in collecting thread: gmm_swap_manager");
		pthread_exit(NULL );
	}
	if (pthread_join(gmm_module_state.gmm_mem_stats_collector, NULL ) != 0) {
		fprintf(stderr,
				"\nError in collecting thread: gmm_mem_stats_collector");
		pthread_exit(NULL );
	}
	if (pthread_join(gmm_module_state.gmm_swap_stats_collector, NULL ) != 0) {
		fprintf(stderr,
				"\nError in collecting thread: gmm_swap_stats_collector");
		pthread_exit(NULL );
	}
	//we are done
	pthread_exit(NULL );
}
/**
 * @brief The work area of gmm_memory manager thread.
 * @param args Threads args
 */
static void *gmm_mem_manager(void *args) {
	int i = 0;
	while (1) {
		if (acp_state.shutdown_in_progress == true) {
			//time to clean up
			break;
		}
		var_debug("gmm_mem_manager: %d", i);
		i++;
		sleep(1);
	}
	sdebug("gmm_mem_manager: shutting down");
	pthread_exit(NULL );
}
/**
 * @brief The work area of gmm_swap manager thread.
 * @param args Threads args
 */
static void *gmm_swap_manager(void *args) {
	int i = 0;
	while (1) {
		if (acp_state.shutdown_in_progress == true) {
			//time to clean up
			break;
		}
		var_debug("gmm_swap_manager: %d", i);
		i++;
		sleep(1);
	}
	sdebug("gmm_swap_manager: shutting down");
	pthread_exit(NULL );
}
/**
 * @brief The work area of gmm_memory stats collector thread.
 * @param args Threads args
 */
static void *gmm_mem_stats_collector(void *args) {
	int i = 0;
	while (1) {
		if (acp_state.shutdown_in_progress == true) {
			//time to clean up
			break;
		}
		var_debug("gmm_mem_stats_manager: %d", i);
		i++;
		sleep(1);
	}
	sdebug("gmm_mem_stats_collector: shutting down");
	pthread_exit(NULL );
}
/**
 * @brief The work area of gmm_swap stats collector thread.
 * @param args Threads args
 */

static void *gmm_swap_stats_collector(void *args) {
	/*
	 * for measuring intervals in accuracy of nanoseconds use
	 * struct timespec ts1,ts2;
	 * if (clock_gettime(CLOCK_REALTIME,&ts1) ==-1){
	 * 	//thats an error
	 * }
	 * do something
	 * .. get ts2 value as bove
	 * print (%ld,(ts2.tv_nsec-t1.tv_nsec))
	 */
	int i = 0;
	while (1) {
		if (acp_state.shutdown_in_progress == true) {
			//time to clean up
			break;
		}
		var_debug("gmm_swap_stats_collector: %d", i);
		i++;
		sleep(1);
	}
	sdebug("gmm_swap_stats_collector: shutting down");
	pthread_exit(NULL );
}

/**
 * @file acp_gmm_utils.c
 * @brief This file code to implement functionality for acp's GMM module.
 *
 * Warning: no routine from one thread will directly access a routine in
 * 		another thread.
 * since many routines from gmm_mem and gmm_swap will be using structures swap_table
 * and gmm_mem_page_table. they can be made global structures (accessible across this file)
 * however any attempt to access them should be through mutexes only.
 *
 */
#include "acp_gmm_utils.h"
static void *gmm_swap_stats_collector(void *args);
static void *gmm_mem_stats_collector(void *args);
static void *gmm_swap_manager(void *args);
static void *gmm_mem_manager(void *args);
static int create_swap_file(const char *filename,unsigned int max_pages);

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
//	gmm_module_state.max_page_limit = 100;
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
	struct swap_page_table_t *swap_table =NULL;
	//create the swap file
	if (create_swap_file(GMM_SWAP_FILE,gmm_module_state.max_swap_pages) != ACP_OK){
		//any cleanup if needed
		pthread_exit(NULL);
	}
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
	//remove swap file
	//unlink(GMM_SWAP_FILE);
	//delete swap table
	cleanup_swap_table();
	pthread_exit(NULL );
}
static int create_swap_file(const char *filename,unsigned int max_pages){
	struct swap_file_record_t file_record;
	int i=0,fd=-1;
	if ((fd = open(filename,O_CREAT|O_WRONLY)) == -1){
		var_error("Failed to open file: %s",filename);
		return ACP_ERR_FILE_IO;
	}
	//create an empty file_record
	file_record.page_id = 0;
	file_record.valid_record = false;

	for(i=0;i<max_pages;i++){
		if ((write(fd,&file_record,sizeof(file_record))) != sizeof(file_record)){
			var_error("Could not write bytes into %s .",filename);
			return ACP_ERR_FILE_IO;
		}
	}
	//do a buffer flush
	fdatasync(fd);
	close(fd);
	return ACP_OK;
}
static int write_page_to_swap(unsigned int page_id){
	/*
	 * 1. since we have page_id of the page that needs to be swapped out. Find
	 * 	the lookup_page_in_page_table() of gmm_mem module and retrieve the entire
	 * 	struct page_table_t *page_node .
	 *
	 * 3. once we get it signal to gmm_mem module through shared message variables
	 * 		that it can clear that page.
	 *
	 * 4. create an entry in swap_page_table
	 * 2. write page info to a swap_record in swap_file
	 */
}
static int load_page_from_swap(struct page_table *page,unsigned int page_id){
	/*
	 * 1. retrieve record index from swap_table by querying page_id in it.
	 * 2. Using this record_index go to the position where this page is located
	 * 		and load it in a struct swap_record_t structure.
	 * 3. Mark this swap record as invalid.
	 */
}
static int lookup_page_in_swap_table(unsigned int page_id,unsigned int *rreord_index){
	/*
	 * 1. find the record_id of queried page_id.
	 * 2. if not found set the record_id to 0 and return an error
	 */
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
//static int gmm_swap_cleanup(){
//
//}
//static int gmm_mem_cleanup(){
//
//}

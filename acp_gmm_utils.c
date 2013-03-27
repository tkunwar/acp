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
 * Generic routines will be given parameters however in routine where global
 * structures will be accessed, there should be no need to pass them.
 *
 * when we access the page_tables, when should we acquire locks ? For deleting
 * a node, yes we should . But do we need a lock even when we are traversing for it.
 * for e.g. searching an element in it. This might be needed since when a thread
 * might be adding a new node. ?? (For time being we will acquire lock only
 * when deleting a node from a list.)
 */
#include "acp_gmm_utils.h"
static void *gmm_swap_stats_collector(void *args);
static void *gmm_mem_stats_collector(void *args);
static void *gmm_swap_manager(void *args);
static void *gmm_mem_manager(void *args);
static int create_swap_file(const char *filename, unsigned int max_pages);
static void update_label(struct acp_global_labels *label, char *new_content);
static void print_page_table(struct page_table *head_ele);
static int page_memory_allocator(struct page_table **head_ele,
		struct page_table **last_ele, unsigned int *page_id_counter,
		unsigned int *n_pages);
static void free_page_table(struct page_table *head_ele);
static bool is_swapping_needed();
static struct page_table* find_oldest_page_in_mem_page_table(
		struct page_table *head);
static int write_page_to_swap(struct swap_file_record_t *swap_record,
		unsigned int *record_index);
static int insert_page_in_swap_page_table(unsigned int page_id,
		unsigned int record_index);
static void page_data_filler(unsigned char *page_data);
static int remove_page_from_page_table(struct page_table **node);
static void free_swap_page_table(struct swap_page_table_t *head_ele);

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

	//initalize mutexes
	if (pthread_mutex_init(&gmm_mutexes.gmm_mem_page_table_mutex, NULL ) != 0) {
		serror("Error in initializing mutex: gmm_mem_page_table");
	}
	if (pthread_mutex_init(&gmm_mutexes.gmm_mem_stats_mutex, NULL ) != 0) {
		serror("Error in initializing mutex: gmm_mem_stats_table");
	}
	if (pthread_mutex_init(&gmm_mutexes.gmm_swap_stats_mutex, NULL ) != 0) {
		serror("Error in initializing mutex: gmm_mem_stats_table");
	}
	if (pthread_mutex_init(&gmm_mutexes.gmm_cdk_screen_mutex, NULL ) != 0) {
		serror("Error in initializing mutex: gmm_cdks");
	}
	gmm_module_state.gmm_page_id_counter = 1;
	gmm_module_state.swap_table = NULL;
	gmm_module_state.swap_table_last_ele = NULL;
	gmm_module_state.used_swap_space = 0;
	gmm_module_state.page_out_timelag = 0;
}
/**
 * @brief the main routine for acp_gmm module.
 * @param args Arguments passed to this routine.
 */
void *acp_gmm_main(void *args) {
	sdebug("acp_gmm module active..");
	char msg[256];

	// total memory of page table
//	struct page_t tmp;
//	unsigned int page_overhead = sizeof(tmp);

//init the gmm state
	init_acp_gmm_state();

	//size of memory (RAM) buffer that we can use
	gmm_module_state.max_mem_limit = (acp_config.max_buff_size * 1024 * 1024); // in bytes
	gmm_module_state.max_swap_size = 2 * gmm_module_state.max_mem_limit;
	snprintf(msg, 256, "%12lu", gmm_module_state.max_swap_size);
	update_label(&acp_state.agl_gmm_swap_max_size, msg);
	/*
	 * TODO: For the time being we are fixing the swap size to be twice of
	 * max_mem_limit. so max. no of pages that we can have is
	 * max_pages = pages in 2*max_mem_limit
	 */
	//max pages limit: no. of pages in (max_mem_limit + max pages that can fit in swap)
	gmm_module_state.max_page_limit = (gmm_module_state.max_mem_limit)
			/ (sizeof(struct page_table));
	gmm_module_state.max_swap_pages = gmm_module_state.max_swap_size
			/ (sizeof(struct swap_file_record_t));

	sdebug("For testing using diferent data set:");
	sdebug("set max_page_limit to 500");
	gmm_module_state.max_page_limit = 500;
	//when do we start swapping
	gmm_module_state.swappiness_threshold_in_pages =
			(gmm_module_state.max_page_limit)
					* ((((float) acp_config.swappiness)) / 100); //swappiness is %age

	var_debug("acp_gmm: max_mem(B): [%lu] max_pages: [%u] st_pages: [%u]",
			gmm_module_state.max_mem_limit, gmm_module_state.max_page_limit,
			gmm_module_state.swappiness_threshold_in_pages);
	var_debug("acp_gmm: max_swap_size: [%lu] max_swap_pages: [%u]",
			gmm_module_state.max_swap_size, gmm_module_state.max_swap_pages);
	//lets limit values for now
//	gmm_module_state.max_page_limit = 100;
	var_debug("stats_ref_rate: %d log: %s", acp_config.stats_refresh_rate,
			acp_config.log_filename);


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
//	int i = 0;
	unsigned int n_pages = 1;
	while (1) {
		if (acp_state.shutdown_in_progress == true) {
			//time to clean up
			break;
		}
		//fir testing break out when we have already allocated 200 pages
		// that should be enough for us
		if (gmm_module_state.total_pages >= gmm_module_state.max_page_limit) {
			sleep(1);
			continue;
//			break;
		}
//		if (gmm_module_state.pages_active >= 100) {
//			sleep(1);
//			continue;
//		}
		/*
		 * if somehow pages could nt be swapped out and we have used our
		 * max buff limit fully. then we sleep so that swap routine gets some time
		 * to move pages from NCM to swap. and then we continue up. That is
		 * until there is more space available we will not allocate new pages.
		 */
		if (gmm_module_state.pages_active >= gmm_module_state.max_page_limit) {
			sleep(1);
			continue;
		}
		//acquire page_table lock
		pthread_mutex_lock(&gmm_mutexes.gmm_mem_page_table_mutex);
		if (page_memory_allocator(&gmm_module_state.page_table,
				&gmm_module_state.page_table_last_ele,
				&gmm_module_state.gmm_page_id_counter, &n_pages) != ACP_OK) {
			serror("Problem in page allocation");
			break;
		}
		pthread_mutex_unlock(&gmm_mutexes.gmm_mem_page_table_mutex);

		//now update stats
		pthread_mutex_lock(&gmm_mutexes.gmm_mem_stats_mutex);
		gmm_module_state.total_memory = gmm_module_state.total_memory
				+ n_pages * sizeof(struct page_table);
		gmm_module_state.total_pages = gmm_module_state.total_pages + n_pages;
		gmm_module_state.pages_active = gmm_module_state.pages_active + n_pages;
		pthread_mutex_unlock(&gmm_mutexes.gmm_mem_stats_mutex);

//		var_debug("Allocated %d pages", n_pages);
		usleep(100000); // sleep in microseconds
	}
//	print_page_table(gmm_module_state.page_table);

	sdebug("gmm_mem_manager: shutting down");
	sdebug("Freeing page_table");
	free_page_table(gmm_module_state.page_table);
	pthread_exit(NULL );
}
/**
 * @brief The work area of gmm_swap manager thread.
 * @param args Threads args
 */
static void *gmm_swap_manager(void *args) {
	struct swap_file_record_t record;
	struct page_table *oldest_page = NULL;
	unsigned long int t1, t2;
	unsigned int swap_record_index = 0;
	//create the swap file
	if (create_swap_file(GMM_SWAP_FILE,
			gmm_module_state.max_swap_pages) != ACP_OK) {
		//any cleanup if needed
		pthread_exit(NULL );
	}
	var_debug("Created swap file: %s", GMM_SWAP_FILE);
	while (1) {
		if (acp_state.shutdown_in_progress == true) {
			//time to clean up
			break;
		}
		/*
		 * 1. check if swapping needed.
		 * 2. if not then sleep
		 * 3. if yes then find oldest page . This routine will return the address
		 * 		of oldest page.
		 * 4. write this page to swap file.
		 * 5. Make an entry for this page to swap_page_table
		 * 6. Send signal to a routine which will clear this page from main memory.
		 *
		 */
		// 1. check is swapping is needed
		if (is_swapping_needed() == true) {
			// 2. find oldest page from the page_table
			oldest_page = find_oldest_page_in_mem_page_table(
					gmm_module_state.page_table);
			if (oldest_page == NULL ) {
				//something is wrong
				sdebug("gmm_swap_manager: swapping needed but could not find a page to swap.");
				sleep(1);
				continue;
			}
			//3. copy stats from oldest page in mem_page_table to swap_record.
			record.page_id = oldest_page->page.page_id;
			record.valid_record = true;
			if (memcpy(record.record_data, oldest_page->page.page_data,
					ACP_PAGE_SIZE) == NULL ) {
				var_error("Problem in copying page [%u] from mem_page_table",
						oldest_page->page.page_id);
				//sleep for a while and then continue up
				sleep(1);
				continue;
			}
//			var_debug("gmm_swap_manager: record_index: %u", swap_record_index);
			// 4. write this page to swap_file
			t1 = t2 = 0;
			t1 = gettime_in_nsecs();
			if (write_page_to_swap(&record, &swap_record_index) != ACP_OK) {
				var_error("Failed to write page [%u] to swap.",
						oldest_page->page.page_id);
				//there has been an error in writing this page so we sleep
				// for a while so as not to be aggressive
				sleep(1);
				continue;
			}
//			sdebug("gmm_swap_manager: updating stats");
			t2 = gettime_in_nsecs();
			pthread_mutex_lock(&gmm_mutexes.gmm_swap_stats_mutex);
			gmm_module_state.page_out_timelag =
					gmm_module_state.page_out_timelag + (t2 - t1);

			gmm_module_state.used_swap_space = gmm_module_state.used_swap_space
					+ sizeof(struct swap_file_record_t);
//			var_debug("pot: %lu uss: %lu", gmm_module_state.page_out_timelag,
//					gmm_module_state.used_swap_space);
			pthread_mutex_unlock(&gmm_mutexes.gmm_swap_stats_mutex);

//			var_debug("Adding page [%u] to swap_table",
//					oldest_page->page.page_id);
			//5. make an entry in swap_page_table
			if (insert_page_in_swap_page_table(oldest_page->page.page_id,
					swap_record_index) != ACP_OK) {
				var_error("Failed to insert page [%u] in swap_page_table",
						oldest_page->page.page_id);
				sleep(1);
				continue;
			}
//			sdebug("Removing page from page_table");
			//6. remove this page from page_table
			if (remove_page_from_page_table(&oldest_page) != ACP_OK) {
				if (gmm_module_state.page_table == NULL){
					sdebug("Probably gmm_mem_manager has already exited !");
				}else{
					var_error("Failed to delete page [%u] from page_table",
											oldest_page->page.page_id);
				}

				sleep(1);
			}
//			sdebug("All done..");
		}else{
			// if swapping not needed sleep for 1 second and then check if swapping
			// is needed again
			usleep(1000);
		}
	}

	sdebug("gmm_swap_manager: shutting down");
	//remove swap file
	sdebug("Removing gmm_swap file..");
	unlink(GMM_SWAP_FILE);
	//delete swap table
	sdebug("Freeing gmm_swap_table..");
	free_swap_page_table(gmm_module_state.swap_table);
	pthread_exit(NULL );
}
/**
 * @brief Checks whether pages need to be swapped from main memory to disk.
 * @return True/false depending upon whether swapping is needed or not.
 */
static bool is_swapping_needed() {
	bool swapping_needed = false;
	pthread_mutex_lock(&gmm_mutexes.gmm_mem_stats_mutex);
	if (gmm_module_state.pages_active
			>= gmm_module_state.swappiness_threshold_in_pages) {
		swapping_needed = true;
	}
//	if (gmm_module_state.pages_active >= 70) {
//		swapping_needed = true;
//	}
	pthread_mutex_unlock(&gmm_mutexes.gmm_mem_stats_mutex);
	return swapping_needed;
}
static int create_swap_file(const char *filename, unsigned int max_pages) {
	struct swap_file_record_t file_record;
	int i = 0, fd = -1;
	if ((fd = open(filename, O_CREAT | O_WRONLY, S_IRWXU)) == -1) {
		var_error("Failed to open file: %s", filename);
		return ACP_ERR_FILE_IO;
	}
	//create an empty file_record
	file_record.page_id = 0;
	file_record.valid_record = false;

	for (i = 0; i < max_pages; i++) {
		if ((write(fd, &file_record, sizeof(file_record)))
				!= sizeof(file_record)) {
			var_error("Could not write bytes into %s .", filename);
			return ACP_ERR_FILE_IO;
		}
	}
	//do a buffer flush
	fdatasync(fd);
	close(fd);
	return ACP_OK;
}
/**
 * @brief Returns pointer to the oldest page in mem_page_table.
 *
 * Since while allocating new pages, we allocate a new page and add it to end
 * of list. So oldest page in case will be the one which is present at the head
 * of list.
 *
 * @return Pointer to first element in mem_page_table
 */
static struct page_table* find_oldest_page_in_mem_page_table(
		struct page_table *head) {
	if (head) {
		return (head);
	}
	return NULL ;
}
//static int remove_page_from_mem_page_table(struct page_table **page_node){
//	return ACP_OK;
//}
static int write_page_to_swap(struct swap_file_record_t *swap_record,
		unsigned int *record_index) {
	/*
	 * 1. we will write record to the first empty location that we find.
	 * 2. Finding an empty location:
	 * 		a) we start from looking from record_index say i, and find the next
	 * 			free record.
	 * 		b) this record index is updated in the process.
	 * 		c) once record_index goes beyond max_swap_pages then we can start from
	 * 			first page to ensure that. it can simply be done by setting
	 * 			record_index = record_index % gmm_max_swap_pages
	 */
	int fd = -1;
	struct swap_file_record_t tmp_record;
	if ((fd = open(GMM_SWAP_FILE, O_RDWR)) == -1) {
		var_error("Failed to open file for I/O: %s [%d]", GMM_SWAP_FILE, errno);
		return ACP_ERR_FILE_IO;
	}

//	*record_index = *record_index % gmm_module_state.max_swap_pages;
	while (*record_index < gmm_module_state.max_swap_pages) {
		if ((lseek(fd, (*record_index) * sizeof(struct swap_file_record_t),
							SEEK_SET)) == -1) {
						var_error("Failed to seek write pointer in %s", GMM_SWAP_FILE);
						close(fd);
						return ACP_ERR_FILE_IO;
					}

		if ((read(fd, (struct swap_file_record_t*) &tmp_record,
				sizeof(struct swap_file_record_t)))
				!= sizeof(struct swap_file_record_t)) {
			var_error("Failed to read %lu bytes from file: %s",
					sizeof(struct swap_file_record_t), GMM_SWAP_FILE);
			close(fd);
			return ACP_ERR_FILE_IO;
		}
		if (tmp_record.valid_record == false) {
//			var_debug("Will write at index: %u", *record_index);
			//ensure write pointer is at correct position
			if ((lseek(fd, (*record_index) * sizeof(struct swap_file_record_t),
					SEEK_SET)) == -1) {
				var_error("Failed to seek write pointer in %s", GMM_SWAP_FILE);
				close(fd);
				return ACP_ERR_FILE_IO;
			}
			if ((write(fd, swap_record, sizeof(struct swap_file_record_t)))
					!= sizeof(struct swap_file_record_t)) {
				var_error("Could not write bytes into %s .", GMM_SWAP_FILE);
				close(fd);
				return ACP_ERR_FILE_IO;
			}
			*record_index = *record_index + 1;
			break;
		}
//		*record_index = *record_index + 1;
	}

	fdatasync(fd);
	close(fd);
	return ACP_OK;
}
static int load_page_from_swap(struct page_table *page, unsigned int page_id) {
	/*
	 * 1. retrieve record index from swap_table by querying page_id in it.
	 * 2. Using this record_index go to the position where this page is located
	 * 		and load it in a struct swap_record_t structure.
	 * 3. Mark this swap record as invalid.
	 */
	return ACP_OK;
}
/**
 * @brief Check if a given page_id is present in swap_table.
 * @param page_id The page_id whose existence is to be checked
 * @param record_index Where this page is actually stored in the swap_file.
 * @return ACP_OK if found or an error code.
 */
static int lookup_page_index_in_swap_file(unsigned int page_id,
		unsigned int *record_index) {
	/*
	 * 1. find the record_id of queried page_id.
	 * 2. if not found set the record_id to 0 and return an error
	 */
	//default value
	*record_index = 0;
	struct swap_page_table_t *node = NULL;
	if (!gmm_module_state.swap_table) {
		*record_index = 0;
		return ACP_ERR_NO_SUCH_ELEMENT;
	}
	for (node = gmm_module_state.swap_table; node != NULL ; node = node->next) {
		if (node->page_id == page_id) {
			*record_index = node->swap_record_index;
			break;
		}
	}
	return ACP_OK;
}
static int remove_page_from_page_table(struct page_table **node) {
	/*
	 * Though not an efficient way but since we know that oldest page
	 * is the first one. So removing a page from page table always means
	 * removing the first element.
	 */
	struct page_table *to_be_deleted = *node;
	if (gmm_module_state.page_table == NULL){
		sdebug("mem_page_table is empty !!");
		return ACP_ERR_MEM_FAULT;
	}
//	var_debug("Removing [%u] next [%u]",to_be_deleted->page.page_id,((*node)->next)->page.page_id);
//	var_debug("Trying to remove page %u", (*node)->page.page_id);
	if (!*node) {
		serror("Trying to delete a NULL page from page_table");
		return ACP_ERR_MEM_FAULT;
	}

	pthread_mutex_lock(&gmm_mutexes.gmm_mem_page_table_mutex);
	gmm_module_state.page_table = (*node)->next;
	free(to_be_deleted);
	pthread_mutex_unlock(&gmm_mutexes.gmm_mem_page_table_mutex);
	//we just removed a page from page_table , decrement pages active count

	pthread_mutex_lock(&gmm_mutexes.gmm_mem_stats_mutex);
	gmm_module_state.pages_active = gmm_module_state.pages_active - 1;
	pthread_mutex_unlock(&gmm_mutexes.gmm_mem_stats_mutex);
	return ACP_OK;
}
/**
 * @brief Adds an entry for page_id,record_index pair as a node in swap page_table.
 * @param page_id The page_id
 * @param record_index Record index where this page has been stored in the swap file.
 * @return ACP_OK if all fine else an error
 */
static int insert_page_in_swap_page_table(unsigned int page_id,
		unsigned int record_index) {
	struct swap_page_table_t *node = NULL;
	node = (struct swap_page_table_t*) malloc(sizeof(struct swap_page_table_t));
	if (!node) {
		serror("malloc: failed ");
		return ACP_ERR_NO_MEM;
	}
	node->page_id = page_id;
	node->swap_record_index = record_index;
	node->next = NULL;

	if (!gmm_module_state.swap_table && !gmm_module_state.swap_table_last_ele) {
		//initial state
		pthread_mutex_lock(&gmm_mutexes.gmm_swap_table_mutex);
		gmm_module_state.swap_table = gmm_module_state.swap_table_last_ele =
				node;
		pthread_mutex_unlock(&gmm_mutexes.gmm_swap_table_mutex);
	} else {
		pthread_mutex_lock(&gmm_mutexes.gmm_swap_table_mutex);
		node->next = NULL;
		gmm_module_state.swap_table_last_ele->next = node;
		gmm_module_state.swap_table_last_ele = node;
		gmm_module_state.swap_table_last_ele->next = NULL;
		pthread_mutex_unlock(&gmm_mutexes.gmm_swap_table_mutex);
	}
	return ACP_OK;
}
static void free_swap_page_table(struct swap_page_table_t *head_ele) {
	struct swap_page_table_t *prev_node = head_ele, *cur_node = head_ele;
	do {
		prev_node = cur_node;
		if (!cur_node){
			break;
		}
		cur_node = cur_node->next;
//		var_debug("deleting %u :",prev_node->page.page_id);
		free(prev_node);
	} while (cur_node);
}
/**
 * @brief The work area of gmm_memory stats collector thread.
 * @param args Threads args
 */
static void *gmm_mem_stats_collector(void *args) {
	char msg[256];
	while (1) {
		if (acp_state.shutdown_in_progress == true) {
			//time to clean up
			break;
		}
		//acquire lock before updating stats
		pthread_mutex_lock(&gmm_mutexes.gmm_mem_stats_mutex);

		snprintf(msg, 256, "</32>%12lu<!32>", gmm_module_state.total_memory);
		update_label(&acp_state.agl_gmm_total_memory, msg);

		snprintf(msg, 256, "</32>%12u<!32>", gmm_module_state.total_pages);
		update_label(&acp_state.agl_gmm_total_pages, msg);

		snprintf(msg, 256, "</32>%12u<!32>", gmm_module_state.pages_active);
		update_label(&acp_state.agl_gmm_pages_active, msg);
		//release lock
		pthread_mutex_unlock(&gmm_mutexes.gmm_mem_stats_mutex);
		sleep(acp_config.stats_refresh_rate);
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
	char msg[256];
	while (1) {
		if (acp_state.shutdown_in_progress == true) {
			//time to clean up
			break;
		}
		pthread_mutex_lock(&gmm_mutexes.gmm_swap_stats_mutex);

		snprintf(msg, 256, "</24>%12lu<!24>", gmm_module_state.used_swap_space);
		update_label(&acp_state.agl_gmm_swap_used_space, msg);

		snprintf(msg, 256, "</24>%12lu<!24>", gmm_module_state.page_out_timelag);
		update_label(&acp_state.agl_gmm_swap_pageout_timelag, msg);

		pthread_mutex_unlock(&gmm_mutexes.gmm_swap_stats_mutex);
		sleep(acp_config.stats_refresh_rate);
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
/**
 * @brief Allocates memory for n pages and adds them to the passed list.
 * @param head_ele The first element of type page_table which stores pages.
 * @param last_ele Pointer to the last element of the list.
 * @param page_id_counter A pointer to the counter which will help in generating page_id for
 * 			newly allocated pages.
 * @param n_pages Denotes how many pages to be allocated. Note that if demand for
 * 		memory to be allocated can not be met (due to out of memory or max. memory
 * 		limit is reached ) n_pages will be modified to denote memory for how
 * 		many pages were actually allocated.
 * @return ACP_OK if succes else an error code.
 */
static int page_memory_allocator(struct page_table **head_ele,
		struct page_table **last_ele, unsigned int *page_id_counter,
		unsigned int *n_pages) {
	struct page_table *node = NULL;
	int i, pages_allocated = 0;
	if (n_pages <= 0)
		return ACP_OK;
	if (!*head_ele && !*last_ele) {
		//initial state
		node = (struct page_table*) malloc(sizeof(struct page_table));
		if (!node) {
			serror("malloc: failed ");
			return ACP_ERR_NO_MEM;
		}
//		sdebug("Allocated mem to head node");
		node->page.LAT = gettime_in_nsecs();
		node->page.dirty = false;
		node->page.page_cur_loc = SL_NCM;
		node->page.page_id = (*page_id_counter)++;
		page_data_filler(node->page.page_data);

		node->next = NULL;
		*last_ele = *head_ele = node;
		pages_allocated++;
	}
	for (i = 0; i < (*n_pages - pages_allocated); i++) {
		node = NULL;
		node = (struct page_table*) malloc(sizeof(struct page_table));
		if (!node) {
			serror("malloc: failed ");
			*n_pages = (*n_pages - pages_allocated);
			return ACP_ERR_NO_MEM;
		}
		node->page.LAT = gettime_in_nsecs();
		node->page.dirty = false;
		node->page.page_cur_loc = SL_NCM;
		node->page.page_id = (*page_id_counter)++;
		page_data_filler(node->page.page_data);

//		var_debug("Allocating to node %d",i+1);
		node->next = NULL;
		(*last_ele)->next = node;
		*last_ele = node;
		(*last_ele)->next = NULL;
	}

	return ACP_OK;
}
static void page_data_filler(unsigned char *page_data) {
	int i;
	for (i = 0; i < ACP_PAGE_SIZE; i++) {
		page_data[i] = 'a';
	}
}
static void free_page_table(struct page_table *head_ele) {
	struct page_table *prev_node = head_ele, *cur_node = head_ele;
	do {
		prev_node = cur_node;
		cur_node = cur_node->next;
//		var_debug("deleting %u :",prev_node->page.page_id);
		free(prev_node);
	} while (cur_node);
	//we have emptied page_table so set the pointer to null.
	gmm_module_state.page_table = NULL;
}
static void print_page_table(struct page_table *head_ele) {
	struct page_table *node = head_ele;
	if (!head_ele) {
		serror("Got no first element");
		return;
	}
	for (; node != NULL ; node = node->next) {
		var_debug("page_id: %u", node->page.page_id);
	}
}
static void update_label(struct acp_global_labels *label, char new_content[]) {
	char *mesg[2];
	curs_set(0);

	mesg[0] = copyChar(new_content);
	pthread_mutex_lock(&gmm_mutexes.gmm_cdk_screen_mutex);
	setCDKLabel(label->lblptr, (CDK_CSTRING2) mesg, 1, false);

	drawCDKLabel(label->lblptr, false);
	pthread_mutex_unlock(&gmm_mutexes.gmm_cdk_screen_mutex);
}

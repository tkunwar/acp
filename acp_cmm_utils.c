/*
 * acp_cmm_utils.c
 *
 *  Created on: Feb 25, 2013
 *      Author: tej
 */

#include "acp_cmm_utils.h"
#include "minilzo.h"
static void *cmm_mem_manager(void *args);
static void *cmm_mem_stats_collector(void *args);
static void *cmm_cc_manager(void *args);
static void *cmm_cc_stats_collector(void *args);
static void free_page_table(struct page_table *head_ele);
static int page_memory_allocator(struct page_table **head_ele,
		struct page_table **last_ele, unsigned int *page_id_counter,
		unsigned int *n_pages);
//static int compress_page(const struct page_t *src_page,
//		struct compressed_page_t *out_page, unsigned int *out_len);
static void update_label(struct acp_global_labels *label, char new_content[]);
//static int decompress_page(struct compressed_page_t *cpage,
//		struct page_t *out_page);
static void page_data_filler(unsigned char *page_data);
static bool is_caching_needed();
static struct page_table* find_oldest_page_in_mem_page_table(
		struct page_table *head);
static int compress_page(const struct page_t *src_page,
		struct compressed_page_t **out_page);
static int decompress_page(const struct compressed_page_t *cpage,
		struct page_t *out_page);
static int remove_page_from_page_table(struct page_table **node);

static int store_page_in_cc(struct compressed_page_t *cpage);
static struct cell_t *allocate_new_cell();
static void save_page_to_cell(struct cell_t *target_cell,
		struct compressed_page_t *cpage);
static unsigned int find_next_pageid_from_cell(const unsigned char *cell_data,
		int *index);
static unsigned int find_next_pagelen_from_cell(const unsigned char *cell_data,
		int *index);
static struct cell_t * find_smallest_cell_from_celltable(_uint16 cpage_len);
static void free_cell_table(struct cell_t *head_ele);

/**
 * @brief Initialize cmm module state.
 */
static void init_acp_cmm_state() {
	//these two variables wont be modifed during program run
	cmm_module_state.max_mem_limit = 0;
	cmm_module_state.max_page_limit = 0;

	// these values will be modified during program
	cmm_module_state.pages_active = 0;
	cmm_module_state.total_memory = 0;
	cmm_module_state.total_pages = 0;
	cmm_module_state.page_table = NULL;
	cmm_module_state.page_table_last_ele = NULL;

	//initalize mutexes
	if (pthread_mutex_init(&cmm_mutexes.cmm_mem_page_table_mutex, NULL ) != 0) {
		serror("Error in initializing mutex: cmm_mem_page_table");
	}
	if (pthread_mutex_init(&cmm_mutexes.cmm_mem_stats_mutex, NULL ) != 0) {
		serror("Error in initializing mutex: cmm_mem_stats_table");
	}
	if (pthread_mutex_init(&cmm_mutexes.cmm_swap_stats_mutex, NULL ) != 0) {
		serror("Error in initializing mutex: cmm_mem_stats_table");
	}
	if (pthread_mutex_init(&cmm_mutexes.cmm_cdk_screen_mutex, NULL ) != 0) {
		serror("Error in initializing mutex: cmm_cdks");
	}
	if (pthread_mutex_init(&cmm_mutexes.cmm_cc_stats_mutex, NULL ) != 0) {
		serror("Error in initializing mutex: cmm_cc_stats_mutex");
	}
	if (pthread_mutex_init(&cmm_mutexes.cmm_cell_table_mutex, NULL ) != 0) {
		serror("Error in initializing mutex: cmm_cell_table_mutex");
	}
	cmm_module_state.cmm_page_id_counter = 1;
	cmm_module_state.swap_table = NULL;
	cmm_module_state.swap_table_last_ele = NULL;
	cmm_module_state.cc_current_size = 0;
	cmm_module_state.cc_page_out_timelag = 0;
	cmm_module_state.cc_max_caching_size = 0;
	cmm_module_state.cc_caching_threshold_in_pages = 0;
	cmm_module_state.cc_cell_id_counter = 1;
	cmm_module_state.cell_table = cmm_module_state.cell_table_last_ele = NULL;
	cmm_module_state.cc_cells_active = 0;

	cmm_module_state.cc_page_in_timelag = 0;
	cmm_module_state.cc_current_size = 0;
	cmm_module_state.cc_current_stored_pages = 0;
}
/**
 * @brief the main routine for acp_cmm module.
 * @param args Arguments passed to this routine.
 */
void *acp_cmm_main(void *args) {
	sdebug("acp_cmm_main module active");
	init_acp_cmm_state();
	cmm_module_state.max_mem_limit = (acp_config.max_buff_size * 1024 * 1024); // in bytes
	cmm_module_state.max_page_limit = (cmm_module_state.max_mem_limit)
			/ (sizeof(struct page_table));

	// this is when we should start caching pages. whether caching is needed or not
	// will be decided based on this parameter.
	cmm_module_state.cc_caching_threshold_in_pages =
			(cmm_module_state.max_page_limit)
					* ((((float) acp_config.swappiness)) / 100); //swappiness is %age

	/*
	 * max caching size is 100 - swappiness -5 . That means that if swappiness is
	 * 70. then we will use only 25 percent of max. memory for our purposes.
	 * 5 % is left for realtime-adjustments for e.g. caching might be slower but
	 * page allocation will be faster in this case. space for caching will be slower
	 * soon eonugh for cc engine to realize it and take action accordingly.
	 */
	cmm_module_state.cc_max_caching_size = (cmm_module_state.max_mem_limit)
			* ((((float) (95 - acp_config.swappiness))) / 100);

	cmm_module_state.cc_max_cell_count = cmm_module_state.cc_max_caching_size
			/ (sizeof(struct cell_t));
	var_debug("cmm_main: max_mem:[%lu] max_page_limit: [%u] ",
			cmm_module_state.max_mem_limit, cmm_module_state.max_page_limit);

	var_debug("cmm_main: caching_th: [%u] max_caching_size: [%lu]",
			cmm_module_state.cc_caching_threshold_in_pages,
			cmm_module_state.cc_max_caching_size);
	var_debug("cmm_main: max_cell_count: %u",
			cmm_module_state.cc_max_cell_count);

	//create threads
	if ((pthread_create(&cmm_module_state.cmm_mem_manager, NULL,
			cmm_mem_manager, NULL )) != 0) {
		serror("pthread_create: Failed to create thread: cmm_mem_manager");
		pthread_exit(NULL );
	}
	if ((pthread_create(&cmm_module_state.cmm_mem_stats_collector, NULL,
			cmm_mem_stats_collector, NULL )) != 0) {
		serror(
				"pthread_create: Failed to create thread: cmm_mem_stats_collector");
		pthread_exit(NULL );
	}
	if ((pthread_create(&cmm_module_state.cmm_cc_manager, NULL, cmm_cc_manager,
			NULL )) != 0) {
		serror("pthread_create: Failed to create thread: cmm_cc_manager");
		pthread_exit(NULL );
	}
	if ((pthread_create(&cmm_module_state.cmm_cc_stats_collector, NULL,
			cmm_cc_stats_collector, NULL )) != 0) {
		serror(
				"pthread_create: Failed to create thread: cmm_cc_stats_collector");
		pthread_exit(NULL );
	}

	//now collect threads
	if (pthread_join(cmm_module_state.cmm_mem_manager, NULL ) != 0) {
		fprintf(stderr, "\nError in collecting thread: cmm_mem_manager");
		pthread_exit(NULL );
	}
	if (pthread_join(cmm_module_state.cmm_mem_stats_collector, NULL ) != 0) {
		fprintf(stderr,
				"\nError in collecting thread: cmm_mem_stats_collector");
		pthread_exit(NULL );
	}
	if (pthread_join(cmm_module_state.cmm_cc_manager, NULL ) != 0) {
		fprintf(stderr, "\nError in collecting thread: cmm_cc_manager");
		pthread_exit(NULL );
	}
	if (pthread_join(cmm_module_state.cmm_cc_stats_collector, NULL ) != 0) {
		fprintf(stderr, "\nError in collecting thread: cmm_cc_stats_collector");
		pthread_exit(NULL );
	}
	//now exit
	pthread_exit(NULL );
}
/**
 * @brief Memory manager section of cmm module. Manages uncompressesd memory section.
 * @param args Arguments if any passed to memory manager.
 */
static void *cmm_mem_manager(void *args) {
	unsigned int n_pages = 1;
	while (1) {
		if (acp_state.shutdown_in_progress == true) {
			//time to clean up
			break;
		}
		//fir testing break out when we have already allocated 200 pages
		// that should be enough for us
		if (cmm_module_state.total_pages >= 200) {
			break;
		}
		// when we already have 100 pages active then slow down and let
		// cache manager to cache a few pages.
		if (cmm_module_state.pages_active >= 100) {
			//			break;
			sleep(1);
			continue;
		}
		/*
		 * if somehow pages could nt be swapped out and we have used our
		 * max buff limit fully. then we sleep so that swap routine gets some time
		 * to move pages from NCM to swap. and then we continue up. That is
		 * until there is more space available we will not allocate new pages.
		 * This situation is although very aggressive. And we should stop before :)
		 */
		if (cmm_module_state.pages_active >= cmm_module_state.max_page_limit) {
			sleep(1);
			continue;
		}
		//acquire page_table lock
		pthread_mutex_lock(&cmm_mutexes.cmm_mem_page_table_mutex);
		if (page_memory_allocator(&cmm_module_state.page_table,
				&cmm_module_state.page_table_last_ele,
				&cmm_module_state.cmm_page_id_counter, &n_pages) != ACP_OK) {
			serror("Problem in page allocation");
			break;
		}
		pthread_mutex_unlock(&cmm_mutexes.cmm_mem_page_table_mutex);

		//now update stats
		pthread_mutex_lock(&cmm_mutexes.cmm_mem_stats_mutex);
		cmm_module_state.total_memory = cmm_module_state.total_memory
				+ n_pages * sizeof(struct page_table);
		cmm_module_state.total_pages = cmm_module_state.total_pages + n_pages;
		cmm_module_state.pages_active = cmm_module_state.pages_active + n_pages;
		pthread_mutex_unlock(&cmm_mutexes.cmm_mem_stats_mutex);

		//		var_debug("Allocated %d pages", n_pages);
		usleep(100000); // sleep in microseconds
	}
	//	print_page_table(gmm_module_state.page_table);

	sdebug("cmm_mem_manager: Shutting down");
	sdebug("cmm_mem_manager: Freeing page_table");
	free_page_table(cmm_module_state.page_table);
	pthread_exit(NULL );

}
/**
 * @brief Stats collector thread of memory section of cmm module.
 * @param args Arguments passed to mem stats collector thread.
 */
static void *cmm_mem_stats_collector(void *args) {
	char msg[256];
	while (1) {
		if (acp_state.shutdown_in_progress == true) {
			//time to clean up
			break;
		}
		//acquire lock before updating stats
		pthread_mutex_lock(&cmm_mutexes.cmm_mem_stats_mutex);

		snprintf(msg, 256, "%12lu", cmm_module_state.total_memory);
		update_label(&acp_state.agl_cmm_ucm_total_memsize, msg);

		snprintf(msg, 256, "%12u", cmm_module_state.total_pages);
		update_label(&acp_state.agl_cmm_ucm_total_pages, msg);

		snprintf(msg, 256, "%12u", cmm_module_state.pages_active);
		update_label(&acp_state.agl_cmm_ucm_pages_active, msg);
		//release lock
		pthread_mutex_unlock(&cmm_mutexes.cmm_mem_stats_mutex);
		sleep(acp_config.stats_refresh_rate);
	}
	sdebug("cmm_mem_stats_collector: shutting down");
	pthread_exit(NULL );
}
/**
 * @brief Compressed cache manager thread.
 *
 * cc_manager forms the core of cmm module. Not only that it caches pages which
 * are swapped out, it also forms a transparent interface to underlying swapping
 * layer. In case, memory cache is full then pages from CC are swapped out.
 * This process however is fully transparent to memory manager. Memory manager only
 * needs to throw out pages, CC will handle the rest.
 * @param args Arguments passed to cc_manager.
 */
static void *cmm_cc_manager(void *args) {
	/*
	 * 1. check if caching needed.
	 * 2. if not then sleep
	 * 3. if yes then find oldest page . This routine will return the address
	 * 		of oldest page.
	 * 4. compress this page and cache it using following algorithm:
	 * 		4.1) Search for a cell with smallest "final free space"**( maintain
	 * 			 a table where cells with final free space in  ascending order
	 * 			 are listed) enough to accomodate this page.
	 *
	 * 		4.2) If a cell is found, make an entry for this page and cell in
	 * 			 the lookup_table (a table where cell and page relationship is
	 * 			  listed).
	 * 			  4.2.1) store this page in this cell and update relevant details
	 * 			  		 such as final available free space in available_free_space
	 * 			  		 table for this cell.
	 * 		4.3) If no such cell found then --
	 * 			4.3.1)  see if a new cell can be created. If yes then create a
	 * 					new cell and follows step 4.2.
	 * 			4.3.2)	If not possible to create a new cell then then swap out
	 * 					oldest page in CC to swap file. And after the LRU page
	 * 					has been swapped out to disk, follow step 4.
	 *
	 * 5. Send signal to a routine which will clear this page from main memory.
	 *
	 *	Note: The swapping layer is completely transparent to mem_manager. Swapping
	 *	if needed will be performed by cc module.
	 */
	struct page_table *oldest_page = NULL;
//	int i=0;
	struct compressed_page_t *page_in_compressed_form = NULL;
	unsigned long int t1, t2;
	while (1) {
		if (acp_state.shutdown_in_progress == true) {
			//time to clean up
			break;
		}
		// 1. check if  caching is needed
		if (is_caching_needed() == true) {
//			sdebug("cmm_cc_manager: Caching needed.");
			// 2. find oldest page to cache
			oldest_page = find_oldest_page_in_mem_page_table(
					cmm_module_state.page_table);
			if (oldest_page == NULL ) {
				//something is wrong
				sdebug(
						"cmm_cc_manager: caching needed but could not find a page to swap.");
				sleep(1);
				continue;
			}
//			var_debug("cmm_cc_manager: Will cache: %u ",oldest_page->page.page_id);
//			sleep(1);
			//3. lets compress oldest page.
			t1 = gettime_in_nsecs();
			if (compress_page(&oldest_page->page, &page_in_compressed_form)
					!= ACP_OK) {
				var_error("cmm_cc_manager: Failed to compressed page %u",
						oldest_page->page.page_id);
				//TODO: take proper action i.e. what should be done with this page ?
				// Since our oldest page is the first page in the list, even next time
				// we will get this page only. So for now we can signal mem_manager to cleanup
				// this page from page_table even though we could not compress it or best would be
				// simply swap it. In second case we will not loose any data.
				continue;
			}
			t2 = gettime_in_nsecs();
			//t2-t1 is what we took for page compression and will be considered
			// as pageout timelag for cc unit.
			pthread_mutex_lock(&cmm_mutexes.cmm_cc_stats_mutex);
			cmm_module_state.cc_page_out_timelag =
					cmm_module_state.cc_page_out_timelag + (t2 - t1);
			pthread_mutex_unlock(&cmm_mutexes.cmm_cc_stats_mutex);

			// since we are not maintaining any global page table other than
			// page_table used by mem_manager we will not store information about
			// location of this page. We are not doing this even in gmm module.
			// this means we can not have lookups or faster look up for a page.
			// All our processing is one way only.
			// now lets store this page in cache

			if (store_page_in_cc(page_in_compressed_form) != ACP_OK) {
				sdebug("Something went wrong");
				//something went wrong
				free(page_in_compressed_form);
				//since we could not cache this page, most probably because
				// cc was full and even after that swap was full. we will not
				// send signal to cmm_mem_manager to cleanup this page.
				// we simply go up. This situation is not good, probably we should
				// break out.
				var_error("cmm_cc_manager: Could not cache page: %u",
						oldest_page->page.page_id);
//				continue;
				// however once this happens other threads/routines in CC layer should
				//terminate too.
				break;
			}
			// operations in cc is transparent to this routine. Stats will be
			// updated where applicable.

			//send signal to clear this page from page_table
			if (remove_page_from_page_table(&oldest_page) != ACP_OK) {
				if (cmm_module_state.page_table == NULL ) {
					sdebug("Probably cmm_mem_manager has already exited !");
				} else {
					var_error("Failed to delete page [%u] from page_table",
							oldest_page->page.page_id);
				}
				sleep(1);
			}

			//clear page_in_compressed_form as its data will be copied to cell's
			// data field.
			free(page_in_compressed_form);
			usleep(1000);
		} else {
//			sdebug("cmm_cc_manager: caching not needed.");
			usleep(1000);
		}
	}
	sdebug("cmm_cc_manager: shutting down");
	//cleanup cache tables
	sdebug("cmm_cc_manager: cleanup cell cache table..");
	free_cell_table(cmm_module_state.cell_table);

	pthread_exit(NULL );

}
/**
 * @brief Check if we need to cache pages from page table in uncompressed memory.
 * 		This routine returns true when pages in page_table form swappiness_factor
 * 		%age of entire memory available.
 *
 * 	Returns true/false indicating whether pages will be cached or not.
 */
static bool is_caching_needed() {
	bool caching_needed = false;
	pthread_mutex_lock(&cmm_mutexes.cmm_cc_stats_mutex);
//	if (cmm_module_state.pages_active >= cmm_module_state.caching_threshold_in_pages){
//		caching_needed = true;
//	}
	if (cmm_module_state.pages_active >= 70) {
		caching_needed = true;
	}
	pthread_mutex_unlock(&cmm_mutexes.cmm_cc_stats_mutex);
	return caching_needed;
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
/**
 * @brief Stats collector thread routine for compressed cache.
 * @param args Arguments passed to this thread.
 */
static void *cmm_cc_stats_collector(void *args) {
	/*
	 * we will update current_cc_size,stored_pages,cc_cells_active and pageout_timelag
	 * all in one go.
	 */
	char msg[256];
	while (1) {
		if (acp_state.shutdown_in_progress == true) {
			//time to clean up
			break;
		}
		//acquire lock before updating stats
		pthread_mutex_lock(&cmm_mutexes.cmm_cc_stats_mutex);

		snprintf(msg, 256, "%12lu", cmm_module_state.cc_current_size);
		update_label(&acp_state.agl_cmm_cc_cur_memsize, msg);

		snprintf(msg, 256, "%12u", cmm_module_state.cc_current_stored_pages);
		update_label(&acp_state.agl_cmm_cc_stored_pages, msg);

		snprintf(msg, 256, "%12u", cmm_module_state.cc_cells_active);
		update_label(&acp_state.agl_cmm_cc_cells, msg);

		snprintf(msg, 256, "%12u", cmm_module_state.cc_page_out_timelag);
		update_label(&acp_state.agl_cmm_cc_pageout_timelag, msg);

		// we are updating pagein timelag
//		snprintf(msg, 256, "%12u", cmm_module_state.cc_page_in_timelag);
//		update_label(&acp_state.agl_cmm_cc_pagein_timelag, msg);

		//release lock
		pthread_mutex_unlock(&cmm_mutexes.cmm_cc_stats_mutex);
		sleep(acp_config.stats_refresh_rate);
	}
	sdebug("cmm_cc_stats_collector: shutting down");
	pthread_exit(NULL );
}
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
 * @return ACP_OK if success else an error code.
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
/**
 * @brief Fills up page_data portion of a newly allocated page with random
 * 		characters.
 * @param page_data The buffer of size 4096 which will be filled up.
 */
static void page_data_filler(unsigned char *page_data) {
	int i;
	int j = 48;
	for (i = 0; i < ACP_PAGE_SIZE; i++) {
		if (j == 122)
			j = 48;
		page_data[i] = (char) j;
		j++;
	}
}
/**
 * @brief Clear memory held by global page table in uncompressed memory section.
 * @param head_ele The pointer to the page_table
 */
static void free_page_table(struct page_table *head_ele) {
	struct page_table *prev_node = head_ele, *cur_node = head_ele;
	do {
		prev_node = cur_node;
		cur_node = cur_node->next;
		free(prev_node);
	} while (cur_node);
	//we have emptied page_table so set the pointer to null.
	cmm_module_state.page_table = NULL;
}

// /////////// Compression/Decompression routines (LZO) ////////////////

/* Work-memory needed for compression. Allocate memory in units
 * of 'lzo_align_t' (instead of 'char') to make sure it is properly aligned.
 */

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);

/**
 * @brief Compresses a page. And out_page points to the compressed page in memory.
 * 		The memory for out_page is allocated dynamically. Therefore, out_page
 * 		should be set to NULL.
 * @param src_page  The source page which will be compressed.
 * @param out_page The resultant page of compression.
 * @return Returns ACP_OK if everything was ok or an appropriate error code.
 */
static int compress_page(const struct page_t *src_page,
		struct compressed_page_t **out_page) {
	unsigned int in_len = 4096;
	int lib_err;
	unsigned int out_len = in_len + in_len / 16 + 64 + 3;
//	var_debug("out_len before compression: %u", out_len);
	unsigned char *tmp_out_buff = (unsigned char*) malloc(out_len);
	if (!tmp_out_buff) {
		serror("malloc: failed");
		return ACP_ERR_NO_MEM;
	}

	//allocate memory to compressed page
	*out_page = (struct compressed_page_t *) malloc(
			sizeof(struct compressed_page_t));
	if (!tmp_out_buff) {
		serror("malloc: failed");
		return ACP_ERR_NO_MEM;
	}

	/*
	 * Step 1: initialize the LZO library
	 */
	if (lzo_init() != LZO_E_OK) {
		serror("Error initializing lzo lib.")
		return ACP_ERR_LZO_INIT;
	}

	/*
	 * Step 3: compress from 'in' to 'out' with LZO1X-1
	 */
	lib_err = lzo1x_1_compress(src_page->page_data, in_len, tmp_out_buff,
			&out_len, wrkmem);
	if (lib_err == LZO_E_OK) {
//		var_debug("compressed [%u] %u bytes into %u bytes",
//				src_page->page_id,in_len, out_len);
	} else {
		/* this should NEVER happen */
		serror("Internal Compression error");
		return ACP_ERR_LZO_INTERNAL;
	}
	/* check for an incompressible block */
	if (out_len >= in_len) {
		// we might want to discard this field
		var_warn("Page %u contains incompressible data.", src_page->page_id);
	}

	/*
	 * now allocate actual out_len bytes of memory to out and copy the
	 * tmp_out_buff contents to it.
	 */
	(*out_page)->page_data = (unsigned char *) malloc(out_len);
	if (!out_page) {
		serror("malloc: failed");
		return ACP_ERR_NO_MEM;
	}

	memcpy((*out_page)->page_data, tmp_out_buff, out_len);
	(*out_page)->page_id = src_page->page_id;
	(*out_page)->page_len = out_len;
//	out_page->next = NULL;

	//now free tmp_out_buff
	free(tmp_out_buff);
	return ACP_OK;
}
/**
 * @brief Accepts a compressed_page_t type structure and decompresses it to get
 * 			original data.
 *
 * Noe that we donot have to provide the out_page len as we will store the decompressed
 * page into a 4096 bytes page. Also only page_id and page_data of out_page will be valid
 * as compressed_page does not other fields in. Since out_page is fixed it point to
 * a valid page_t variable.
 * @param cpgae The compressed page which has compresed data.
 * @param out_page Should be valid structure of type page_t which will store the decompressed result.
 * @return ACP_OK if everything was fine else an error code.
 */
static int decompress_page(const struct compressed_page_t *cpage,
		struct page_t *out_page) {
	if (lzo_init() != LZO_E_OK) {
		serror("Error initializing lzo lib.")
		return ACP_ERR_LZO_INIT;
	}
	unsigned int new_len = 0, lib_err;
	lib_err = lzo1x_decompress(cpage->page_data, cpage->page_len,
			out_page->page_data, &new_len, NULL );
	if (lib_err == LZO_E_OK && new_len == 4096) {
		//evrything is fine
	} else {
		var_error("Error in decompressing page: %u", cpage->page_id);
		return ACP_ERR_LZO_INTERNAL;
	}
	out_page->page_id = cpage->page_id;
	return ACP_OK;
}
/**
 * @brief Updates a label which represents some information.This routine is
 * 			thread safe.
 * @param label The global lable in acp_state which needs to be updated.
 * @param new_content The content which will appear on the designated label.
 */
static void update_label(struct acp_global_labels *label, char new_content[]) {
	char *mesg[2];
	curs_set(0);

	mesg[0] = copyChar(new_content);
	pthread_mutex_lock(&cmm_mutexes.cmm_cdk_screen_mutex);
	setCDKLabel(label->lblptr, (CDK_CSTRING2) mesg, 1, false);

	drawCDKLabel(label->lblptr, false);
	pthread_mutex_unlock(&cmm_mutexes.cmm_cdk_screen_mutex);
}

// ////////////////////// CC_STORAGE_API ////////////////
/**
 * @brief The central routine which forms the interface to CC API. This
 * 			routine stores a compressed page as pointed by cpage in to the cell.
 * 			Where the page is stored (in CC or in swap) is not visible to caller.
 * @param cpage The compressed page.
 * @return Returns a code indicating operation was SUCCESS or FAILURE.
 */
static int store_page_in_cc(struct compressed_page_t *cpage) {
	struct cell_t *target_cell = NULL;
	int n=cpage->page_len + 6;
//	var_debug("to store cpage: %u", cpage->page_id);
	if (!cmm_module_state.cell_table) {
		// initial case
		target_cell = allocate_new_cell();
//		var_debug("new cell: %u",target_cell->cell_id);
		var_debug("will cache page: %u in cell: %u",cpage->page_id,target_cell->cell_id);
		save_page_to_cell(target_cell, cpage);

		cmm_module_state.cell_table = cmm_module_state.cell_table_last_ele =
				target_cell;
		cmm_module_state.cell_table->next =
				cmm_module_state.cell_table_last_ele->next = NULL;
	} else {
		// find the required cell with best fit strategy
		target_cell = find_smallest_cell_from_celltable(cpage->page_len);
		if (target_cell){
//			var_debug("cid: %u avail_size: %u page_count: %u",target_cell->cell_id,target_cell->available_size,target_cell->stored_page_count);
		}

		if (!target_cell) {
			target_cell = allocate_new_cell();
//			var_debug("new cell: %u",target_cell->cell_id);
//			var_debug("No space currently,new cell: %u",target_cell->cell_id);
			if (!target_cell) {
				// This is bad neither do we have a cell with enough space to
				// accomodate this page nor can we allocate a new one.
				// log an error and return
				// TODO: Here we swap the page to disk. So basically the interface to
				// swapping unit in cmm comes here.
				var_error(
						"store_page_in_cc(): No cell to fit page: [%u], neither can allocate a new one.",
						cpage->page_id);
				return ACP_ERR_NO_CELL;
			}
//			var_debug("Will store page [%u] with req_len [%d] in cell: %u",cpage->page_id,(n),target_cell->cell_id);
			var_debug("will cache page: %u in cell: %u",cpage->page_id,target_cell->cell_id);
			save_page_to_cell(target_cell, cpage);
			//form the list
			cmm_module_state.cell_table_last_ele->next = target_cell;
			cmm_module_state.cell_table_last_ele = target_cell;
			cmm_module_state.cell_table_last_ele = NULL;
			return ACP_OK;
		}
//		var_debug("Will store page [%u] with req_len [%d] in cell: %u",cpage->page_id,(n),target_cell->cell_id);
		var_debug("will cache page: %u in cell: %u",cpage->page_id,target_cell->cell_id);
		save_page_to_cell(target_cell, cpage);
	}
	return ACP_OK;
}
/**
 * @brief Allocates memory for a new cell if possible. The newly allocated
 * 			cell is also provided with some initial attributes.
 * @return If a new cell could be allocated then a non-null pointer or in case of
 * 			falure a NULL.
 */
static struct cell_t *allocate_new_cell() {
	struct cell_t *node = NULL;
	if (cmm_module_state.cc_cells_active
			>= cmm_module_state.cc_max_cell_count) {
		serror("allocate_new_cell(): Max cell allocation limit reached.");
		return NULL ;
	}
	node = (struct cell_t*) malloc(sizeof(struct cell_t));
	if (!node) {
		return NULL ;
	}
	node->available_size = 12288;
	node->cell_id = cmm_module_state.cc_cell_id_counter;
	// increment cell_id_counter
	cmm_module_state.cc_cell_id_counter = cmm_module_state.cc_cell_id_counter
			+ 1;

	node->stored_page_count = 0;
	node->next = NULL;
	memset(node->data, 0, 12288);

	//also increment cell count
	pthread_mutex_lock(&cmm_mutexes.cmm_cc_stats_mutex);
	cmm_module_state.cc_cells_active = cmm_module_state.cc_cells_active + 1;
	cmm_module_state.cc_current_size = cmm_module_state.cc_current_size
			+ sizeof(struct cell_t);
	pthread_mutex_unlock(&cmm_mutexes.cmm_cc_stats_mutex);
	return node;
}
/**
 * @brief Save a page in the specified target cell.
 * @param target_cell The cell where this page will be stored. Note that
 * 			this routine does not try to check if this page can be stored in
 * 			this cell or not. It's the caller's responsibility.
 * @param cpage The compressed page which will be stored in the specified cell.
 */
static void save_page_to_cell(struct cell_t *target_cell,
		struct compressed_page_t *cpage) {
	int page_data_index = 0;
	int next_page_id = 0;
	int next_page_len = 0;
	unsigned char tmp_buff[4];
//	var_debug("in save_page_to_cell: [%u] page: [%u]",target_cell->cell_id,cpage->page_id);
	do {
		next_page_id = find_next_pageid_from_cell(target_cell->data, &page_data_index);
		if (next_page_id == 0) {
			break;
		}
		next_page_len = find_next_pagelen_from_cell(target_cell->data, &page_data_index);
		// move index len_no. of bytes
		page_data_index = page_data_index + next_page_len;
	} while (next_page_id);
	// so index is from where we will save the page.
	//first save the page_id
	memset(tmp_buff, 0, 4);
	store32(tmp_buff, cpage->page_id);
	memcpy((target_cell->data + page_data_index), tmp_buff, 4);
	page_data_index = page_data_index + 4;

	//now save page_len
	memset(tmp_buff, 0, 4);
	store16(tmp_buff, cpage->page_len);
	memcpy((target_cell->data + page_data_index), tmp_buff, 2);
	page_data_index = page_data_index + 2;

	//now we save the page_data in compressed form
	memcpy((target_cell->data + page_data_index), cpage->page_data, cpage->page_len);
//	page_data_index = page_data_index + cpage->page_len; //not needed as such

	//update stats for this cell
	target_cell->available_size = target_cell->available_size - cpage->page_len
			- 4 - 2; // 4 for page_id and 2 for page_len fields
	target_cell->stored_page_count = target_cell->stored_page_count + 1;

	//update the pages_cached flag
	pthread_mutex_lock(&cmm_mutexes.cmm_cc_stats_mutex);
	cmm_module_state.cc_current_stored_pages =
			cmm_module_state.cc_current_stored_pages + 1;
	pthread_mutex_unlock(&cmm_mutexes.cmm_cc_stats_mutex);
}
/**
 * @brief Returns the next page from the data section of a cell.
 * @param cell_data cell_data is 12KB of buffer where a compressed page is stored.
 * 			Note that depdending upon compression , no. of pages stored in the
 * 			cell may vary.
 * @param index Last processed index, that we have seen so far in cell_data
 * 			buffer under consideration.
 * @return Return page_id or 0 if a page is not present
 */
static unsigned int find_next_pageid_from_cell(const unsigned char *cell_data,
		int *index) {
	//page_id is of 4 bytes
	unsigned int page_id = 0;
	unsigned char tmp_buff[4];
	memcpy(tmp_buff, cell_data + (*index), 4);

	load32(tmp_buff, &page_id);
	if (page_id){
		*index = *index + 4; //advance to four positions next
	}
	return page_id;
}
/**
 * @brief Similar to find_next_pageid_from_cell() routine this routine finds the
 * 			page_len of next page stored in the cell_data section of a cell.
 * @param cell_data The cell_data buffer under consideration.
 * @param index The last processed index
 * @return page_len of the next page found in the data buffer of cell.
 */
static unsigned int find_next_pagelen_from_cell(const unsigned char *cell_data,
		int *index) {
	//page_len is of 2 bytes
	_uint16 page_len = 0;
	unsigned char tmp_buff[2];
	memcpy(tmp_buff, cell_data + (*index), 2);

	load16(tmp_buff, &page_len);
	if (page_len){
		*index = *index + 2; //advance to four positions next
	}
	return page_len;
}

/**
 * @brief Finds the smallest cell which has space just enough to store a page
 * 			of length cpage_len.
 * @param cpage_len Length of page which will be stored.
 * @return A pointer to such a cell or NULL if no such cell.
 */
static struct cell_t * find_smallest_cell_from_celltable(_uint16 cpage_len) {
	int required_len = cpage_len + 4 + 2;
	struct cell_t *smallest_cell=NULL, *cur_cell = NULL;
	for (cur_cell = cmm_module_state.cell_table; cur_cell != NULL ; cur_cell =
			cur_cell->next) {
		if ((cur_cell->available_size) > required_len) {
			if (!smallest_cell) {
				smallest_cell = cur_cell;
				continue;
			}
			if ((cur_cell->available_size) < (smallest_cell->available_size)) {
				smallest_cell = cur_cell;
			}
		}
	}
//	var_debug("smallest search: req len: %d avail: %u",required_len,smallest_cell->available_size);
	return smallest_cell;
}
/**
 * @brief Clear cell_table from memory. This includes cleaning of all cells and
 * 			the data held by them.
 * @param head_ele The front of the cell_table.
 */
static void free_cell_table(struct cell_t *head_ele) {
	struct cell_t *prev_node = head_ele, *cur_node = head_ele;
//	pthread_mutex_lock(&cmm_mutexes.cmm_cell_table_mutex);
	if (!head_ele){
		cmm_module_state.page_table = NULL;
		return ;
	}

	do {
		prev_node = cur_node;
		cur_node = cur_node->next;
		free(prev_node);
	} while (cur_node);
	//we have emptied page_table so set the pointer to null.
	cmm_module_state.page_table = NULL;
//	pthread_mutex_unlock(&cmm_mutexes.cmm_cell_table_mutex);
}
/**
 * @brief Remove a page from page table in memory.
 * @param node The page which will be removed from the page table.
 * @return A status indicating whether operation succeded or failed.
 */
static int remove_page_from_page_table(struct page_table **node) {
	/*
	 * Though not an efficient way but since we know that oldest page
	 * is the first one. So removing a page from page table always means
	 * removing the first element.
	 */
	struct page_table *to_be_deleted = *node;
	if (cmm_module_state.page_table == NULL ) {
		sdebug("cmm_mem_page_table is empty !!");
		return ACP_ERR_MEM_FAULT;
	}
//	var_debug("Removing [%u] next [%u]",to_be_deleted->page.page_id,((*node)->next)->page.page_id);
//	var_debug("Trying to remove page %u", (*node)->page.page_id);
	if (!*node) {
		serror("Trying to delete a NULL page from page_table");
		return ACP_ERR_MEM_FAULT;
	}

	pthread_mutex_lock(&cmm_mutexes.cmm_mem_page_table_mutex);
	cmm_module_state.page_table = (*node)->next;
	free(to_be_deleted);
	pthread_mutex_unlock(&cmm_mutexes.cmm_mem_page_table_mutex);
	//we just removed a page from page_table , decrement pages active count

	pthread_mutex_lock(&cmm_mutexes.cmm_mem_stats_mutex);
	cmm_module_state.pages_active = cmm_module_state.pages_active - 1;
	pthread_mutex_unlock(&cmm_mutexes.cmm_mem_stats_mutex);
	return ACP_OK;
}

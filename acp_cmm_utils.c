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
static int compress_page(const struct page_t *src_page,
		struct compressed_page_t *out_page, unsigned int *out_len);
static void update_label(struct acp_global_labels *label, char new_content[]);
static int decompress_page(struct compressed_page_t *cpage,
		struct page_t *out_page);
static void page_data_filler(char *page_data);

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

	cmm_module_state.cmm_page_id_counter = 1;
	cmm_module_state.swap_table = NULL;
	cmm_module_state.swap_table_last_ele = NULL;
	cmm_module_state.used_swap_space = 0;
	cmm_module_state.page_out_timelag = 0;
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
	 */
	pthread_exit(NULL );

}
static void *cmm_cc_stats_collector(void *args) {
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
static void page_data_filler(char *page_data) {
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
		free(prev_node);
	} while (cur_node);
	//we have emptied page_table so set the pointer to null.
	cmm_module_state.page_table = NULL;
}
/* Work-memory needed for compression. Allocate memory in units
 * of 'lzo_align_t' (instead of 'char') to make sure it is properly aligned.
 */

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);

static int compress_page(const struct page_t *src_page,
		struct compressed_page_t *out_page, unsigned int *out_len) {
	unsigned int in_len = 4096;
	int lib_err;
	*out_len = in_len + in_len / 16 + 64 + 3;
	unsigned char *tmp_out_buff = (unsigned char*) malloc(*out_len);
	if (!tmp_out_buff) {
		serror("malloc: failed");
		return ACP_ERR_NO_MEM;
	}

	//allocate memory to compressed page
	out_page = (struct compressed_page_t *) malloc(
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
			out_len, wrkmem);
	if (lib_err == LZO_E_OK) {
		var_debug("compressed %lu bytes into %lu bytes",
				(unsigned long ) in_len, (unsigned long ) *out_len);
	} else {
		/* this should NEVER happen */
		serror("Internal Compression error");
		return ACP_ERR_LZO_INTERNAL;
	}
	/* check for an incompressible block */
	if (*out_len >= in_len) {
		// we might want to discard this field
		var_warn("Page %u contains incompressible data.", src_page->page_id);
	}

	/*
	 * now allocate actual out_len bytes of memory to out and copy the
	 * tmp_out_buff contents to it.
	 */
	out_page->page_data = (unsigned char *) malloc(*out_len);
	if (!out_page) {
		serror("malloc: failed");
		return ACP_ERR_NO_MEM;
	}

	memcpy(out_page->page_data, tmp_out_buff, *out_len);
	out_page->page_id = src_page->page_id;
	out_page->page_len = *out_len;
	out_page->next = NULL;

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
static int decompress_page(struct compressed_page_t *cpage,
		struct page_t *out_page) {
	if (lzo_init() != LZO_E_OK) {
		serror("Error initializing lzo lib.")
		return ACP_ERR_LZO_INIT;
	}
	int new_len = 0, lib_err;
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
static void update_label(struct acp_global_labels *label, char new_content[]) {
	char *mesg[2];
	curs_set(0);

	mesg[0] = copyChar(new_content);
	pthread_mutex_lock(&cmm_mutexes.cmm_cdk_screen_mutex);
	setCDKLabel(label->lblptr, (CDK_CSTRING2) mesg, 1, false);

	drawCDKLabel(label->lblptr, false);
	pthread_mutex_unlock(&cmm_mutexes.cmm_cdk_screen_mutex);
}

/**
 * @file acp_gmm_utils.h
 * @brief Header file for acp_gmm_utils.c
 */

#ifndef ACP_GMM_UTILS_H_
#define ACP_GMM_UTILS_H_
#include "acp_common.h"

#define GMM_SWAP_FILE "gmm_swap.file"
// warning : variables in this structure will be updated by threads so do not
// modify the values directly
struct gmm_module_state_t {
	//immutable structures
	unsigned int swappiness_threshold_in_pages ; /**< No.of pages allocated when we start swapping. */
	unsigned long int max_mem_limit; /**< In bytes, memory limit which we cant cross,size of whole buffer*/
	unsigned int max_page_limit; /**< Max. no pages that we can allocate including both active and swapped*/
	unsigned long int max_swap_size ; /**< Max. size in bytes that will be allocated for
	 	 	 	 	 	 	 	 	 storing pages. Note we talking about storing page_data only
	 	 	 	 	 	 	 	 	 not size used in storing metadata for pages.
	 	 	 	 	 	 	 	 	 Thats an overhead but we choose to ignore it.*/
	unsigned int max_swap_pages; /**< Max. pages that can be stored in swap*/

	//mutable variables
	unsigned long int total_memory; /**< How many bytes currently allocated*/
	unsigned int total_pages; /**< Total pages allocated so far, including pages swapped and active*/
	unsigned int pages_active; /**< Total active pages*/

	//threads that we will be working with
	pthread_t gmm_mem_manager; /**< Memory manager thread. will allocate new pages to global_page_table*/
	pthread_t gmm_swap_manager; /**< Swap manager thread. Moves pages to swap file*/
	pthread_t gmm_mem_stats_collector; /**< Collects and prints stats related to memory area*/
	pthread_t gmm_swap_stats_collector; /**< Collects and prints stats related to swap area*/

	struct page_table *page_table; /**< Global page table for gmm module. Points to first element */
	struct page_table *page_table_last_ele; /**< Always points to the last non-null element of gmm's global
	 	 	 	 	 	 	 	 	 	 	 page table*/
} gmm_module_state;
/**
 * @struct swap_page_table_t
 * @brief A linked list of pages that are stored in swap file. A lookup in this
 * 			table for page will tell whether a page is stored in the swap file or not.
 */
struct swap_page_table_t{
	unsigned int page_id; /**< Page id of a page stored in swap file.*/
	unsigned int swap_record_index; /**< Index of record in swap file.*/
	struct swap_page_table_t *next;
};
/**
 * @struct swap_file_record_t
 * @brief Defines a record in swap file.
 */
struct swap_file_record_t{
	bool valid_record; /**< Is this record valid ? When a page is swapped out to swap file
								then this bit will be set to true. However when swap in occurs
								for this record then this field will be set to false and so it will
								mean a record deleted or not present.*/
	unsigned int page_id; /**< Page id of the page stored in this record */
	unsigned char record_data[4096]; /**< Stores page data*/
};
void *acp_gmm_main(void *args);
#endif /* ACP_GMM_UTILS_H_ */

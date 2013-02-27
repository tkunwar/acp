/**
 * @file acp_gmm_utils.h
 * @brief Header file for acp_gmm_utils.c
 */

#ifndef ACP_GMM_UTILS_H_
#define ACP_GMM_UTILS_H_
#include "acp_common.h"

//warning : variables in this structure will be updated by threads so donot
// modify the values directly
struct gmm_module_state_t {
	//immutable structures
	unsigned int swappiness_threshold_in_pages ; /**< No.of pages allocated when we start swapping. */
	unsigned long int max_mem_limit; /**< In bytes, memory limit which we cant cross,size of whole buffer*/
	unsigned int max_page_limit; /**< Max. no pages that we can allocate including both active and swapped*/
	unsigned long int max_swap_size ; /**< Max. size in bytes that will be allocated for
	 	 	 	 	 	 	 	 	 storing pages. Note we talking about storing page_data only
	 	 	 	 	 	 	 	 	 not size used in storing metadata for pages.*/
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
void *acp_gmm_main(void *args);
#endif /* ACP_GMM_UTILS_H_ */

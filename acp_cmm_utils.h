/**
 * @file acp_cmm_utils.h
 * @brief Header file for acp_cmm_utils.c
 */

#ifndef ACP_CMM_UTILS_H_
#define ACP_CMM_UTILS_H_
#include "acp_common.h"

#define CMM_SWAP_FILE "cmm_swap.file"
/**
 * @struct compressed_page_t
 * @brief Data structure for storing a list of compressed pages allocate at heap.
 */
struct compressed_page_t{
	unsigned int page_id; /**< page_id of stored page*/
	unsigned char* page_data; /**< pointer to compressed buffer */
	_uint16 page_len;
//	struct compressed_page_t *next;
};
/**
 * @def MAX_CELL_DATA_SIZE
 * @brief Defines the max. size of data field of a cell = 12 KB
 */
#define MAX_CELL_DATA_SIZE 12288
/**
 * @struct cell_t
 * @brief Data structure for cell. Stores a list of compressed pages using best
 * 			fit strategy. Standard cell size is 12 KB.
 */
struct cell_t{
	unsigned int available_size; /**< How much space is available in this cell (in bytes) ?*/
	unsigned int stored_page_count; /**< Count of compressed pages stored */
	unsigned int cell_id;/**< This cell needs an unique id too.*/
	unsigned char data[MAX_CELL_DATA_SIZE]; /**< Stores compressed pages. Pages are stored in this format.
	 	 	 	 	 	 	 	 	 	 	 	|<page_id (_uint32)>|<page_len>(_uint16)|<page_len bytes of binary data>|
	 	 	 	 	 	 	 	 	 	 	 	The idea is to store as much pages as we can.*/
	struct cell_t *next;
};

/**
 * @struct cmm_module_state_t
 * @brief The state structure for acp cmm module.
 */
struct cmm_module_state_t {
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
	unsigned  int total_pages; /**< Total pages allocated so far, including pages swapped and active*/
	unsigned  int pages_active; /**< Total active pages*/

	//threads that we will be working with
	pthread_t cmm_mem_manager; /**< Memory manager thread. will allocate new pages to global_page_table*/
	pthread_t cmm_cc_manager; /**< Compressed cache manager.*/
	pthread_t cmm_swap_manager; /**< Swap manager thread. Moves pages to swap file*/
	pthread_t cmm_mem_stats_collector; /**< Collects and prints stats related to memory area*/
	pthread_t cmm_cc_stats_collector; /**< Populates statistics in compressed cache section.*/
	pthread_t cmm_swap_stats_collector; /**< Collects and prints stats related to swap area*/

	struct page_table *page_table; /**< Global page table for cmm module. Points to first element */
	struct page_table *page_table_last_ele; /**< Always points to the last non-null element of cmm's global
	 	 	 	 	 	 	 	 	 	 	 page table*/
	unsigned int cmm_page_id_counter;

	struct swap_page_table_t *swap_table;
	struct swap_page_table_t *swap_table_last_ele;

	//variables related to stats in cc
	unsigned long int cc_page_out_timelag;
	unsigned long int cc_page_in_timelag;
	unsigned long int cc_current_size;
	unsigned int cc_cells_active;
	unsigned int cc_current_stored_pages;

	unsigned int cc_caching_threshold_in_pages; /**< When do we begin caching?*/
	unsigned long int cc_max_caching_size; /**< This is the max. limit of memory that we will
	 	 	 	 	 	 	 	 	 use for caching the pages in cc. This parameter is
	 	 	 	 	 	 	 	 	 specified in bytes.*/
	unsigned int cc_cell_id_counter;
	struct cell_t *cell_table; /**< List of cells maintained for storing pages in compressed pages.*/
	struct cell_t *cell_table_last_ele; /**< Points to last element stored in cell_table.*/
	unsigned int cc_max_cell_count; /**< Max. no. of cells that can be allocated.*/

} cmm_module_state;

struct cmm_mutexes_t{
	pthread_mutex_t cmm_mem_page_table_mutex; /**< mutex for accessing and adding new nodes in the
		 	 	 	 	 	 	 	 	 	 	 	 cmm page table*/
	pthread_mutex_t cmm_mem_stats_mutex; /**< For protecting total_pages,pages_active*/
	pthread_mutex_t cmm_swap_stats_mutex; /**< For protecting variables in swap section*/
	pthread_mutex_t cmm_swap_table_mutex; /**< For protecting swap_table in swap section*/
	pthread_mutex_t cmm_cdk_screen_mutex; /**< To ensure that only one thread is writing to UI*/
	pthread_mutex_t cmm_cc_stats_mutex;
	pthread_mutex_t cmm_cell_table_mutex;
}cmm_mutexes;

void *acp_cmm_main(void *args);
#endif /* ACP_CMM_UTILS_H_ */

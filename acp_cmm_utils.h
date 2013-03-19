/*
 * acp_cmm_utils.h
 *
 *  Created on: Feb 25, 2013
 *      Author: tej
 */

#ifndef ACP_CMM_UTILS_H_
#define ACP_CMM_UTILS_H_
#include "acp_common.h"

/**
 * @struct compressed_page_t
 * @brief Data structure for storing a list of compressed pages allocate at heap.
 */
struct compressed_page_t{
	unsigned int page_id; /**< page_id of stored page*/
	unsigned char* page_data; /**< pointer to compressed buffer */
	unsigned int page_len;
	struct compressed_page_t *next;
};
/**
 * @struct cell_t
 * @brief Data structure for cell. Stores a list of compressed pages using best
 * 			fit strategy. Standard cell size is 12 KB.
 */
struct cell_t{
	unsigned int available_size; /**< How much space is available in this cell (in bytes) ?*/
	unsigned int stored_page_count; /**< Count of compressed pages stored */
	unsigned int cell_id;/**< This cell needs an unique id too.*/

	struct compressed_page_t *list_head; /**< Points to beginning of list storing compressed pages*/
	struct compressed_page_t *list_end; /**< Points to end of list. Helpful in O(1) insertion for new nodes.*/

	struct cell_t *next;
};
#endif /* ACP_CMM_UTILS_H_ */

/*
 * acp_cmm_utils.c
 *
 *  Created on: Feb 25, 2013
 *      Author: tej
 */

#include "acp_cmm_utils.h"
#include "minilzo.h"

/* Work-memory needed for compression. Allocate memory in units
 * of 'lzo_align_t' (instead of 'char') to make sure it is properly aligned.
 */

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);

int compress_page(const struct page_t *src_page,
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
	out_page = (struct compressed_page_t *)malloc(sizeof(struct compressed_page_t));
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
	lib_err = lzo1x_1_compress(src_page->page_data, in_len, tmp_out_buff, out_len, wrkmem);
	if (lib_err == LZO_E_OK){
		var_debug("compressed %lu bytes into %lu bytes", (unsigned long) in_len,
				(unsigned long) *out_len);
	}
	else {
		/* this should NEVER happen */
		serror("Internal Compression error");
		return ACP_ERR_LZO_INTERNAL;
	}
	/* check for an incompressible block */
	if (*out_len >= in_len) {
		// we might want to discard this field
		var_warn("Page %u contains incompressible data.",src_page->page_id);
	}

	/*
	 * now allocate actual out_len bytes of memory to out and copy the
	 * tmp_out_buff contents to it.
	 */
	out_page->page_data = (unsigned char *)malloc(*out_len);
	if (!out_page){
		serror("malloc: failed");
		return ACP_ERR_NO_MEM;
	}

	memcpy(out_page->page_data,tmp_out_buff,*out_len);
	out_page->page_id = src_page->page_id;
	out_page->page_len = *out_len;
	out_page->next= NULL;

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
int decompress_page(struct compressed_page_t *cpage,struct page_t *out_page){
	if (lzo_init() != LZO_E_OK) {
			serror("Error initializing lzo lib.")
			return ACP_ERR_LZO_INIT;
		}
	int new_len = 0,lib_err;
	lib_err = lzo1x_decompress(cpage->page_data,cpage->page_len,out_page->page_data,&new_len,NULL);
	if (lib_err == LZO_E_OK && new_len == 4096){
		//evrything is fine
	}else{
		var_error("Error in decompressing page: %u",cpage->page_id);
		return ACP_ERR_LZO_INTERNAL;
	}
	out_page->page_id = cpage->page_id;
	return ACP_OK;
}

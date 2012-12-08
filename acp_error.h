/*
 * acp_error.h
 *
 *  Created on: Nov 8, 2012
 *      Author: tej
 */

#ifndef ACP_ERROR_H_
#define ACP_ERROR_H_

#define ACP_OK 0
//standard error codes
typedef enum{ACP_ERR_INIT_GUI=2,ACP_ERR_WINDOW_SIZE_TOO_SHORT,
			ACP_ERR_NO_COLOR,
			ACP_ERR_COLOR_INIT_FAILED,
			ACP_ERR_CDK_CONSOLE_DRAW,
			ACP_ERR_WINDOW_CONFIG_MISMATCH,
			ACP_ERR_GENERIC} error_codes_t;


#endif /* ACP_ERROR_H_ */

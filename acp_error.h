/*
 * acp_error.h
 *
 *  Created on: Nov 8, 2012
 *      Author: tej
 */

#ifndef ACP_ERROR_H_
#define ACP_ERROR_H_

#ifndef ACP_DTYPES_H_
	#include "acp_dtypes.h"
#endif

//standard error codes
typedef enum {
    ACP_ERR_ACP_STATE_INIT = 2,
    ACP_ERR_NO_CONFIG_FILE,
    ACP_ERR_CONFIG_ABORT, /*Configuration is unacceptable, abort */
    ACP_ERR_INIT_UI ,
    ACP_ERR_WINDOW_SIZE_TOO_SHORT,
    ACP_ERR_NO_COLOR,
    ACP_ERR_COLOR_INIT_FAILED,
    ACP_ERR_CDK_CONSOLE_DRAW,
    ACP_ERR_WINDOW_CONFIG_MISMATCH,
    ACP_ERR_DRAW_WINDOWS,
    ACP_ERR_GENERIC
} error_codes_t;

typedef enum {
    LOG_DEBUG, LOG_WARN, LOG_ERROR
} log_level_t;

#endif /* ACP_ERROR_H_ */

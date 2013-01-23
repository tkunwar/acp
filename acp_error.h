/**
 * @file acp_error.h
 * @brief This file defines error codes which will be used throughout acp.
 * @author Tej
 */
#ifndef ACP_ERROR_H_
#define ACP_ERROR_H_

#ifndef ACP_DTYPES_H_
	#include "acp_dtypes.h"
#endif

/**
 * @enum error_codes_t
 * @brief Defines error codes
 */
typedef enum {
    ACP_ERR_ACP_STATE_INIT = 2, /**< Error in initializing acp_state */
    ACP_ERR_NO_CONFIG_FILE,  /**<  Configuration file is missing */
    ACP_ERR_CONFIG_ABORT,  /**< Fatal error in accessing configuration file */
    ACP_ERR_INIT_UI ,    /**< Error initializing UI */
    ACP_ERR_WINDOW_SIZE_TOO_SHORT, /**< UI error: window size specified is too short */
    ACP_ERR_NO_COLOR,  /**< UI Error: curses does not have support for colors */
    ACP_ERR_COLOR_INIT_FAILED,/**< UI Error: Failed to activate color mode*/
    ACP_ERR_CDK_CONSOLE_DRAW,/**< Error drawing console window */
    ACP_ERR_WINDOW_CONFIG_MISMATCH,/**< UI Error: config mismatch in specified windows's sizes */
    ACP_ERR_DRAW_WINDOWS,/**< Error in drawing windows*/
    ACP_ERR_GENERIC /** Generic error: not sure what it is, but it's fishy anyway*/
} error_codes_t;
/**
 * @enum log_level_t
 * @brief Defines logging levels.
 */
typedef enum {
    LOG_DEBUG, /**< The message is for debugging purposes. */
    LOG_WARN, /**< The message is a warning */
    LOG_ERROR /**< The message is an error */
} log_level_t;

#endif /* ACP_ERROR_H_ */

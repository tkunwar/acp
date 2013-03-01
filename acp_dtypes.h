/**
 * @file acp_dtypes.h
 * @brief Define custom data types that will be used universally.
 * @author Tej
 */
#ifndef ACP_DTYPES_H_
#define ACP_DTYPES_H_
/**
 * @def ACP_OK
 * @brief Symbol meaning that a operation has completed sucessfully.
 */
#define ACP_OK 0
/**
 * @enum ON_OFF
 * @brief Creates two symbols on and off having values 1 and 0 respectively.
 */
typedef enum{
	off, /**< Signifies that value is reset */
	on /**< Signifies that value is set */
} ON_OFF;

#endif /* ACP_DTYPES_H_ */

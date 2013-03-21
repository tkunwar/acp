/**
 * @file acp_dtypes.h
 * @brief Define custom data types that will be used universally.
 * @author Tej
 */
#ifndef ACP_DTYPES_H_
#define ACP_DTYPES_H_

typedef unsigned short int _uint16;
typedef unsigned int _uint32;
typedef unsigned long int _uint64;


/**
 * @enum ON_OFF
 * @brief Creates two symbols on and off having values 1 and 0 respectively.
 */
typedef enum{
	off, /**< Signifies that value is reset */
	on /**< Signifies that value is set */
} ON_OFF;

#endif /* ACP_DTYPES_H_ */

#ifndef PTHREAD_BITMASK_H_INCLUDED
#define PTHREAD_BITMASK_H_INCLUDED

// https://www.learn-c.org/en/Bitmasks

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>

typedef struct {
	char * bm_mask;
	size_t bm_len;
	pthread_mutex_t bm_mutex;
} bitmask_t;

/*
*	DESCRIPTION
*		Initializes a bitmask, allocates memory and sets bm_len
*			All bit are set to 0
*
*	PARAMETERS
*		bitm:	Pointer to the bitmask
*		len:	The number of elements in the bitmask
*
*	RETURN VALUE
*		On success:	0
*		On error:	-1
*/
int bitmask_init(bitmask_t *bitm, size_t len);

/*
*	DESCRIPTION
*		Closes a bitmask, frees memory
*
*	PARAMETERS
*		bitm:	Pointer to the bitmask
*
*	RETURN VALUE
*		On success:	0
*/
int bitmask_close(bitmask_t *bitm);

/*
*	DESCRIPTION
*		Prints all the bytes in the bitmask
*
*	PARAMETERS
*		bitm: Pointer to the bitmask
*/
void bitmask_print(bitmask_t *bitm);

/*
*	DESCRIPTION
*		Sets all bits in a bit mask to 1
*
*	RETURN VALUE
*		On success:	0
*/
int bitmask_fill(bitmask_t *bitm);

/*
*	DESCRIPTION
*		Sets all bits in a bit mask to 0
*
*	RETURN VALUE
*		On success:	0
*/
int bitmask_empty(bitmask_t *bitm);

/*
*	DESCRIPTION
*		Sets the target-th bit in a bit mask to 1
*
*	RETURN VALUE
*		On success:	0
*/
int bitmask_add(bitmask_t *bitm, size_t target);

/*
*	DESCRIPTION
*		Sets the target-th bit in a bit mask to 0
*
*	RETURN VALUE
*		On success:	0
*/
int bitmask_del(bitmask_t *bitm, size_t target);

/*
*	DESCRIPTION
*		Returns value of the target-th bit in a bit mask
*
*	RETURN
*		On success:	The value of the bit
*		On error: -1
*/
int bitmask_is_member(bitmask_t *bitm, size_t target);

/*
*	DESCRIPTION
*		Returns value of the target-th bit in a bit mask
*			Doesn't block the bitmask
*
*	RETURN
*		On success:	The value of the bit
*		On error: -1
*/
int bitmask_is_member_nolock(bitmask_t *bitm, size_t target);

/*
*	DESCRIPTION
*		Looks for the first bit set to 1 in a bit mask
*
*	RETURN
*		On success, if bit found:	The index of the bit
*		On success, if bit not found: -1
*/
size_t bitmask_find_first_set(bitmask_t *bitm);


/*
*	DESCRIPTION
*		Converts from length of the bitmask and bytes
*			1 bit -> 1 byte
*			...
*			8 bit -> 1 byte
*			9 bit -> 2 byte
*
*	PARAMETERS
*		len:	the number of element in a bitmask
*
*	RETURN VALUE
*		The number of bytes that are needed to store len elements
*/
size_t len_to_bytes(size_t len);

/*
*	DESCRIPTION
*		Allocates and returns a string with the binary representation of a byte
*
*	PARAMETERS
*		byte:	The byte to print
*
*	RETURN VALUE
*		On succes: 	returns pointer to a string allocated in heap (should be freed).
*		On error:	returns NULL
*/
char *byte_in_binary(char byte);

/*
*	DESCRIPTION
*		Same as byte_in_binary() but limits the bits printed
*
*	EXAMPLE
*		Only the first 4 bit of a byte are used, so byte_in_binary(byte, 4);
*
*	PARAMETERS
*		byte:	The byte to print
*		limit:	The amount of bits to print
*
*	RETURN VALUE
*		Same as byte_in_binary()
*/
char *byte_in_binary2(char byte, int limit);

#endif

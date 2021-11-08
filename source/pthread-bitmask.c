#include "pthread-bitmask.h"

int bitmask_init(bitmask_t *bitm, size_t len){
	// Check arguments
	if(bitm == NULL || len <= 0) return -1;

	// Compute the number of bytes required
	size_t bytes = len_to_bytes(len);

	// Allocate memory
	if((bitm->bm_mask = calloc(sizeof(char), bytes)) == NULL) return -1;

	// Set len and sem
	bitm->bm_len = len;
	if(pthread_mutex_init(&bitm->bm_mutex, NULL) < 0) return -1;

	// Set all bytes to 0x00
	memset(bitm->bm_mask, 0x00, bytes);

	return 0;
}

int bitmask_close(bitmask_t *bitm){
	free(bitm->bm_mask);
	bitm->bm_len = 0;
	pthread_mutex_destroy(&bitm->bm_mutex);

	return 0;
}

void bitmask_print(bitmask_t *bitm){
	char *binary;
	size_t i;

	// Full_bytes is the number of bytes with all 8 bits meaningful
	size_t full_bytes = bitm->bm_len / 8;

	// Remaining bits are the meaninful bits (if present) in the last byte 
	int remaining_bits = bitm->bm_len - (full_bytes * 8);

	// Lock
	pthread_mutex_lock(&bitm->bm_mutex);

	for(i = 0; i < full_bytes; i++){
		binary = byte_in_binary(bitm->bm_mask[i]);
		printf("Byte[%ld] = %s\n", i, binary);
		free(binary);
	}
	if(remaining_bits > 0){
		binary = byte_in_binary2(bitm->bm_mask[i], remaining_bits);
		printf("Byte[%ld] = %s\n", i, binary);
	}
	printf("\n");

	// Unlock
	pthread_mutex_unlock(&bitm->bm_mutex);

	return;
}

int bitmask_fill(bitmask_t *bitm){
	// Lock
	pthread_mutex_lock(&bitm->bm_mutex);

	for(int i = 0; i < len_to_bytes(bitm->bm_len); i++)
		bitm->bm_mask[i] = 0xff;

	// Unlock
	pthread_mutex_unlock(&bitm->bm_mutex);

	return 0;
}

int bitmask_empty(bitmask_t *bitm){
	// Lock
	pthread_mutex_lock(&bitm->bm_mutex);

	for(int i = 0; i < len_to_bytes(bitm->bm_len); i++){
		bitm->bm_mask[i] = 0x00;
	}

	// Unlock
	pthread_mutex_unlock(&bitm->bm_mutex);
	
	return 0;
}

int bitmask_add(bitmask_t *bitm, size_t target){
	if(target < 0 || target >= bitm->bm_len) return -1;

	// Lock
	pthread_mutex_lock(&bitm->bm_mutex);

	// The first n-3 bits are used as byte index
	// The last 3 bits are used as index in the byte
	bitm->bm_mask[target >> 3] |= (1 << target % 8);

	// Unlock
	pthread_mutex_unlock(&bitm->bm_mutex);

	return 0;
}

int bitmask_del(bitmask_t *bitm, size_t target){
	if(target < 0 || target >= bitm->bm_len) return -1;

	// Lock
	pthread_mutex_lock(&bitm->bm_mutex);

	// The first n-3 bits are used as byte index
	// The last 3 bits are used as index in the byte
	bitm->bm_mask[target >> 3] &= ~(1 << target % 8);

	// Unlock
	pthread_mutex_unlock(&bitm->bm_mutex);

	return 0;
}

int bitmask_is_member(bitmask_t *bitm, size_t target){
	if(target < 0 || target >= bitm->bm_len) return -1;

	// Lock
	pthread_mutex_lock(&bitm->bm_mutex);

	char result = bitm->bm_mask[target >> 3] & (1 << target % 8);

	// Unlock
	pthread_mutex_unlock(&bitm->bm_mutex);

	return result != 0;
}

int bitmask_is_member_nolock(bitmask_t *bitm, size_t target){
	if(target < 0 || target >= bitm->bm_len) return -1;

	char result = bitm->bm_mask[target >> 3] & (1 << target % 8);

	return result != 0;
}

size_t bitmask_find_first_set(bitmask_t *bitm){

	// Lock
	pthread_mutex_lock(&bitm->bm_mutex);

	for(uint64_t i = 0; i < bitm->bm_len; i++){
		if(bitmask_is_member_nolock(bitm, i)){

			// Unlock
			pthread_mutex_unlock(&bitm->bm_mutex);

			return i;
		}
	}

	// Unlock
	pthread_mutex_unlock(&bitm->bm_mutex);

	return -1;
}

size_t len_to_bytes(size_t len){
	return ((len - 1) / 8) + 1;
}

char *byte_in_binary(char byte){
	// Allocate the memory
	char *string = malloc(9);
	if(string == NULL) return NULL;

	string[8] = '\0';

	// Fill the string
	for(int i = 0; i < 8; i++)
		string[i] = (byte & (1 << i)) ? '1' : '0';

	return string;
}

char *byte_in_binary2(char byte, int limit){
	char *string = malloc(limit + 1);
	string[limit] = '\0';

	for(int i = 0; i < limit; i++){
		string[i] = (byte & (1 << i)) ? '1' : '0';
	}
	
	return string;
}

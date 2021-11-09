#ifndef CAESAR_CIPHER_H_INCLUDED
#define CAESAR_CIPHER_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Char between ' ' and '~' are printable
const char lowest = ' ';
const char highest = '~';
const char range = (highest - lowest + 1);

/*
	DESCRIPTION:
		A modified version of the Caesar cipher, in which the amount of displacement
			is incremented on every character by increment.
	RETURNS:
		A pointer to the new string in case of success.
		NULL in case of unsuccess.
*/
char *caesar_cipher_2(char *source, int amount, int increment){
	int new_char;

	// Allocate memory for the new string
	size_t source_len = strlen(source);
	char *output = calloc(sizeof(char), source_len + 1);
	if(output == NULL) return NULL;
	output[source_len] = '\0';

	// If amount == 0 and increment == 0 -> no character will be modified
	if(amount == 0 && increment == 0) return strcpy(output, source);

	// Cycle through the original string
	for(int i = 0; i < source_len; i++){
		// If char is between ' ' and '~' -> char is printable -> char should be modified 
		if(source[i] >= lowest && source[i] <= highest){
			// Displace
			new_char = source[i] + amount;

			// Normalize in range [' ', '~']
			while(new_char > highest)
				new_char -= range;
			while(new_char < lowest)
				new_char += range;

			// Write in new string
			output[i] = new_char;

			// Update the amount of the displacement			
			amount += increment;

		// Else -> char is not printable -> char should not be modified
		} else {
			output[i] = source[i];
		}
	}

	return output;
}

/*
	DESCRIPTION:
		A version of the Caesar cipher.
	RETURNS:
		A pointer to the new string in case of success.
		NULL in case of unsuccess.
*/
char *caesar_cipher(char *source, int amount){
	return caesar_cipher_2(source, amount, 0);
}

#endif

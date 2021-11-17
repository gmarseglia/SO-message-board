/*
**	Encrypt and decrypt using Caesar cipher.
**		Also present a different version with increasing amount of displacement.
*/

#ifndef CAESAR_CIPHER_H_INCLUDED	/* Double-inclusion guard */
	#define CAESAR_CIPHER_H_INCLUDED

	/* Headers */
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

	/* Only char between ' ' and '~' are printable */
	const char lowest = ' ';
	const char highest = '~';
	const char range = (highest - lowest + 1);

	/*
	**	DESCRIPTION
	**		A modified version of the Caesar cipher, in which the amount of displacement
	**			is incremented on every character by increment.
	**
	**	RETURN VALUE
	**		A pointer to the new string in case of success.
	**		NULL in case of unsuccess.
	*/
	char *caesar_cipher_2(char *source, int amount, int increment){
		int new_char;

		/* Allocate memory for the new string */
		size_t source_len = strlen(source);
		char *output = calloc(sizeof(char), source_len + 1);
		if(output == NULL) return NULL;
		output[source_len] = '\0';

		/* If amount == 0 and increment == 0 then no character will be modified */
		if(amount == 0 && increment == 0) return strcpy(output, source);

		/* Cycle through the original string */
		for(int i = 0; i < source_len; i++){
			/* If char is between ' ' and '~' -> char is printable -> char should be modified */
			if(source[i] >= lowest && source[i] <= highest){

				new_char = source[i] + amount;	/* Apply displacement */

				/* Normalize in range [' ', '~'] */
				while(new_char > highest)
					new_char -= range;
				while(new_char < lowest)
					new_char += range;

				output[i] = new_char; /* Write in new string */

				/* The amount of the displacement is incremented every time by "increment" */
				amount += increment; 

			/* Else -> char is not printable so char should not be modified */
			} else {
				output[i] = source[i];
			}
		}

		return output;
	}

	/*
	**	DESCRIPTION
	**		A version of the Caesar cipher.
	**
	**	RETURNS
	**		A pointer to the new string in case of success.
	**		NULL in case of unsuccess.
	*/
	char *caesar_cipher(char *source, int amount){
		return caesar_cipher_2(source, amount, 0);
	}

#endif

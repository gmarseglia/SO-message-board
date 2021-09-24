#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define MAX_ID 1
#define MAX_USERNAME 16
#define MAX_PASSWD 16

const char *user_filename = "file.txt";

int main(int argc, char const *argv[])
{
	FILE *user_file = fopen(user_filename, "r+");
	if(user_file == NULL){
		perror("Error in fopen");
		exit(EXIT_FAILURE);
	}

	int lines_read = 0;

	#ifdef FGETS
	printf("FGETS declared\n");
	int max_line_len = MAX_ID + MAX_USERNAME + MAX_PASSWD;
	char buffer[max_line_len];
	while(fgets(buffer, max_line_len, user_file) != NULL) lines_read++;
	#endif

	#ifdef FSCANF
	printf("FSCANF declared\n");
	char *buffer, *last_buffer = NULL;
	while(fscanf(user_file, "%m[^\n]", &buffer) != EOF){
		fgetc(user_file);
		lines_read++;
		if(last_buffer != NULL) free(last_buffer);
		last_buffer = buffer;
	}
	buffer = last_buffer;
	#endif

	// This is faster
	#ifdef FSEEK
	printf("FSEEK declared\n");
	int max_line_len = MAX_ID + MAX_USERNAME + MAX_PASSWD;
	char buffer[max_line_len];
	fseek(user_file, -max_line_len, SEEK_END);
	while(fgets(buffer, max_line_len, user_file) != NULL) lines_read++;
	#endif

	printf("read %d lines, last line=%s\n", lines_read, buffer);

	return 0;
}
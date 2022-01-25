#include "server.h"
#include "../common/caesar-cipher.h"

const int amount = 10;
const int increment = 20; 

extern __thread int acceptfd;
extern __thread user_info_t client_ui;
extern __thread operation_t op;
extern __thread int id;

__thread int read_uid;
__thread char read_op;
__thread user_info_t *read_ui;
__thread FILE *users_file;


/*
**	DESCRIPTION
**		Allows Client to register their credentials.
**
**	RETURN VALUE
**		In case of success: 0
**		In case of unsuccess, retry possible: -1
**		In case of error: exit(EXIT_FAILURE)
*/
int registration();

/*
**	DESCRIPTION
**		Log in Client if their credentials are already been saved.
**
**	RETURN VALUE
**		In case of success: 0
**		In case of unsuccess, retry possible: -1
**		In case of error: exit(EXIT_FAILURE)
*/
int login();

// ------------------------------------------------------------

int login_registration(){

	/* #1: Receive operation to start handshake */
	if(receive_operation_from_2(acceptfd, &op) < 0)
		return -1;

	/* Op (UID_ANON, OP_OK, *) is required */
	if(op.code != OP_OK || op.uid != UID_ANON){
		/* Send OP (UID_SERVER, OP_NOT_ACCEPTED, txt) if Client sent uncorrect operation */
		send_operation_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED,
			"Incorrect handshake. Expected OP_OK.");
		return -1;
	}

	/* #2: Send operation to complete handshake 
		(UID_SERVER, OP_OK, NULL) */
	if(send_operation_to(acceptfd, UID_SERVER, OP_OK, NULL) < 0)
		return -1;
	
	printf("Thread[%d]: Handshake complete.\n", id);

	/* #3: Receive operation with mode and user info */
	if(receive_operation_from_2(acceptfd, &op) < 0)
		return -1;

	sscanf(op.text, "%ms %ms", &(client_ui.username), &(client_ui.passwd));
	free(op.text);

	/* Switch to registration, login or uncorrect mode */
	if(op.uid == UID_ANON && op.code == OP_REG)
		return registration();
	else if(op.uid == UID_ANON && op.code == OP_LOG)
		return login();
	else{
		send_operation_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED,
			"Incorrect first operation_t. Expected OP_LOG or OP_REG");
		return -1;
	}
}

int login(){

	/* #1: wait(UW, 1) */
	short_semop(UW, -1);

	/* #2: wait(UR, 1) */
	short_semop(UR, -1);

	/* #3: signal(UW, 1) */
	short_semop(UW, 1);

	/* #4: search for username and fill user info */
	read_ui = find_user_by_username(client_ui.username);

	/* #5: Signal (UR, 1) */
	short_semop(UR, 1);

	/* Check if username found */
	if(read_ui == NULL){
	/* #6a: Send operation NOT ACCEPTED */
		send_operation_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Username not found");
		return -1;
	}

	/* #6: Decrypt and compare password */
	char *double_encrypted_passwd = caesar_cipher_2(client_ui.passwd, amount, increment);

	/* Check if passwords match */
	if(strcmp(double_encrypted_passwd, read_ui->passwd) != 0){
		/* If passwords are different, then
			#7a: Send operation NOT ACCEPTED * */
		send_operation_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Passwords don't match");
		return -1;
	}
	free(double_encrypted_passwd);

	// Save UID in client_ui
	client_ui.uid = read_ui->uid;

	/* #7: Send operation OK with stored UID */
	char uid_str[6];
	sprintf(uid_str, "%d", read_ui->uid);
	if(send_operation_to(acceptfd, UID_SERVER, OP_OK, uid_str) < 0)
		return -1;

	return 0;
}

int registration(){
	
	/* #1: wait(UW, 1) */
	short_semop(UW, -1);

	/* #2: wait(UR, MAX_THREAD) */
	short_semop(UR, -MAX_THREAD);

	/* #3: search for username and fill user info */ 
	read_ui = find_user_by_username(client_ui.username);

	/* Check if username found */
	if(read_ui != NULL){
		/* If username already found, then
			#4a: signal(UR, MAX_THREAD) */
		short_semop(UR, MAX_THREAD);

		/* #5a: signal(UW, 1) */
		short_semop(UW, 1);

		/* #6a: Send operation NOT ACCEPTED */
		send_operation_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Username already existing.");

		return -1;
	}
	
	/* Open files */
	users_file = FDOPEN_USERS();
	if(users_file == NULL) perror_and_failure("FDOPEN_USERS", __func__);

	/* #4: Compute new UID */
	int last_uid;
	int file_len;
	fseek(users_file, 0, SEEK_END);
	file_len = ftell(users_file);

	/* Find last UID */
	if(file_len == 0){	/* If file is empty, last UID is INITIAL_UID*/
		last_uid = INITIAL_UID;
	} else {
		/* 5 for UID + MAXSIZE of username and passwd + 3 Blanks + 1 new line */
		int max_line_len = MAXSIZE_USERNAME + MAXSIZE_PASSWD + 5 + 3 + 1;
		char line_buffer[max_line_len + 1];		/* Allocate space */
		memset(line_buffer, '\0', max_line_len + 1);

		fseek(users_file, file_len > max_line_len ? -max_line_len : -file_len, SEEK_END);
		while(fgets(line_buffer, max_line_len, users_file) != NULL);	/* Read last line */
		sscanf(line_buffer, "%d", &last_uid);	/* sscanf to convert to int */
	}

	/* New UID to assign is the last UID + 1 */
	client_ui.uid = last_uid + 1;

	/* #5: Append on files */
	fseek(users_file, 0, SEEK_END);
	char *double_encrypted_passwd = caesar_cipher_2(client_ui.passwd, amount, increment);
	fprintf(users_file, "%d %s %s\n", client_ui.uid, client_ui.username, double_encrypted_passwd);
	free(double_encrypted_passwd);
	fsync(fileno(users_file));
	fclose(users_file);	/* It's important to close, to save changes on files */

	/* #6: signal(UR, MAX_THREAD) */
	short_semop(UR, MAX_THREAD);

	/* #7: signal(UW, 1) */
	short_semop(UW, 1);

	/* #8: Send operation OK with UID */
	char uid_str[6];
	sprintf(uid_str, "%d", client_ui.uid);
	if(send_operation_to(acceptfd, UID_SERVER, OP_OK, uid_str) < 0)
		return -1;

	printf("Thread[%d]: (%s, %d) registered successfully.\n", id,  client_ui.username, client_ui.uid);

	return 0;
}


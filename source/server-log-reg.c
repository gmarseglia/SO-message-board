#include "server.h"

extern __thread int acceptfd;
extern __thread user_info client_ui;
extern __thread operation op;

__thread int read_uid;
__thread char read_op;
__thread user_info *read_ui;
__thread FILE *users_file;

/*
	DESCRIPTION:
		documentation/Server-Registration.jpeg
	RETURNS:
		In case of success: 0
		In case of unsuccess, retry possible: -1
		In case of error: exit(EXIT_FAILURE)
*/
int registration();

/*
	DESCRIPTION:
		documentation/Client-Registration.png
	RETURNS:
		In case of success: 0
		In case of unsuccess, retry possible: -1
		In case of error: exit(EXIT_FAILURE)
*/
int login();
// ------------------------------------------------------------

int login_registration(){
	if(receive_message_from(acceptfd, &read_uid, &read_op, &(client_ui.username)) < 0)
		return -1;

	if(read_uid == UID_ANON && read_op == OP_REG_USERNAME)
		return registration(acceptfd, client_ui);
	else if(read_uid == UID_ANON && read_op == OP_LOG_USERNAME)
		return login(acceptfd, client_ui);
	else{
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Incorrect first operation. Expected OP_LOG_USERNAME or OP_REG_USERNAME");
		return -1;
	}
}

int registration(){
	// #2 Receive passwd
	if(receive_message_from(acceptfd, &read_uid, &read_op, &(client_ui.passwd)) < 0)
		return -1;

	// If not correct read_op OP_REG_PASSWD send read_op OP_NOT_ACCEPTED
	if(!(read_uid == UID_ANON && read_op == OP_REG_PASSWD)){
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Incorrect second operation. Expected OP_REG_PASSWD");
		return -1;
	}

	// #3 Wait (UW, 1)
	short_semop(UW, -1);

	// #4 Wait (UR, MAX_THREAD)
	short_semop(UR, -MAX_THREAD);

	// #5 Find username
	read_ui = find_user_by_username(client_ui.username);

	// If username not found
	if(read_ui != NULL){	
		// #6a Signal (UR, MAX_THREAD)
		short_semop(UR, MAX_THREAD);

		// #7a Signal (UW, 1)
		short_semop(UW, 1);

		// #8a Send OP_NOT_ACCEPTED to client
		send_message_to(acceptfd, 1, OP_NOT_ACCEPTED, "Username already existing.");

		// #9a
		return -1;
	}
	
	//	At the moment writes and reads are exclusive to this thread

	// Open Users file
	users_file = FDOPEN_USERS();
	if(users_file == NULL) perror_and_failure("FDOPEN_USERS", __func__);

	// #6b Compute new UID
	int last_uid;
	int file_len;
	fseek(users_file, 0, SEEK_END);
	file_len = ftell(users_file);

	// Find last UID
	if(file_len == 0){
		last_uid = INITIAL_UID;
	} else {
		int max_line_len = MAXSIZE_USERNAME + MAXSIZE_PASSWD + 5 + 3 + 1;
		char line_buffer[max_line_len + 1];
		memset(line_buffer, '\0', max_line_len + 1);

		fseek(users_file, file_len > max_line_len ? -max_line_len : -file_len, SEEK_END);
		while(fgets(line_buffer, max_line_len, users_file) != NULL);
		sscanf(line_buffer, "%d", &last_uid);
	}

	client_ui.uid = last_uid + 1;

	#ifdef PRINT_DEBUG_FINE
	printf("last_uid=%d, client_ui.uid=%d\n", last_uid, client_ui.uid);
	#endif

	// 7b Append on and close file
	fseek(users_file, 0, SEEK_END);
	fprintf(users_file, "%d %s %s\n", client_ui.uid, client_ui.username, client_ui.passwd);
	fclose(users_file);

	// #8b Signal (UR, MAX_THREAD)
	short_semop(UR, MAX_THREAD);

	// #9b Signal UW
	short_semop(UW, 1);

	// #10b Send client OP_REG_UID with uid
	char uid_str[6];
	sprintf(uid_str, "%d", client_ui.uid);
	if(send_message_to(acceptfd, UID_SERVER, OP_REG_UID, uid_str) < 0)
		return -1;

	// 11b go to main cycle
	return 0;
}

int login(){
	// #1: Receive passwd
	if(receive_message_from(acceptfd, &read_uid, &read_op, &(client_ui.passwd)) < 0)
		return -1;

	// #2a: op incorrect -> send OP_NOT_ACCEPTED
	if(!(read_uid == UID_ANON && read_op == OP_LOG_PASSWD)){
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Incorrect OP, expected OP_LOG_PASSWD");
		return -1;
	}

	// #2: wait(UW, 1)
	short_semop(UW, -1);

	// #3: wait(UR, 1);
	short_semop(UR, -1);

	// #4: signal(UW, 1)
	short_semop(UW, 1);

	// #5: search for username and fill user info
	read_ui = find_user_by_username(client_ui.username);

	// #6: Signal (UR, 1)
	short_semop(UR, 1);

	// #8a: If username ! found -> send OP_NOT_ACCEPTED
	if(read_ui == NULL){
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Username not found");
		return -1;
	}

	// #8: Compare password
	if(strcmp(client_ui.passwd, read_ui->passwd) != 0){
		//	9a: If passwords don't match -> Send OP_NOT_ACCEPTED
		send_message_to(acceptfd, UID_SERVER, OP_NOT_ACCEPTED, "Passwords don't match");
		return -1;
	}

	// Save UID in client_ui
	client_ui.uid = read_ui->uid;

	// #9: Send OP_LOG_UID with stored UID
	char uid_str[6];
	sprintf(uid_str, "%d", read_ui->uid);
	if(send_message_to(acceptfd, UID_SERVER, OP_LOG_UID, uid_str) < 0)
		return -1;

	// #10: return to main cycle
	return 0;
}

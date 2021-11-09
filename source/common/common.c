#include "common.h"

int send_message_to(int sockfd, int uid, char code, char *text){
	uint32_t send_uid, send_text_len;
	size_t byte_written;

	// iovec used by writev
	struct iovec iov[3];

	send_uid = (uint32_t) uid;
	iov[0].iov_base = &send_uid;
	iov[0].iov_len = sizeof(uint32_t);

	iov[1].iov_base = &code;
	iov[1].iov_len = sizeof(char);

	send_text_len = (text == NULL) ? 0 : strlen(text);
	iov[2].iov_base = &send_text_len;
	iov[2].iov_len = sizeof(uint32_t);

	// Send pre-message:
	//	writev enables to send different arrays as one
	if((byte_written = writev(sockfd, iov, 3)) < 0){
		perror("Error in send_message_to on writev");
		return -1;
	}

	#ifdef PRINT_DEBUG_MESSAGE
	printf("writev has written %d bytes\n", byte_written);
	#endif

	// If messagge == NULL, don't send second part
	if(text == NULL) return 0;

	// Send text
	if((byte_written = write(sockfd, text, send_text_len)) != send_text_len){
		perror("Error in send_message_to on write");
		return -1;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("write has written %d bytes=%s\n", byte_written, text);
	#endif

	return 0;
}

int send_operation_to(int sockfd, operation op){
	return send_message_to(sockfd, op.uid, op.code, op.text);
}

int receive_message_from(int sockfd, int *uid, char *code, char **text){
	size_t byte_read;

	uint32_t read_uid, read_text_len;
	char read_code;

	struct iovec iov[3];

	iov[0].iov_base = &read_uid;
	iov[0].iov_len = sizeof(uint32_t);

	iov[1].iov_base = (code == NULL) ? &read_code : code;
	iov[1].iov_len = sizeof(char);

	iov[2].iov_base = &read_text_len;
	iov[2].iov_len = sizeof(uint32_t);

	if((byte_read = readv(sockfd, iov, 3)) <= 0){
		if(byte_read < 0) perror("Error in receive_message_from on readv");
		return -1;
	}

	if(uid != NULL) *uid = (int) read_uid;

	#ifdef PRINT_DEBUG_MESSAGE
	printf("readv has read %d bytes, read_text_len=%d\n", byte_read, read_text_len);
	#endif

	// If text_len == 0 -> sender has not included message
	// If text == NULL -> receiver is not interested
	if(read_text_len == 0 || text == NULL){
		*text = NULL;
		return 0;
	}

	*text = calloc(sizeof(char), read_text_len + 1);
	memset(*text, '\0', read_text_len + 1);

	if((byte_read = read(sockfd, *text, read_text_len)) <= 0){
		if(byte_read < 0) perror("Error in receive_message_from on readv");
		return -1;
	}

	#ifdef PRINT_DEBUG_MESSAGE
	printf("read has read %d bytes, message=%s\n", byte_read, *text);
	#endif

	return 0;
}

int receive_operation_from(int sockfd, operation *op){
	return receive_message_from(sockfd, &(op->uid), &(op->code), &(op->text));
}

void sockaddr_in_setup_inaddr(struct sockaddr_in *addr, unsigned int ip_address, int port){
	memset(addr, 0, sizeof(struct sockaddr_in));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = ip_address;
	return;
}

void sockaddr_in_setup(struct sockaddr_in *addr, const char *ip_address, int port){
	return sockaddr_in_setup_inaddr(addr, inet_addr(ip_address), port);
}

void print_operation(operation *op){
	printf("(UID=%d, Code=%c, Text=%s)\n", op->uid, op->code, op->text);
	return;
}

int short_semop(int SEMAPHORE, int op){
	struct sembuf actual_sem_op = {.sem_flg = 0, .sem_num = 0};
	actual_sem_op.sem_op = op;
	if(semop(SEMAPHORE, &actual_sem_op, 1) < 0){
		if(errno == EINTR)	return -1;
		perror_and_failure(__func__, NULL);
	}
	return 0;
}

void perror_and_failure(const char *s, const char *func){
	if(func != NULL){
		fprintf(stderr, "Error in %s on %s:\n", func, s);
		perror(NULL);
	} else {
		perror(s);
	}
	exit_failure();
}
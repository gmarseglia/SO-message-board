#include "common.h"

int send_message_to(int sockfd, int uid, char code, char *text){
	int text_len, byte_written;
	text_len = text == NULL ? 0 : strlen(text);

	// iovec used by writev
	struct iovec iov[3];

	iov[0].iov_base = &uid;
	iov[0].iov_len = SIZEOF_INT;

	iov[1].iov_base = &code;
	iov[1].iov_len = SIZEOF_CHAR;

	iov[2].iov_base = &text_len;
	iov[2].iov_len = SIZEOF_INT;

	// Send pre-message:
	//	writev enables to send different arrays as one
	if((byte_written = writev(sockfd, iov, 3)) < 0){
		perror("Error in send_message_to on writev");
		return -1;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("writev has written %d bytes\n", byte_written);
	#endif

	// If messagge == NULL, don't send second part
	if(text == NULL)
		return 0;

	// Send text
	if((byte_written = write(sockfd, text, text_len)) 
		!= text_len){
		perror("Error in send_message_to on write");
		return -1;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("write has written %d bytes=%s\n", byte_written, text);
	#endif

	return byte_written;
}

int send_operation_to(int sockfd, operation op){
	return send_message_to(sockfd, op.uid, op.code, op.text);
}

int receive_message_from(int sockfd, int *uid, char *code, char **text){
	int text_len;
	int null_uid;
	char null_code;
	int byte_read = 0;

	struct iovec iov[3];

	iov[0].iov_base = uid == NULL ? &null_uid : uid;
	iov[0].iov_len = SIZEOF_INT;

	iov[1].iov_base = code == NULL ? &null_code : code;
	iov[1].iov_len = SIZEOF_CHAR;

	iov[2].iov_base = &text_len;
	iov[2].iov_len = SIZEOF_INT;

	if((byte_read = readv(sockfd, iov, 3)) <= 0){
		if(byte_read < 0) perror("Error in receive_message_from on readv");
		return byte_read;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("readv has read %d bytes, text_len=%d\n",
		byte_read, text_len);
	#endif

	// If text_len == 0 -> sender has not included message
	// If text == NULL -> receiver is not interested
	if(text_len == 0 || text == NULL){
		*text = NULL;
		return 0;
	}

	*text = calloc(sizeof(char), text_len + 1);
	memset(*text, '\0', text_len + 1);

	if((byte_read = read(sockfd, *text, text_len)) <= 0){
		if(byte_read < 0) perror("Error in receive_message_from on readv");
		return byte_read;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("read has read %d bytes, message=%s\n",
		byte_read, *text);
	#endif

	return byte_read;
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
#include "common.h"

int send_message_to(int sockfd, int uid, char op, char *message){
	int message_len, byte_written;
	message_len = message == NULL ? 0 : strlen(message);

	// iovec used by writev
	struct iovec iov[3];

	iov[0].iov_base = &uid;
	iov[0].iov_len = SIZEOF_INT;

	iov[1].iov_base = &op;
	iov[1].iov_len = SIZEOF_CHAR;

	iov[2].iov_base = &message_len;
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
	if(message == NULL)
		return 0;

	// Send message
	if((byte_written = write(sockfd, message, message_len)) 
		!= message_len){
		perror("Error in send_message_to on write");
		return -1;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("write has written %d bytes=%s\n", byte_written, message);
	#endif

	return byte_written;
}

int send_operation_to(int sockfd, operation op){
	return send_message_to(sockfd, op.uid, op.code, op.text);
}

int receive_message_from(int sockfd, int *uid, char *op, char **recipient){
	int message_len;
	int null_uid;
	char null_op;
	int byte_read = 0;

	struct iovec iov[3];

	iov[0].iov_base = uid == NULL ? &null_uid : uid;
	iov[0].iov_len = SIZEOF_INT;

	iov[1].iov_base = op == NULL ? &null_op : op;
	iov[1].iov_len = SIZEOF_CHAR;

	iov[2].iov_base = &message_len;
	iov[2].iov_len = SIZEOF_INT;

	if((byte_read = readv(sockfd, iov, 3)) <= 0){
		if(byte_read < 0) perror("Error in receive_message_from on readv");
		return byte_read;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("readv has read %d bytes, message_len=%d\n",
		byte_read, message_len);
	#endif

	// If message_len == 0 -> sender has not included message
	// If recipient == NULL -> receiver is not interested
	if(message_len == 0 || recipient == NULL){
		*recipient = NULL;
		return 0;
	}

	*recipient = calloc(sizeof(char), message_len + 1);
	memset(*recipient, '\0', message_len + 1);

	if((byte_read = read(sockfd, *recipient, message_len)) <= 0){
		if(byte_read < 0) perror("Error in receive_message_from on readv");
		return byte_read;
	}

	#ifdef PRINT_DEBUG_FINE
	printf("read has read %d bytes, message=%s\n",
		byte_read, *recipient);
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
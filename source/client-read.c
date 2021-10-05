#include "client.h"

int read_all(int sockfd, user_info client_ui){
	operation op;

	if(send_message_to(sockfd, client_ui.uid, OP_READ_REQUEST, NULL) < 0)
		return -1;

	while(receive_operation_from(sockfd, &op) == 0){
		switch(op.code){
			case OP_READ_RESPONSE:
				// Body ends with a single '\n'
				printf("%s\n%s\n%s\n", SEP, op.text, SEP);
				free(op.text);
				break;
			case OP_OK:
			case OP_NOT_ACCEPTED:
				return 0;
		}
	}

	return 0;
}
